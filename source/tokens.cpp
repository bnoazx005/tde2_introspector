#include "../include/tokens.h"


namespace TDEngine2
{
	TToken::TToken(E_TOKEN_TYPE type, const std::string& value, const TCursorPos& pos):
		mType(type), mPos(pos), mValue(value), mIsValid(true)
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
				return ":";
			case E_TOKEN_TYPE::TT_OPEN_BRACE:
				return "{";
			case E_TOKEN_TYPE::TT_CLOSE_BRACE:
				return "}";
			case E_TOKEN_TYPE::TT_SEMICOLON:
				return ";";
			case E_TOKEN_TYPE::TT_ASSIGN_OP:
				return "=";
			case E_TOKEN_TYPE::TT_COMMA:
				return ",";
			case E_TOKEN_TYPE::TT_NUMBER:
				return "NUMBER";
			case E_TOKEN_TYPE::TT_UNKNOWN:
				return "UNKNOWN";
		}

		return "";
	}
}