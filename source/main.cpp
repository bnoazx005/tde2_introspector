#include <iostream>
#include <array>
#include <filesystem>
#include "../include/common.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/symtable.h"
#include "../include/codegenerator.h"
#include "../include/jobmanager.h"
#include "../deps/archive/archive.h"


using namespace TDEngine2;


int main(int argc, const char** argv)
{
	TIntrospectorOptions options = ParseOptions(argc, argv);
	if (!options.mIsValid)
	{
		return -1;
	}

	auto&& coutStreamBuffer = std::cout.rdbuf();

	if (!options.mIsLogOutputEnabled)
	{
		std::cout.set_rdbuf(nullptr); // \note disable standard console output here and restore at the end of execution
	}

	DEFER([coutStreamBuffer]
	{
		std::cout.set_rdbuf(coutStreamBuffer);
	});

	// \note Scan given directory for cpp header files
	const std::vector<std::string>& filesToProcess = GetHeaderFiles(options.mInputSources);
	if (filesToProcess.empty())
	{
		WriteOutput("Nothing to process... Exit\n");
		return -1;
	}

	auto&& cacheDirectory = std::filesystem::path(options.mCacheDirname);
	if (!std::filesystem::exists(cacheDirectory))
	{
		std::filesystem::create_directory(cacheDirectory);
	}

	auto&& outputDirectory = std::filesystem::path(options.mOutputDirname);
	if (!std::filesystem::exists(outputDirectory))
	{
		std::filesystem::create_directory(outputDirectory);
	}

	TCacheData cachedData;
	cachedData.Load(options.mCacheDirname, options.mCacheIndexFilename);

	if (cachedData.GetInputHash() != GetHashFromInputFiles(options.mInputSources)) // \note Reset cache if we introspect another headers pack
	{
		cachedData.Reset();
	}

	CodeGenerator::TSymbolTablesArray symbolsPerFile { filesToProcess.size() };

	{
		JobManager jobManager(options.mCurrNumOfThreads); // jobManager as a scoped object makes us possible to wait for all jobs will be done to the end of the scope

		// \note Build symbol tables for each header file
		for (size_t i = 0; i < filesToProcess.size(); ++i)
		{
			jobManager.SubmitJob(std::function<void()>([&filesToProcess, &symbolsPerFile, &cachedData, i, cacheDirectory = options.mCacheDirname]
			{
				const std::string& filename = filesToProcess[i];
				const std::string hash = GetHashFromFilePath(filename);

				const auto& cachePath = std::filesystem::path(cacheDirectory).concat(hash).string();

				if (cachedData.Contains(filename, hash))
				{
					// \note If the specified file exists then reuse data inside it
					std::ifstream symTableSourceFile(cachePath, std::ios::binary);

					if (symTableSourceFile.is_open())
					{
						WriteOutput(std::string("\n").append("Reuse cached version of ").append(filename).append(" file... "));

						Archive<std::ifstream> symTableSourceArchive(symTableSourceFile);

						symbolsPerFile[i] = std::make_unique<SymTable>();
						symbolsPerFile[i]->Load(symTableSourceArchive);

						symTableSourceFile.close();

						return;
					}					
				}

				symbolsPerFile[i] = std::move(ProcessHeaderFile(filename));

				// \note Serialize data
				{
					std::ofstream symTableOutputFile(cachePath, std::ios::binary);
					Archive<std::ofstream> symTableOutputArchive(symTableOutputFile);

					symbolsPerFile[i]->Save(symTableOutputArchive);

					symTableOutputFile.close();

					cachedData.AddSymTableEntity(filename, hash);
				}
			}));
		}
	}

	// \note Generate meta-information as cpp files
	CodeGenerator codeGenerator;

	const std::string outputFilename = std::filesystem::path(options.mOutputDirname + "/").concat(options.mOutputFilename).string();

	if (!codeGenerator.Init([](const std::string& filename) { return std::make_unique<FileOutputStream>(filename); }, outputFilename))
	{
		return -1;
	}

	if (!codeGenerator.Generate(std::move(symbolsPerFile)))
	{
		return -1;
	}

	// \note Update cache if the feature isn't disabled
	cachedData.SetInputHash(GetHashFromInputFiles(options.mInputSources));
	cachedData.Save(options.mCacheDirname, options.mCacheIndexFilename);

	return 0;
}