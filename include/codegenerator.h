#pragma once


#include "symtable.h"


namespace TDEngine2
{
	class IOutputStream;


	class CodeGenerator: public ITypeVisitor
	{
		public:
			CodeGenerator() = delete;
			explicit CodeGenerator(IOutputStream& outputStream);
			~CodeGenerator();
		private:
	};
}