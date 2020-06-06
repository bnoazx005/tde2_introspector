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
		const TToken* pCurrToken = nullptr;

		while ((pCurrToken = &mpLexer->GetCurrToken())->mType != E_TOKEN_TYPE::TT_EOF)
		{
			switch (pCurrToken->mType)
			{
				case E_TOKEN_TYPE::TT_NAMESPACE:
					_parseNamespaceDefinition();
					break;
				case E_TOKEN_TYPE::TT_ENUM:
					_parseEnumDeclaration();
					break;
				case E_TOKEN_TYPE::TT_CLOSE_BRACE: // there are some tokens that we can skip in this method
					return true;
				default:
					mpLexer->GetNextToken(); // just skip unknown tokens
					break;
			}
		}

		return true;
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

	bool Parser::_parseBlockDeclaration()
	{
		return _parseEnumDeclaration();
	}

	bool Parser::_parseEnumDeclaration()
	{
		if (E_TOKEN_TYPE::TT_ENUM != mpLexer->GetCurrToken().mType)
		{
			return false;
		}

		mpLexer->GetNextToken();

		// \note enum class case
		if (mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_CLASS)
		{

			mpLexer->GetNextToken();
		}

		if (!_expect(E_TOKEN_TYPE::TT_IDENTIFIER, mpLexer->GetCurrToken()))
		{
			return false;
		}

		mpLexer->GetNextToken();

		// \todo parse enumeration's underlying type
		if (mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_COLON)
		{
			mpLexer->GetNextToken();

			// \todo Replace it with parseEnumUnderlyingType
			while (mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_SEMICOLON && mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_OPEN_BRACE)
			{
				mpLexer->GetNextToken();
			}
		}

		if (mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_OPEN_BRACE)
		{
			mpLexer->GetNextToken(); // eat {

			_parseEnumBody();

			if (!_expect(E_TOKEN_TYPE::TT_CLOSE_BRACE, mpLexer->GetCurrToken()))
			{
				return false;
			}

			mpLexer->GetNextToken(); // eat }
		}

		if (!_expect(E_TOKEN_TYPE::TT_SEMICOLON, mpLexer->GetCurrToken()))
		{
			return false;
		}

		mpLexer->GetNextToken();

		return true;
	}

	bool Parser::_parseEnumBody()
	{
		while (_parseEnumeratorDefinition() && mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_COMMA) 
		{
			mpLexer->GetNextToken(); // eat ',' token
		}

		return true;
	}

	bool Parser::_parseEnumeratorDefinition()
	{
		if (!_expect(E_TOKEN_TYPE::TT_IDENTIFIER, mpLexer->GetCurrToken()))
		{
			return false;
		}

		mpLexer->GetNextToken();

		if (mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_ASSIGN_OP) /// \note try to parse value of the enumerator
		{
			mpLexer->GetNextToken();

			// \todo for now we just skip this part
			while (mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_COMMA && mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_EOF)
			{
				mpLexer->GetNextToken();
			}

			if (mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_EOF)
			{
				mOnErrorCallback({}); // \todo add correct error handling here
				return false;
			}
		}

		return true;
	}

	bool Parser::_eatUnknownTokens()
	{
		while (mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_UNKNOWN)
		{
			mpLexer->GetNextToken();
		}

		return mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_EOF;
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