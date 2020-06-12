#include <iostream>
#include <array>
#include "../include/common.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/symtable.h"


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

	// \note Run for each header parser utility
	for (size_t i = 0; i < symbolsPerFile.size(); ++i)
	{
		const std::string& currFilename = filesToProcess[i];

		std::cout << ((i == 0) ? "" : "\n") << "Process " << currFilename << " file... ";

		if (std::unique_ptr<IInputStream> pFileStream{ new FileInputStream(currFilename) })
		{
			if (!pFileStream->Open())
			{
				std::cerr << "\nError (" << currFilename << "): File's not found\n";
				continue;
			}

			Lexer lexer{ *pFileStream };

			symbolsPerFile[i] = std::make_unique<SymTable>();

			bool hasErrors = false;

			Parser{ lexer, *symbolsPerFile[i], [&currFilename, &hasErrors](auto&& error)
			{
				hasErrors = true;
				std::cerr << "\nError (" << currFilename << ")" << error.ToString();
			} }.Parse();

			if (!hasErrors)
			{
				std::cout << "OK\n";
			}

			symbolsPerFile[i]->Visit(enumsExtractor);
		}
	}

	// \todo Generate meta-information as cpp files

	return 0;
}