#pragma once


#include <cstdint>
#include <tuple>


namespace TDEngine2
{
	enum class E_TOKEN_TYPE: uint16_t
	{
		TT_EOF,

		// keywords
		TT_NAMESPACE,
		TT_IDENTIFIER,

		// symbols
		TT_COLON,
		TT_OPEN_BRACE,
		TT_CLOSE_BRACE,
		TT_UNKNOWN,
	};


	struct TToken
	{
		using TCursorPos = std::tuple<uint32_t, uint32_t>;

		E_TOKEN_TYPE mType = E_TOKEN_TYPE::TT_EOF;

		TCursorPos   mPos = { 0, 0 };
	};
}