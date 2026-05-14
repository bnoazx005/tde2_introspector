#pragma once


#include <cstdint>
#include <tuple>
#include <string>


namespace TDEngine2
{
	enum class E_TOKEN_TYPE: uint16_t
	{
		TT_EOF,

		// keywords
		TT_NAMESPACE,
		TT_IDENTIFIER,
		TT_ENUM,
		TT_CLASS,
		TT_STRUCT,
		TT_PUBLIC,
		TT_PRIVATE,
		TT_PROTECTED,
		TT_VIRTUAL,
		TT_OVERRIDE,
		TT_FINAL,
		TT_TEMPLATE,
		TT_BEGIN_IGNORE_SECTION,
		TT_END_IGNORE_SECTION,
		TT_SECTION,
		TT_CHAR,
		TT_CHAR16_T,
		TT_CHAR32_T,
		TT_WCHAR_T,
		TT_BOOL,
		TT_SHORT,
		TT_INT,
		TT_LONG,
		TT_SIGNED,
		TT_UNSIGNED,
		TT_FLOAT,
		TT_DOUBLE,
		TT_VOID,
		TT_AUTO,
		TT_DECLTYPE,
		TT_TYPEDEF,

		// symbols
		TT_COLON,
		TT_OPEN_BRACE,
		TT_CLOSE_BRACE,
		TT_OPEN_PARENTHES,
		TT_CLOSE_PARENTHES,
		TT_SEMICOLON,
		TT_ASSIGN_OP,
		TT_COMMA,
		TT_LESS,
		TT_GREAT,

		// attributes
		TT_ENUM_META_ATTRIBUTE,
		TT_CLASS_META_ATTRIBUTE,
		TT_INTERFACE_META_ATTRIBUTE,

		TT_NUMBER,
		TT_UNKNOWN,
	};


	struct TToken
	{
		using TCursorPos = std::tuple<uint32_t, uint32_t>;

		TToken() = default;
		TToken(E_TOKEN_TYPE type, const std::string& value = "", const TCursorPos & pos = {0, 0});

		E_TOKEN_TYPE mType = E_TOKEN_TYPE::TT_EOF;
		TCursorPos   mPos = { 0, 0 };
		std::string  mValue = "";
		bool         mIsValid = false;
	};


	std::string TokenTypeToString(const E_TOKEN_TYPE& type);
}