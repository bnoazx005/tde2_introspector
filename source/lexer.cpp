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

		return lineStr;
	}


	const Lexer::TKeywordsMap Lexer::mReservedTokens
	{
		{ "namespace", E_TOKEN_TYPE::TT_NAMESPACE },
		{ "enum", E_TOKEN_TYPE::TT_ENUM },
		{ "class", E_TOKEN_TYPE::TT_CLASS },
		{ "{", E_TOKEN_TYPE::TT_OPEN_BRACE },
		{ "}", E_TOKEN_TYPE::TT_CLOSE_BRACE },
		{ ":", E_TOKEN_TYPE::TT_COLON },
		{ ";", E_TOKEN_TYPE::TT_SEMICOLON },
	};

	Lexer::Lexer(IInputStream& streamSource):
		mpStream(&streamSource), mCurrProcessedText()
	{
	}

	const TToken& Lexer::GetCurrToken() const
	{
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
			while (std::isspace(ch = _getCurrChar()))
			{
				_getNextChar();
			}

			if (pRecognizedToken = std::move(_parseReservedKeywordsAndIdentifiers()))
			{
				return std::move(pRecognizedToken);
			}
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

	char Lexer::_getCurrChar() const
	{
		return mCurrProcessedText.front();
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
				return std::unique_ptr<TToken>(new TToken{ iter->second });
			}
			
			return std::unique_ptr<TToken>(new TIdentifierToken{ possibleIdentifier });
		}

		// \note try to detect symbol
		auto&& iter = mReservedTokens.find(possibleIdentifier);
		return std::make_unique<TToken>((iter != mReservedTokens.cend()) ? iter->second : E_TOKEN_TYPE::TT_UNKNOWN);
	}
}