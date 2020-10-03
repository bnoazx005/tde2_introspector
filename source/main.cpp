#include <iostream>
#include <array>
#include <experimental/filesystem>
#include "../include/common.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/symtable.h"
#include "../include/codegenerator.h"
#include "../include/jobmanager.h"


using namespace TDEngine2;


int main(int argc, const char** argv)
{
	TIntrospectorOptions options = ParseOptions(argc, argv);
	if (!options.mIsValid)
	{
		return -1;
	}

	// \note Scan given directory for cpp header files
	std::vector<std::string> filesToProcess = GetHeaderFiles(options.mInputSources);
	if (filesToProcess.empty())
	{
		std::cout << "Nothing to process... Exit\n";
		return -1;
	}

	CodeGenerator::TSymbolTablesArray symbolsPerFile { filesToProcess.size() };

	{
		JobManager jobManager(options.mCurrNumOfThreads); // jobManager as a scoped object makes us possible to wait for all jobs will be done to the end of the scope

		// \note Build symbol tables for each header file
		for (size_t i = 0; i < filesToProcess.size(); ++i)
		{
			jobManager.SubmitJob(std::function<void()>([&filesToProcess, &symbolsPerFile, i]
			{
				symbolsPerFile[i] = std::move(ProcessHeaderFile(filesToProcess[i]));
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

	return 0;
}