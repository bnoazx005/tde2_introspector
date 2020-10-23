#include "../include/lexer.h"
#include <cctype>


namespace TDEngine2
{
	const std::string FileInputStream::mEmptyStr = "";

	FileInputStream::FileInputStream(const std::string& filename):
		mFilename(filename)
	{
	}

	FileInputStream::~FileInputStream()
	{
		Close();
	}

	bool FileInputStream::Open()
	{
		if (mFileStream.is_open() || mFilename.empty())
		{
			return false;
		}

		mFileStream.open(mFilename);

		return mFileStream.is_open();
	}

	bool FileInputStream::Close()
	{
		if (mFileStream.is_open())
		{
			mFileStream.close();
			
			return true;
		}

		return false;
	}

	std::string FileInputStream::ReadLine()
	{
		std::string lineStr;

		if (!mFileStream.is_open())
		{
			return mEmptyStr;
		}

		std::getline(mFileStream, lineStr, '\n');
		
		return mFileStream.eof() ? lineStr : lineStr.append("\n");
	}


	const Lexer::TKeywordsMap Lexer::mReservedTokens
	{
		{ "namespace", E_TOKEN_TYPE::TT_NAMESPACE },
		{ "enum", E_TOKEN_TYPE::TT_ENUM },
		{ "class", E_TOKEN_TYPE::TT_CLASS },
		{ "struct", E_TOKEN_TYPE::TT_STRUCT },
		{ "public", E_TOKEN_TYPE::TT_PUBLIC },
		{ "protected", E_TOKEN_TYPE::TT_PROTECTED },
		{ "private", E_TOKEN_TYPE::TT_PRIVATE },
		{ "virtual", E_TOKEN_TYPE::TT_VIRTUAL },
		{ "override", E_TOKEN_TYPE::TT_OVERRIDE },
		{ "final", E_TOKEN_TYPE::TT_FINAL },
		{ "ENUM_META", E_TOKEN_TYPE::TT_ENUM_META_ATTRIBUTE },
		{ "CLASS_META", E_TOKEN_TYPE::TT_CLASS_META_ATTRIBUTE },
		{ "INTERFACE_META", E_TOKEN_TYPE::TT_INTERFACE_META_ATTRIBUTE },
		{ "{", E_TOKEN_TYPE::TT_OPEN_BRACE },
		{ "}", E_TOKEN_TYPE::TT_CLOSE_BRACE },
		{ ":", E_TOKEN_TYPE::TT_COLON },
		{ ";", E_TOKEN_TYPE::TT_SEMICOLON },
		{ "=", E_TOKEN_TYPE::TT_ASSIGN_OP },
		{ ",", E_TOKEN_TYPE::TT_COMMA },
		{ "<", E_TOKEN_TYPE::TT_LESS },
		{ ">", E_TOKEN_TYPE::TT_GREAT },
	};

	Lexer::Lexer(IInputStream& streamSource):
		mpStream(&streamSource), mCurrProcessedText()
	{
	}

	const TToken& Lexer::GetCurrToken()
	{
		if (mTokensQueue.empty())
		{
			return GetNextToken();
		}

		return *mTokensQueue.front();
	}

	const TToken& Lexer::GetNextToken()
	{
		if (!mTokensQueue.empty())
		{
			mTokensQueue.erase(mTokensQueue.cbegin());
		}

		mTokensQueue.emplace_back(_scanToken());

		return *mTokensQueue.front();
	}

	const TToken& Lexer::PeekToken(uint32_t offset)
	{
		return *mTokensQueue.front();
	}

	std::unique_ptr<TToken> Lexer::_scanToken()
	{
		char ch = ' ';

		std::unique_ptr<TToken> pRecognizedToken = nullptr;

		while ((ch = _getNextChar()) != EOF)
		{
			if (_skipComments())
			{
				continue;
			}

			while (std::isspace(ch = _getCurrChar()))
			{
				if (ch == '\n' || ch == '\r')
				{
					mCurrHorPosIndex = 0;
					++mCurrLineIndex;
				}
				
				_getNextChar();
			}

			if (_skipMacroDefinitions())
			{
				continue;
			}

			if ((pRecognizedToken = std::move(_parseNumbers())))
			{
				return std::move(pRecognizedToken);
			}

			if ((pRecognizedToken = std::move(_parseReservedKeywordsAndIdentifiers())))
			{
				return std::move(pRecognizedToken);
			}

			return std::make_unique<TToken>(E_TOKEN_TYPE::TT_UNKNOWN, std::tuple<uint32_t, uint32_t>(mCurrHorPosIndex, mCurrLineIndex));
		}

		/*while (!eof)
		{
			skip whitespaces
			skip comments
			read literals
			read identifiers and keywords
		}*/

		return std::make_unique<TToken>(E_TOKEN_TYPE::TT_EOF);
	}

