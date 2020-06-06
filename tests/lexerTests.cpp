#include <vector>
#include <string>
#include <lexer.h>
#include "mockInputStream.h"
#include <catch2/catch.hpp>


using namespace TDEngine2;


TEST_CASE("Lexer tests")
{
	SECTION("TestGetNextToken_PassKeywords_ReturnsSequenceOfTokens")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream { 
			{
				// lines here
				"namespace   {       }     ",
				"::",
				"enum class;"
			} } };

		Lexer lexer(*stream);

		const TToken* pCurrToken = nullptr;

		uint32_t currExpectedTokenIndex = 0;

		std::vector<E_TOKEN_TYPE> expectedTokens
		{
			E_TOKEN_TYPE::TT_NAMESPACE,
			E_TOKEN_TYPE::TT_OPEN_BRACE,
			E_TOKEN_TYPE::TT_CLOSE_BRACE,
			E_TOKEN_TYPE::TT_COLON,
			E_TOKEN_TYPE::TT_COLON,
			E_TOKEN_TYPE::TT_ENUM,
			E_TOKEN_TYPE::TT_CLASS,
			E_TOKEN_TYPE::TT_SEMICOLON,
		};

		while ((pCurrToken = &lexer.GetNextToken())->mType != E_TOKEN_TYPE::TT_EOF)
		{
			if (currExpectedTokenIndex >= expectedTokens.size())
			{
				REQUIRE(false);
				break;
			}

			REQUIRE(pCurrToken->mType == expectedTokens[currExpectedTokenIndex++]);
		}
	}

	SECTION("TestGetNextToken_PassOperators_ReturnsSequenceOfTokens")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"=,;:{}"
			} } };

		Lexer lexer(*stream);

		const TToken* pCurrToken = nullptr;

		uint32_t currExpectedTokenIndex = 0;

		std::vector<E_TOKEN_TYPE> expectedTokens
		{
			E_TOKEN_TYPE::TT_ASSIGN_OP,
			E_TOKEN_TYPE::TT_COMMA,
			E_TOKEN_TYPE::TT_SEMICOLON,
			E_TOKEN_TYPE::TT_COLON,
			E_TOKEN_TYPE::TT_OPEN_BRACE,
			E_TOKEN_TYPE::TT_CLOSE_BRACE,
		};

		while ((pCurrToken = &lexer.GetNextToken())->mType != E_TOKEN_TYPE::TT_EOF)
		{
			if (currExpectedTokenIndex >= expectedTokens.size())
			{
				REQUIRE(false);
				break;
			}

			REQUIRE(pCurrToken->mType == expectedTokens[currExpectedTokenIndex++]);
		}
	}

	SECTION("TestGetNextToken_PassSingleLineComment_ReturnsNothing")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"// This is a test",
			} } };

		Lexer lexer(*stream);

		REQUIRE(lexer.GetNextToken().mType == E_TOKEN_TYPE::TT_EOF);
	}

	SECTION("TestGetNextToken_PassSimpleMultiLineComments_ReturnsNothing")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"/* This is a test */",				
			} } };

		Lexer lexer(*stream);

		REQUIRE(lexer.GetNextToken().mType == E_TOKEN_TYPE::TT_EOF);
	}

	SECTION("TestGetNextToken_PassNestedMultiLineComments_ReturnsNothing")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"/* This is a test /* */ /**/ */",
			} } };

		Lexer lexer(*stream);

		REQUIRE(lexer.GetNextToken().mType == E_TOKEN_TYPE::TT_EOF);
	}

	SECTION("TestGetNextToken_PassMultiLineComments_ReturnsNothing")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"/* Test */",
				"/*",
				"Test"
				"*/",
			} } };

		Lexer lexer(*stream);

		REQUIRE(lexer.GetNextToken().mType == E_TOKEN_TYPE::TT_EOF);
	}
}