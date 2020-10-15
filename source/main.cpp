#include <iostream>
#include <array>
#include <experimental/filesystem>
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

	// \note Scan given directory for cpp header files
	const std::vector<std::string>& filesToProcess = GetHeaderFiles(options.mInputSources);
	if (filesToProcess.empty())
	{
		std::cout << "Nothing to process... Exit\n";
		return -1;
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
				const std::string hash = GetHashFromFilePath(filesToProcess[i]);

				if (cachedData.Contains(filesToProcess[i]))
				{
					// \note Deserialize data
					std::ifstream symTableSourceFile(std::experimental::filesystem::path{ cacheDirectory }.concat(hash), std::ios::binary);
					Archive<std::ifstream> symTableSourceArchive(symTableSourceFile);

					symbolsPerFile[i] = std::make_unique<SymTable>();
					symbolsPerFile[i]->Load(symTableSourceArchive);

					symTableSourceFile.close();

					return;
				}

				symbolsPerFile[i] = std::move(ProcessHeaderFile(filesToProcess[i]));

				// \note Serialize data
				{
					std::ofstream symTableOutputFile(std::experimental::filesystem::path{ cacheDirectory }.concat(hash), std::ios::binary);
					Archive<std::ofstream> symTableOutputArchive(symTableOutputFile);

					symbolsPerFile[i]->Save(symTableOutputArchive);

					symTableOutputFile.close();

					cachedData.AddSymTableEntity(filesToProcess[i], hash);
				}
			}));
		}
	}

	// \note Generate meta-information as cpp files
	CodeGenerator codeGenerator;

	if (!codeGenerator.Init([](const std::string& filename) { return std::make_unique<FileOutputStream>(filename); }, options.mOutputFilename))
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