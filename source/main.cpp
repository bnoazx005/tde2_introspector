#include <iostream>
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

	// \todo Run for each header parser utility
	for (const std::string& currFilename : filesToProcess)
	{
		std::cout << "Process " << currFilename << " file...\n";

		if (std::unique_ptr<IInputStream> pFileStream{ new FileInputStream(currFilename) })
		{
			pFileStream->Open();

			Lexer lexer{ *pFileStream };

#if 0
			const TToken* currToken = &lexer.GetCurrToken();
			while ((currToken = &lexer.GetCurrToken())->mType != E_TOKEN_TYPE::TT_EOF)
			{
				std::cout << TokenTypeToString(currToken->mType) << std::endl;
				lexer.GetNextToken();
			}
#endif

			SymTable symTable;

			Parser{ lexer, symTable, [](auto&&)
			{
				std::cerr << "Error: " << std::endl;
			} }.Parse();
		}
		
	}

	// \todo Generate meta-information as cpp files

	return 0;
}