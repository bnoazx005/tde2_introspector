#pragma once


#include <string>


namespace TDEngine2
{
	static struct TVersion
	{
		const uint32_t mMajor = 0;
		const uint32_t mMinor = 1;
	} ToolVersion;

	struct TIntrospectorOptions
	{
		bool        mIsValid;

		std::string mInputDirname = ".";
		std::string mOutputDirname = ".";

		static TIntrospectorOptions mInvalid;
	};

	TIntrospectorOptions ParseOptions(int argc, const char** argv);
}