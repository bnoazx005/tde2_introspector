#include <iostream>
#include "../include/common.h"


using namespace TDEngine2;


int main(int argc, const char** argv)
{
	TIntrospectorOptions options = ParseOptions(argc, argv);
	if (!options.mIsValid)
	{
		return -1;
	}

	std::cout << "Input: " << options.mInputDirname << std::endl
		<< "Output: " << options.mOutputDirname << std::endl;

	// \note Scan given directory for cpp header files
	std::vector<std::string> filesToProcess = GetHeaderFiles(options.mInputDirname);
	if (filesToProcess.empty())
	{
		std::cout << "Nothing to process... Exit\n";
		return -1;
	}

	// \todo Run for each header parser utility
	// \todo Generate meta-information as cpp files

	return 0;
}