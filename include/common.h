#pragma once


#include <string>
#include <vector>
#include <fstream>
#include <array>
#include <cstdint>
#include <sstream>


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


	class IOutputStream
	{
		public:
			virtual ~IOutputStream() = default;

			virtual bool Open() = 0;
			virtual bool Close() = 0;

			virtual bool WriteString(const std::string& data) = 0;
		protected:
			IOutputStream() = default;
	};


	class FileOutputStream : public IOutputStream
	{
		public:
			FileOutputStream() = delete;
			explicit FileOutputStream(const std::string& filename);
			virtual ~FileOutputStream();

			bool Open() override;
			bool Close() override;

			bool WriteString(const std::string& data) override;
		private:
			std::string mFilename;

			std::ofstream mFileStream;
	};


	class StringUtils
	{
		public:
			template <typename... TArgs>
			static std::string Format(const std::string& formatStr, TArgs&&... args)
			{
				constexpr uint32_t argsCount = sizeof...(args);

				std::array<std::string, argsCount> arguments;
				_convertToStringsArray<sizeof...(args), TArgs...>(arguments, std::forward<TArgs>(args)...);

				std::string formattedStr = formatStr;
				std::string currArgValue;
				std::string currArgPattern;

				currArgPattern.reserve(5);

				std::string::size_type pos = 0;

				/// \note replace the following patterns {i}
				for (uint32_t i = 0; i < argsCount; ++i)
				{
					currArgPattern = "{" + ToString<uint32_t>(i) + "}";
					currArgValue = arguments[i];

					while ((pos = formattedStr.find(currArgPattern)) != std::string::npos)
					{
						formattedStr.replace(pos, currArgPattern.length(), currArgValue);
					}
				}

				return formattedStr;
			}

			template <typename T>
			static std::string ToString(const T& arg)
			{
				std::ostringstream stream;
				stream << arg;
				return stream.str();
			}
	private:
		template <uint32_t size>
		static void _convertToStringsArray(std::array<std::string, size>& outArray) {}

		template <uint32_t size, typename Head, typename... Tail>
		static void _convertToStringsArray(std::array<std::string, size>& outArray, Head&& firstArg, Tail&&... rest)
		{
			outArray[size - 1 - sizeof...(Tail)] = ToString(std::forward<Head>(firstArg));
			_convertToStringsArray<size, Tail...>(outArray, std::forward<Tail>(rest)...);
		}
	};
}