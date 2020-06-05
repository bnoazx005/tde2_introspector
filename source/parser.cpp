#include "../include/parser.h"
#include "../include/lexer.h"


namespace TDEngine2
{
	Parser::Parser(Lexer& lexer, const TOnErrorCallback& onErrorCallback):
		mpLexer(&lexer), mOnErrorCallback(onErrorCallback)
	{
	}

	void Parser::Parse()
	{
	}
}