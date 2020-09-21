#pragma once


#include <string>
#include <vector>
#include <fstream>
#include <array>
#include <cstdint>
#include <sstream>
#include <memory>


namespace TDEngine2
{
#if TDE2_USE_NOEXCEPT
	#define TDE2_NOEXCEPT noexcept
#else 
	#define TDE2_NOEXCEPT 
#endif


	class SymTable;


	static struct TVersion
	{
		const uint32_t mMajor = 0;
		const uint32_t mMinor = 1;
	} ToolVersion;

	struct TIntrospectorOptions
	{
		static constexpr uint16_t mMaxNumOfThreads = 32;

		bool                      mIsValid;

		std::vector<std::string>  mInputSources { "." };
		std::string               mOutputDirname = ".";
		std::string               mOutputFilename = "metadata.h";

		uint16_t                  mCurrNumOfThreads = 1;

		static TIntrospectorOptions mInvalid;
	};

	TIntrospectorOptions ParseOptions(int argc, const char** argv) TDE2_NOEXCEPT;

	std::vector<std::string> GetHeaderFiles(const std::vector<std::string>& directories) TDE2_NOEXCEPT;
	
	void WriteOutput(const std::string& text) TDE2_NOEXCEPT;

	std::unique_ptr<SymTable> ProcessHeaderFile(const std::string& filename) TDE2_NOEXCEPT;


	const std::string GeneratedHeaderPrelude = R"(
#include <array>
#include <string>
#include <type_traits>


enum class Type: uint8_t
{
	Enum,
	Class,
	Struct,
	Function,
	Method,
	Unknown
};


/*!
	\brief The method computes 32 bits hash based on an input string's value.
	The underlying algorithm's description can be found here
	http://www.cse.yorku.ca/~oz/hash.html

	\param[in] pStr An input string
	\param[in] hash The argument is used to store current hash value during a recursion

	\return 32 bits hash of the input string
*/

constexpr uint32_t ComputeHash(const char* pStr, uint32_t hash = 5381)
{
	return (*pStr != 0) ? ComputeHash(pStr + 1, ((hash << 5) + hash) + *pStr) : hash;
}


enum class TypeID : uint32_t 
{
	Invalid = 0x0
};


#define TYPEID(TypeName) static_cast<TypeID>(ComputeHash(#TypeName))


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

template <typename TEnum>
struct EnumTrait
{
	static const bool         isOpaque = false;
	static const unsigned int elementsCount = 0;

    static const std::array<EnumFieldInfo<TEnum>, 0>& GetFields() { return {}; }
};


struct EnumInfo
{	
};


sturct ClassInfo
{
};


struct TypeInfo
{
    TypeID      mID;
	Type        mType;
	std::string mName;

	union
	{
		/// 
	}           mRawInfo;
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
			static std::string ReplaceAll(const std::string& input, const std::string& what, const std::string& replacement);

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