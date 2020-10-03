#include "../include/common.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/symtable.h"
#include "../deps/argparse/argparse.h"
#include <iostream>
#include <experimental/filesystem>
#include <array>
#include <mutex>
#include <unordered_set>


namespace fs = std::experimental::filesystem;


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
		int taggedOnly = 0;

		const char* pOutputDirectory = nullptr;
		const char* pOutputFilename = nullptr;

		struct argparse_option options[] = {
			OPT_HELP(),
			OPT_GROUP("Basic options"),
			OPT_BOOLEAN('V', "version", &showVersion, "Print version info and exit"),
			OPT_STRING('O', "outdir", &pOutputDirectory, "Write output into specified <dirname>"),
			OPT_STRING('o', "outfile", &pOutputFilename, "Output file's name <filename>"),
			OPT_INTEGER('T', "num-threads", &numOfThreads, "A number of available threads to process a few header files simultaneously"),
			OPT_BOOLEAN('t', "tagged-only", &taggedOnly, "The flag enables a mode when only tagged with corresponding attributes types will be passed into output file"),
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
			utilityOptions.mOutputDirname = std::experimental::filesystem::path(pOutputDirectory).string();
		}		

		if (pOutputFilename)
		{
			utilityOptions.mOutputFilename = pOutputFilename;
		}

		if (numOfThreads <= 0 || numOfThreads > (std::numeric_limits<int>::max() / 2))
		{
			std::cerr << "Error: too many threads cound was specified\n";
			std::terminate();
		}

		utilityOptions.mCurrNumOfThreads = static_cast<uint16_t>(numOfThreads);
		
#if 0
		compilerOptions.mPrintFlags = pPrintArg ? ((strcmp(pPrintArg, "symtable-dump") == 0) ? PF_SYMTABLE_DUMP :
			((strcmp(pPrintArg, "targets") == 0) ?
				PF_COMPILER_TARGETS : 0x0)) : 0x0;
		compilerOptions.mEmitFlag = pEmitArg ? ((strcmp(pEmitArg, "llvm-ir") == 0) ?
			E_EMIT_FLAGS::EF_LLVM_IR :
			((strcmp(pEmitArg, "llvm-bc") == 0) ?
				E_EMIT_FLAGS::EF_LLVM_BC :
				((strcmp(pEmitArg, "asm") == 0) ?
					E_EMIT_FLAGS::EF_ASM :
					E_EMIT_FLAGS::EF_NONE))) : E_EMIT_FLAGS::EF_NONE;

		compilerOptions.mOptimizationLevel = std::min<U8>(3, std::max<U8>(0, compilerOptions.mOptimizationLevel)); // in range of [0; 3]
#endif
		return std::move(utilityOptions);
	}


	std::vector<std::string> GetHeaderFiles(const std::vector<std::string>& directories) TDE2_NOEXCEPT
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
}