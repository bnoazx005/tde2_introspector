#pragma once


#include <string>
#include <vector>


namespace TDEngine2
{
#if TDE2_USE_NOEXCEPT
	#define TDE2_NOEXCEPT noexcept
#else 
	#define TDE2_NOEXCEPT 
#endif


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

	TIntrospectorOptions ParseOptions(int argc, const char** argv) TDE2_NOEXCEPT;

	std::vector<std::string> GetHeaderFiles(const std::string& directory) TDE2_NOEXCEPT;
}