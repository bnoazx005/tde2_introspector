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
}