#include "../include/common.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/symtable.h"
#include "../deps/argparse/argparse.h"
#include "../deps/PicoSHA2/picosha2.h"
#include "../deps/archive/archive.h"
#include "../deps/Wrench/source/stringUtils.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <array>
#include <mutex>
#include <unordered_set>
#include <cstring>


namespace fs = std::filesystem;


namespace TDEngine2
{
	TIntrospectorOptions TIntrospectorOptions::mInvalid { false };

	constexpr const char* Usage[] =
	{
		"tde2_introspector <input> .. <input> [options]",
		"where <usage> - single file path or directory",
		0
	};


	TIntrospectorOptions ParseOptions(int argc, const char** argv) TDE2_NOEXCEPT
	{
		int showVersion = 0;
		int numOfThreads = 1;

		// flags
		int taggedOnly = 0;
		int suppressLogOutput = 0;
		int forceMode = 0;
		int emitFlags = 0;

		const char* pOutputDirectory = nullptr;
		const char* pOutputFilename = nullptr;
		const char* pExcludedPathsStr = nullptr;

		const char* pCacheOutputDirectory = nullptr;

		struct argparse_option options[] = {
			OPT_HELP(),
			OPT_GROUP("Basic options"),
			OPT_BOOLEAN('V', "version", &showVersion, "Print version info and exit"),
			OPT_STRING('O', "outdir", &pOutputDirectory, "Write output into specified <dirname>"),
			OPT_STRING('o', "outfile", &pOutputFilename, "Output file's name <filename>"),
			OPT_STRING('C', "cache-dir", &pOutputFilename, "All cache files will be written into the specified <dirname>"),
			OPT_INTEGER('T', "num-threads", &numOfThreads, "A number of available threads to process a few header files simultaneously"),
			OPT_BOOLEAN('t', "tagged-only", &taggedOnly, "The flag enables a mode when only tagged with corresponding attributes types will be passed into output file"),
			OPT_BOOLEAN('q', "quiet", &suppressLogOutput, "Enables suppresion of program's output"),
			OPT_BOOLEAN('F', "force", &forceMode, "Enables force mode for the utility, all cached data will be ignored"),
			OPT_BIT(0, "emit-enums", &emitFlags, "Enables code generation for enumerations", NULL, static_cast<int>(E_EMIT_FLAGS::ENUMS), OPT_NONEG),
			OPT_BIT(0, "emit-classes", &emitFlags, "Enables code generation for classes", NULL, static_cast<int>(E_EMIT_FLAGS::CLASSES), OPT_NONEG),
			OPT_BIT(0, "emit-structs", &emitFlags, "Enables code generation for structures", NULL, static_cast<int>(E_EMIT_FLAGS::STRUCTS), OPT_NONEG),
			OPT_STRING(0, "exclude-paths", &pExcludedPathsStr, "Paths that should be excluded from introspection in the following format \"<path1>;<path2>;...\""),
			OPT_END(),
		};

		struct argparse argparse;
		argparse_init(&argparse, options, Usage, 0);
		argparse_describe(&argparse, "\nThe utility is a introspection tool for C++ language which is a part of TDE2 game engine", "\n");
		argc = argparse_parse(&argparse, argc, argv);

		if (showVersion)
		{
			std::cout << "tde2_introspector, version " << ToolVersion.mMajor << "." << ToolVersion.mMinor << std::endl;
			exit(0);
		}

		TIntrospectorOptions utilityOptions;

		utilityOptions.mIsValid = true;
		utilityOptions.mIsTaggedOnlyModeEnabled = static_cast<bool>(taggedOnly);
		utilityOptions.mIsLogOutputEnabled = !static_cast<bool>(suppressLogOutput);
		utilityOptions.mIsForceModeEnabled = static_cast<bool>(forceMode);

		// \note parse input files before any option, because argparse library will remove all argv's values after it processes that
		if (argc >= 1)
		{
			auto& sources = utilityOptions.mInputSources;
			sources.clear();

			for (int i = 0; i < argc; ++i)
			{
				sources.push_back(argparse.out[i]);
			}
		}

		if (utilityOptions.mInputSources.empty())
		{
			std::cerr << "Error: no input found\n";
			std::terminate();
		}

		if (pOutputDirectory)
		{
			utilityOptions.mOutputDirname = std::filesystem::path(pOutputDirectory).string();
		}		

		if (pOutputFilename)
		{
			utilityOptions.mOutputFilename = pOutputFilename;
		}

		if (pCacheOutputDirectory)
		{
			utilityOptions.mCacheDirname = pCacheOutputDirectory;
		}

		if (numOfThreads <= 0 || numOfThreads > (std::numeric_limits<int>::max() / 2))
		{
			std::cerr << "Error: too many threads cound was specified\n";
			std::terminate();
		}

		utilityOptions.mCurrNumOfThreads = static_cast<uint16_t>(numOfThreads);
		utilityOptions.mEmitFlags = static_cast<E_EMIT_FLAGS>(emitFlags);

		if (pExcludedPathsStr)
		{
			utilityOptions.mPathsToExclude = Wrench::StringUtils::Split(std::string(pExcludedPathsStr), ";");
		}

		utilityOptions.mPathsToExclude.push_back(utilityOptions.mOutputFilename);
		
		return std::move(utilityOptions);
	}


