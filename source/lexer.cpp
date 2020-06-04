#include "../include/lexer.h"


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
		{ "{", E_TOKEN_TYPE::TT_OPEN_BRACE },
		{ "}", E_TOKEN_TYPE::TT_CLOSE_BRACE },
		{ ":", E_TOKEN_TYPE::TT_COLON },
	};

	Lexer::Lexer(IInputStream& streamSource):
		mpStream(&streamSource), mCurrProcessedText()
	{
	}

	const TToken& Lexer::GetCurrToken() const
	{
		return mTokensQueue.front();
	}

	const TToken& Lexer::GetNextToken()
	{
		mTokensQueue.erase(mTokensQueue.cbegin());
		mTokensQueue.emplace_back(_scanToken());

		return mTokensQueue.front();
	}

	const TToken& Lexer::PeekToken(uint32_t offset)
	{
		return mTokensQueue.front();
	}

	TToken Lexer::_scanToken()
	{
		/*while (!eof)
		{
			skip whitespaces
			skip comments
			read literals
			read identifiers and keywords
		}*/

		return {};
	}

	char Lexer::_getCurrChar() const
	{
		return mCurrProcessedText.front();
	}

	char Lexer::_getNextChar()
	{
		mCurrProcessedText.erase(0, 1);

		if (mCurrProcessedText.empty())
		{
			mCurrProcessedText.append(mpStream->ReadLine());
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
}