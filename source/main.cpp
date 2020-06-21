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
	std::vector<std::string> filesToProcess = GetHeaderFiles(options.mInputDirname);
	if (filesToProcess.empty())
	{
		std::cout << "Nothing to process... Exit\n";
		return -1;
	}

	std::vector<std::unique_ptr<SymTable>> symbolsPerFile { filesToProcess.size() };

	EnumsMetaExtractor enumsExtractor;
	
	JobManager jobManager(options.mCurrNumOfThreads);

	// \note Run for each header parser utility
	for (size_t i = 0; i < symbolsPerFile.size(); ++i)
	{
		const std::string& currFilename = filesToProcess[i];

		WriteOutput(std::string((i == 0) ? "" : "\n").append("Process ").append(currFilename).append(" file... "));

		if (std::unique_ptr<IInputStream> pFileStream{ new FileInputStream(currFilename) })
		{
			if (!pFileStream->Open())
			{
				WriteOutput(std::string("\nError (").append(currFilename).append("): File's not found\n"));
				continue;
			}

			Lexer lexer{ *pFileStream };

			symbolsPerFile[i] = std::make_unique<SymTable>();
			symbolsPerFile[i]->SetSourceFilename(std::experimental::filesystem::absolute(currFilename).string());

			bool hasErrors = false;

			Parser{ lexer, *symbolsPerFile[i], [&currFilename, &hasErrors](auto&& error)
			{
				hasErrors = true;
				WriteOutput(std::string("\nError (").append(currFilename).append(")").append(error.ToString()));
			} }.Parse();

			if (!hasErrors)
			{
				WriteOutput("OK\n");
			}

			symbolsPerFile[i]->Visit(enumsExtractor);
		}
	}

	// \todo Generate meta-information as cpp files
	CodeGenerator codeGenerator;

	if (!codeGenerator.Init([](const std::string& filename) { return std::make_unique<FileOutputStream>(filename); }, options.mOutputFilename))
	{
		return -1;
	}

	codeGenerator.WriteEnumsMetaData(enumsExtractor);

	return 0;
}