	bool Lexer::_skipMacroDefinitions()
	{
		if (_getCurrChar() != '#')
		{
			return false;
		}
		
		int depth = 1;

		while (depth > 0)
		{
			char ch = _getNextChar();

			if (ch == '\\')
			{
				++depth;
			}

			if ((ch == '\n') || (ch == '\r') || (ch == -1))
			{
				--depth;
			}
		}

		return true;
	}

	char Lexer::_getCurrChar() const
	{
		return mCurrProcessedText.empty() ? EOF : mCurrProcessedText.front();
	}

	char Lexer::_getNextChar()
	{
		if (!mCurrProcessedText.empty())
		{
			mCurrProcessedText.erase(0, 1);
		}

		if (mCurrProcessedText.empty())
		{
			mCurrProcessedText.append(mpStream->ReadLine());
		}

		if (mCurrProcessedText.empty()) // \note if it's still empty then we've reached end of the text
		{
			return EOF;
		}

		++mCurrHorPosIndex;

		return mCurrProcessedText.front();
	}

	char Lexer::_peekNextChar(uint32_t offset)
	{
		if (offset >= mCurrProcessedText.size())
		{
			mCurrProcessedText.append(mpStream->ReadLine());
		}

		return mCurrProcessedText[offset];
	}

	std::unique_ptr<TToken> Lexer::_parseNumbers()
	{
		return {};
	}

	std::unique_ptr<TToken> Lexer::_parseReservedKeywordsAndIdentifiers()
	{
		char ch = _getCurrChar();

		std::string possibleIdentifier{ ch };

		// \note try to detect identifier
		if (std::isalpha(ch) || ch == '_')
		{
			while (((ch = _peekNextChar(1)) != EOF) && (std::isalnum(ch) || ch == '_'))
			{
				possibleIdentifier.push_back(ch);
				_getNextChar();
			}

			auto&& iter = mReservedTokens.find(possibleIdentifier);
			if (iter != mReservedTokens.cend())
			{
				return std::unique_ptr<TToken>(new TToken{ iter->second, { mCurrHorPosIndex, mCurrLineIndex } });
			}
			
			return std::unique_ptr<TToken>(new TIdentifierToken{ possibleIdentifier, { mCurrHorPosIndex, mCurrLineIndex } });
		}

		// \note try to detect symbol
		auto&& iter = mReservedTokens.find(possibleIdentifier);
		if (iter == mReservedTokens.cend())
		{
			if (_getCurrChar() == EOF)
			{
				return std::make_unique<TToken>(E_TOKEN_TYPE::TT_EOF, std::tuple<uint32_t, uint32_t>(mCurrHorPosIndex, mCurrLineIndex));
			}

			return nullptr;
		}

		return std::make_unique<TToken>(iter->second, std::tuple<uint32_t, uint32_t>(mCurrHorPosIndex, mCurrLineIndex));
	}

	bool Lexer::_skipComments()
	{
		if (_getCurrChar() != '/')
		{
			return false;
		}

		char ch = _peekNextChar(1);

		if (std::isspace(ch))
		{
			// increment counter of lines
			return false;
		}

		switch (ch)
		{
			case '/':
				_getNextChar(); // take '/'
				_getNextChar();
				_skipSingleLineComment();
				break;
			case '*':
				_getNextChar(); // take '/'
				_getNextChar();
				_skipMultiLineComment();
				break;
			default:
				return false;
		}

		return true;
	}

	void Lexer::_skipSingleLineComment()
	{
		char ch = ' ';

		while ((ch = _getNextChar()) != EOF && ch != '\n') {}

		// \todo increment counter of lines
	}

	void Lexer::_skipMultiLineComment()
	{
		char currCh = ' ';
		char nextCh = ' ';

		/*uint32_t x = mCurrPos;
		uint32_t y = mCurrLine;*/

		while (((currCh = _getNextChar()) != EOF && currCh != '*') ||
			(currCh == '*' && (nextCh = _peekNextChar(1)) != '/'))
		{
			_skipComments();
		}

		switch (currCh)
		{
			case EOF:
				// the end of the file was reached, but there is no end of the comment
				//OnErrorOutput.Invoke({ LE_INVALID_END_OF_MULTILINE_COMMENT, x, y });
				break;

			case '*':
				// try to read '/'
				currCh = _getNextChar();

				if (currCh != '/')
				{
					// invalid end of the multi-line comment
					//OnErrorOutput.Invoke({ LE_INVALID_END_OF_MULTILINE_COMMENT, x, y });
				}

				break;
		}
	}
}