	std::vector<std::string> GetHeaderFiles(const std::vector<std::string>& directories, const std::vector<std::string>& excludedPaths) TDE2_NOEXCEPT
	{
		if (directories.empty())
		{
			return {};
		}

		static const std::array<std::string, 2> extensions { ".h", ".hpp" };

		auto&& hasValidExtension = [=](const std::string& ext)
		{
			return (ext == extensions[0]) || (ext == extensions[1]);
		};
		
		std::unordered_set<std::string> processedPaths; // contains absolute paths that already have been processed 

		std::vector<std::string> headersPaths;

		for (auto&& currSource : directories)
		{
			// files
			if (!fs::is_directory(currSource))
			{
				auto&& path = fs::path{ currSource };

				auto&& absPathStr = fs::canonical(currSource).string();

				if ((processedPaths.find(absPathStr) == processedPaths.cend()) && hasValidExtension(path.extension().string()))
				{
					headersPaths.emplace_back(currSource);
					processedPaths.emplace(absPathStr);
				}

				continue;
			}

			// directories
			for (auto&& directory : fs::recursive_directory_iterator{ currSource })
			{
				auto&& path = directory.path();

				auto&& absPathStr = fs::canonical(path).string();

				if ((processedPaths.find(absPathStr) == processedPaths.cend()) && hasValidExtension(path.extension().string()))
				{
					headersPaths.emplace_back(path.string());
					processedPaths.emplace(absPathStr);
				}
			}
		}

		// \note exclude some paths from the list
		for (auto&& currPathToExclude : excludedPaths)
		{
			std::string canonicalExcludingPath = StringUtils::ReplaceAll(currPathToExclude, "\\", "/");

			auto iter = headersPaths.cend();

			while ((iter = std::find_if(headersPaths.cbegin(), headersPaths.cend(), [&canonicalExcludingPath](auto&& headerPath)
					{
						return StringUtils::ReplaceAll(headerPath, "\\", "/").find(canonicalExcludingPath) != std::string::npos; // \todo refactor this later
					})) != headersPaths.cend())
			{
				headersPaths.erase(iter);
			}
		}

		return headersPaths;
	}


	static std::mutex StdoutMutex;

	void WriteOutput(const std::string& text)
	{
		std::lock_guard<std::mutex> lock(StdoutMutex);
		std::cout << text;
	}

	std::unique_ptr<SymTable> ProcessHeaderFile(const std::string& filename) TDE2_NOEXCEPT
	{
		WriteOutput(std::string("\n").append("Process ").append(filename).append(" file... "));

		if (std::unique_ptr<IInputStream> pFileStream{ new FileInputStream(filename) })
		{
			if (!pFileStream->Open())
			{
				WriteOutput(std::string("\nError (").append(filename).append("): File's not found\n"));
				return nullptr;
			}

			Lexer lexer{ *pFileStream };

			std::unique_ptr<SymTable> pSymTable = std::make_unique<SymTable>();
			pSymTable->SetSourceFilename(fs::canonical(filename).string());

			bool hasErrors = false;

			Parser{ lexer, *pSymTable, [&filename, &hasErrors](auto&& error)
			{
				hasErrors = true;
				WriteOutput(std::string("\nError (").append(filename).append(")").append(error.ToString()));
			} }.Parse();

			if (!hasErrors)
			{
				WriteOutput("OK\n");
			}

			return pSymTable;
		}

		return nullptr;
	}


	/*!
		\brief FileOutputStream's definition
	*/

	FileOutputStream::FileOutputStream(const std::string& filename):
		IOutputStream(), mFilename(filename)
	{
	}

	FileOutputStream::~FileOutputStream()
	{
		Close();
	}

	bool FileOutputStream::Open()
	{
		if (mFilename.empty() || mFileStream.is_open())
		{
			return false;
		}

		mFileStream.open(mFilename);

		return true;
	}

	bool FileOutputStream::Close()
	{
		if (!mFileStream.is_open())
		{
			return true;
		}

		mFileStream.close();

		return true;
	}

