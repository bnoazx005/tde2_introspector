#include "../include/tokens.h"


namespace TDEngine2
{
	TToken::TToken(E_TOKEN_TYPE type, const std::string& value, const TCursor& pos):
		mType(type), mPos(pos), mValue(value), mIsValid(true)
	{
	}
}