#pragma once


#include <functional>
#include <tuple>
#include <cstdint>


namespace TDEngine2
{
	class Lexer;


	struct TParserError
	{
		using TCursorPos = std::tuple<uint32_t, uint32_t>;

		enum class E_PARSER_ERROR_CODE: uint32_t
		{
			UNKNOWN
		};

		E_PARSER_ERROR_CODE mCode;

		TCursorPos          mPos;
	};


	class Parser
	{
		public:
			using TOnErrorCallback = std::function<void(const TParserError&)>;
		public:
			explicit Parser(Lexer& lexer, const TOnErrorCallback& onErrorCallback);
			~Parser() = default;

			void Parse();
		private:
			Parser() = default;
		private:
			Lexer*           mpLexer;

			TOnErrorCallback mOnErrorCallback;
	};
}