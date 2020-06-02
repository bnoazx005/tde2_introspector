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
}