	bool FileOutputStream::WriteString(const std::string& data)
	{
		if (!mFileStream.is_open())
		{
			return false;
		}

		mFileStream << data;

		return true;
	}


	std::string StringUtils::ReplaceAll(const std::string& input, const std::string& what, const std::string& replacement)
	{
		std::string output = input;

		std::string::size_type pos = 0;
		std::string::size_type whatStrLength = what.length();

		while ((pos = output.find_first_of(what)) != std::string::npos)
		{
			output = output.substr(0, pos) + replacement + output.substr(pos + whatStrLength);
		}

		return output;
	}

	bool TCacheData::Load(const std::string& cacheSourceDirectory, const std::string& cacheFilename)
	{
		std::ifstream inputFile(std::filesystem::path(cacheSourceDirectory).concat(cacheFilename), std::ios::binary);

		DEFER([&inputFile]
		{
			inputFile.close();
		});

		if (!inputFile.is_open())
		{
			return false;
		}

		Archive<std::ifstream> cacheArchive{ inputFile };

		cacheArchive >> mInputHash;
		
		size_t entitiesCount = 0;

		cacheArchive >> entitiesCount;

		std::string currPath;
		std::string currHash;

		for (size_t i = 0; i < entitiesCount; ++i)
		{
			cacheArchive >> currPath >> currHash;
			mSymTablesTable.emplace(currPath, currHash);
		}

		return true;
	}

	bool TCacheData::Save(const std::string& cacheSourceDirectory, const std::string& cacheFilename)
	{
		std::ofstream cacheFile(std::filesystem::path(cacheSourceDirectory).concat(cacheFilename), std::ios::binary);

		DEFER([&cacheFile]
		{
			cacheFile.close();
		});

		Archive<std::ofstream> cacheArchive { cacheFile };

		cacheArchive << mInputHash;

		cacheArchive << mSymTablesTable.size();

		for (auto&& currSymTableInfo : mSymTablesTable)
		{
			cacheArchive << currSymTableInfo.first << currSymTableInfo.second;
		}

		return true;
	}

	void TCacheData::Reset()
	{
		std::lock_guard<std::mutex> lock{ mMutex };

		mInputHash.clear();
		mSymTablesTable.clear();
	}

	void TCacheData::AddSymTableEntity(const std::string& filePath, const std::string& fileHash)
	{
		std::lock_guard<std::mutex> lock{ mMutex };

		if (mSymTablesTable.find(filePath) == mSymTablesTable.cend())
		{
			mSymTablesTable.emplace(filePath, fileHash);
			return;
		}

		mSymTablesTable[filePath] = fileHash;
	}

	bool TCacheData::Contains(const std::string& filePath, const std::string& fileHash) const
	{
		std::lock_guard<std::mutex> lock{ mMutex };

		auto iter = mSymTablesTable.find(filePath);
		
		return (iter != mSymTablesTable.cend()) && (iter->second == fileHash);
	}

	void TCacheData::SetInputHash(const std::string& hash)
	{
		std::lock_guard<std::mutex> lock{ mMutex };
		mInputHash = hash;
	}

	void TCacheData::SetSymTablesIndex(TCacheIndexTable&& table)
	{
		std::lock_guard<std::mutex> lock{ mMutex };
		std::swap(mSymTablesTable, table);
	}

	const TCacheData::TCacheIndexTable& TCacheData::GetSymTablesIndex() const
	{
		return mSymTablesTable;
	}

	const std::string& TCacheData::GetInputHash() const
	{
		return mInputHash;
	}


	std::string GetHashFromInputFiles(const std::vector<std::string>& inputFiles)
	{
		picosha2::hash256_one_by_one hashGenerator;

		hashGenerator.init();

		for (auto&& currInputFilePath : inputFiles)
		{
			hashGenerator.process(currInputFilePath.cbegin(), currInputFilePath.cend());
		}

		hashGenerator.finish();

		std::string outputHashStr;
		picosha2::get_hash_hex_string(hashGenerator, outputHashStr);

		return outputHashStr;
	}

	std::string GetHashFromFilePath(const std::string& value)
	{
		picosha2::hash256_one_by_one hashGenerator;

		hashGenerator.init();
		hashGenerator.process(value.cbegin(), value.cend());

		std::string timestampStr = std::to_string(std::filesystem::last_write_time(value).time_since_epoch().count());
		hashGenerator.process(timestampStr.cbegin(), timestampStr.cend());

		hashGenerator.finish();

		std::string outputHashStr;
		picosha2::get_hash_hex_string(hashGenerator, outputHashStr);

		return outputHashStr;
	}
}