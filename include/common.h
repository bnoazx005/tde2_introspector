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
		static constexpr uint16_t mMaxNumOfThreads = 32;

		bool                      mIsValid;

		std::string               mInputDirname = ".";
		std::string               mOutputDirname = ".";

		uint16_t                  mCurrNumOfThreads = 1;

		static TIntrospectorOptions mInvalid;
	};

	TIntrospectorOptions ParseOptions(int argc, const char** argv) TDE2_NOEXCEPT;

	std::vector<std::string> GetHeaderFiles(const std::string& directory) TDE2_NOEXCEPT;


	const std::string GeneratedHeaderPrelude = R"(
		/*
			\brief The section is auto generated code that contains all needed types, functcions and other
			infrastructure to provide correct work of meta-data
		*/

		template <typename TEnum>
		struct EnumFieldInfo
		{
			const TEnum       value;
			const std::string name;
		};

	)";
}