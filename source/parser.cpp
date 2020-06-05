#include "../include/parser.h"
#include "../include/lexer.h"
#include "../include/tokens.h"


namespace TDEngine2
{
	Parser::Parser(Lexer& lexer, const TOnErrorCallback& onErrorCallback):
		mpLexer(&lexer), mOnErrorCallback(onErrorCallback)
	{
	}

	void Parser::Parse()
	{
		_parseDeclarationSequence();
	}

	bool Parser::_parseDeclarationSequence()
	{
		while (_parseDeclaration()) {}

		return true;
	}

	bool Parser::_parseDeclaration()
	{
		return _parseNamespaceDefinition();
	}

	bool Parser::_parseNamespaceDefinition()
	{
		if (mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_NAMESPACE)
		{
			return false;
		}

		mpLexer->GetNextToken();

		return _parseAnonymusNamespaceDefinition() || _parseNamedNamespaceDefinition();
	}

	bool Parser::_parseNamedNamespaceDefinition()
	{
		if (!_expect(E_TOKEN_TYPE::TT_IDENTIFIER, mpLexer->GetCurrToken()))
		{
			return false;
		}

		mpLexer->GetNextToken();

		if (!_expect(E_TOKEN_TYPE::TT_OPEN_BRACE, mpLexer->GetCurrToken()))
		{
			return false;
		}

		mpLexer->GetNextToken();

		// \note parse the body of the namespace
		if (!_parseDeclarationSequence())
		{
			return false;
		}

		if (!_expect(E_TOKEN_TYPE::TT_CLOSE_BRACE, mpLexer->GetCurrToken()))
		{
			return false;
		}

		mpLexer->GetNextToken();

		return true;
	}

	bool Parser::_parseAnonymusNamespaceDefinition()
	{
		if (mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_OPEN_BRACE)
		{
			return false;
		}

		auto&& currToken = mpLexer->GetNextToken(); // eat {

		// \note parse the body of the namespace
		if (!_parseDeclarationSequence())
		{
			return false;
		}

		if (!_expect(E_TOKEN_TYPE::TT_CLOSE_BRACE, mpLexer->GetCurrToken()))
		{
			return false;
		}

		mpLexer->GetNextToken();

		return true;
	}

	bool Parser::_expect(E_TOKEN_TYPE expectedType, const TToken& token)
	{
		if (expectedType == token.mType)
		{
			return true;
		}
		
		if (mOnErrorCallback)
		{
			mOnErrorCallback({ TParserError::E_PARSER_ERROR_CODE::UNEXPECTED_SYMBOL });
		}

		return false;
	}
}