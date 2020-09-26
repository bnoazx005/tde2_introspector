#pragma once


#include <functional>
#include <tuple>
#include <cstdint>
#include <memory>
#include <string>


namespace TDEngine2
{
	class Lexer;
	class SymTable;

	struct TToken;
	class TType;
	struct TEnumType;

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

		union
		{
			struct TUnexpectedTokenErrorData
			{
				E_TOKEN_TYPE mActualToken;
				E_TOKEN_TYPE mExpectedToken;
			};

			TUnexpectedTokenErrorData mUnexpectedTokenErrData;
		} mData;

		std::string ToString() const;
	};


	std::string ParserErrorToString(TParserError::E_PARSER_ERROR_CODE errorCode);


	class Parser
	{
		public:
			using TOnErrorCallback = std::function<void(const TParserError&)>;
		public:
			Parser(Lexer& lexer, SymTable& symTable, const TOnErrorCallback& onErrorCallback);
			~Parser() = default;

			void Parse();
		private:
			Parser() = default;

			bool _expect(E_TOKEN_TYPE expectedType, const TToken& token);

			bool _parseDeclarationSequence();

			bool _parseNamespaceDefinition();
			bool _parseNamedNamespaceDefinition();
			bool _parseAnonymusNamespaceDefinition();

			bool _parseBlockDeclaration();

			bool _parseEnumDeclaration();
			bool _parseEnumBody(TEnumType* pEnumType);
			bool _parseEnumeratorDefinition(TEnumType* pEnumType);

			std::unique_ptr<TType> _parseTypeSpecifiers();

			bool _parseClassDeclaration();
			bool _parseClassHeader(const std::string& className);
			bool _parseClassBody(const std::string& className);

			std::string _parseClassIdentifier();
			std::string _parseSimpleTemplateIdentifier();

			bool _eatUnknownTokens();
		private:
			Lexer*           mpLexer;

			SymTable*        mpSymTable;

			TOnErrorCallback mOnErrorCallback;
	};
}