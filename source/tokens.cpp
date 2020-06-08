#include "../include/tokens.h"


namespace TDEngine2
{
	TToken::TToken(E_TOKEN_TYPE type, const TCursorPos& pos):
		mType(type), mPos(pos)
	{
	}

	TIdentifierToken::TIdentifierToken(const std::string& id):
		TToken(E_TOKEN_TYPE::TT_IDENTIFIER, { 0, 0 }), mId(id)
	{
	}

	TNumberToken::TNumberToken(const std::string& value):
		TToken(E_TOKEN_TYPE::TT_NUMBER, { 0, 0 }), mValue(value)
	{
	}


	std::string TokenTypeToString(const E_TOKEN_TYPE& type)
	{
		switch (type)
		{
			case E_TOKEN_TYPE::TT_EOF:
				return "EOF";
			case E_TOKEN_TYPE::TT_NAMESPACE:
				return "NAMESPACE";
			case E_TOKEN_TYPE::TT_IDENTIFIER:
				return "ID";
			case E_TOKEN_TYPE::TT_ENUM:
				return "ENUM";
			case E_TOKEN_TYPE::TT_CLASS:
				return "CLASS";
			case E_TOKEN_TYPE::TT_STRUCT:
				return "STRUCT";
			case E_TOKEN_TYPE::TT_COLON:
				return "COLON";
			case E_TOKEN_TYPE::TT_OPEN_BRACE:
				return "OPEN_BRACE";
			case E_TOKEN_TYPE::TT_CLOSE_BRACE:
				return "CLOSE_BRACE";
			case E_TOKEN_TYPE::TT_SEMICOLON:
				return "SEMICOLON";
			case E_TOKEN_TYPE::TT_ASSIGN_OP:
				return "ASSIGN";
			case E_TOKEN_TYPE::TT_COMMA:
				return "COMMA";
			case E_TOKEN_TYPE::TT_NUMBER:
				return "NUMBER";
			case E_TOKEN_TYPE::TT_UNKNOWN:
				return "UNKNOWN";
		}

		return "";
	}
}