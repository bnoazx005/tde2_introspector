#pragma once


#include <functional>
#include <tuple>
#include <cstdint>


namespace TDEngine2
{
	class Lexer;
	struct TToken;
	enum class E_TOKEN_TYPE : uint16_t;


	struct TParserError
	{
		using TCursorPos = std::tuple<uint32_t, uint32_t>;

		enum class E_PARSER_ERROR_CODE: uint32_t
		{
			UNEXPECTED_SYMBOL,
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

			bool _expect(E_TOKEN_TYPE expectedType, const TToken& token);

			bool _parseDeclarationSequence();
			bool _parseDeclaration();

			bool _parseNamespaceDefinition();
			bool _parseNamedNamespaceDefinition();
			bool _parseAnonymusNamespaceDefinition();

			bool _parseBlockDeclaration();

			bool _parseEnumDeclaration();
			bool _parseEnumBody();
			bool _parseEnumeratorDefinition();
		private:
			Lexer*           mpLexer;

			TOnErrorCallback mOnErrorCallback;
	};
}