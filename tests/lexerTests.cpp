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

	/*SECTION("TestGetNextToken_PassNumberLiterals_ReturnsSequenceOfTokens")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"42"
			} } };

		Lexer lexer(*stream);

		const TToken* pCurrToken = nullptr;

		uint32_t currExpectedTokenIndex = 0;

		std::vector<E_TOKEN_TYPE> expectedTokens
		{
			E_TOKEN_TYPE::TT_NUMBER,
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
	}*/

	SECTION("TestGetNextToken_PassStronglyTypedEnum_ReturnsTokens")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"enum class TEST: U32 {",
				"\tFIRST,",
				"\tSECOND,",
				"\tTHIRD",
				"};"
			} } };

		Lexer lexer(*stream);

		const TToken* pCurrToken = nullptr;

		uint32_t currExpectedTokenIndex = 0;

		std::vector<E_TOKEN_TYPE> expectedTokens
		{
			E_TOKEN_TYPE::TT_ENUM,
			E_TOKEN_TYPE::TT_CLASS,
			E_TOKEN_TYPE::TT_IDENTIFIER,
			E_TOKEN_TYPE::TT_COLON,
			E_TOKEN_TYPE::TT_IDENTIFIER,
			E_TOKEN_TYPE::TT_OPEN_BRACE,
			E_TOKEN_TYPE::TT_IDENTIFIER,
			E_TOKEN_TYPE::TT_COMMA,
			E_TOKEN_TYPE::TT_IDENTIFIER,
			E_TOKEN_TYPE::TT_COMMA,
			E_TOKEN_TYPE::TT_IDENTIFIER,
			E_TOKEN_TYPE::TT_CLOSE_BRACE,
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

	SECTION("TestGetNextToken_PassMultilineStream_ReturnsCorrectLineIndexesNumbersInTokens")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"a",
				"b",
				"c",
				"d",
			} } };

		Lexer lexer(*stream);

		REQUIRE(lexer.GetNextToken().mPos == std::tuple<uint32_t, uint32_t>(1, 1));
		REQUIRE(lexer.GetNextToken().mPos == std::tuple<uint32_t, uint32_t>(1, 2));
		REQUIRE(lexer.GetNextToken().mPos == std::tuple<uint32_t, uint32_t>(1, 3));
		REQUIRE(lexer.GetNextToken().mPos == std::tuple<uint32_t, uint32_t>(1, 4));
		REQUIRE(lexer.GetNextToken().mType == E_TOKEN_TYPE::TT_EOF);
	}

	SECTION("TestGetNextToken_PassMultilineStream_ReturnsCorrectHorizontalLineIndexesNumbersInTokens")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"a b c d",
			} } };

		Lexer lexer(*stream);

		REQUIRE(lexer.GetNextToken().mPos == std::tuple<uint32_t, uint32_t>(1, 1));
		REQUIRE(lexer.GetNextToken().mPos == std::tuple<uint32_t, uint32_t>(3, 1));
		REQUIRE(lexer.GetNextToken().mPos == std::tuple<uint32_t, uint32_t>(5, 1));
		REQUIRE(lexer.GetNextToken().mPos == std::tuple<uint32_t, uint32_t>(7, 1));
		REQUIRE(lexer.GetNextToken().mType == E_TOKEN_TYPE::TT_EOF);
	}

	SECTION("TestGetNextToken_PassMacroDefinition_EatsMacroDefinitionAndReturnsEOFToken")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"#define FOO(x) x ## x",
				""
			} } };

		Lexer lexer(*stream);

		REQUIRE(lexer.GetNextToken().mType == E_TOKEN_TYPE::TT_EOF);
	}

	SECTION("TestGetNextToken_PassFewLinesMacroDefinitions_EatsMacroDefinitionAndReturnsEOFToken")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"#define FOO(x) \\",
				"{\\",
				" blah blah blah\\",
				"}",
				"test",
			} } };

		Lexer lexer(*stream);

		REQUIRE(lexer.GetNextToken().mType == E_TOKEN_TYPE::TT_IDENTIFIER);
		REQUIRE(lexer.GetNextToken().mType == E_TOKEN_TYPE::TT_EOF);
	}

	SECTION("TestGetNextToken_PassOperatorsWithIgnoreBlock_ReturnsSequenceOfTokensWithoutThatExistInbetween")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"id",
				"BEGIN_IGNORE_META_SECTION",
				"id2",
				"END_IGNORE_META_SECTION",
				"id3",
			} } };

		Lexer lexer(*stream);

		const TToken* pCurrToken = nullptr;

		uint32_t currExpectedTokenIndex = 0;

		std::vector<E_TOKEN_TYPE> expectedTokens
		{
			E_TOKEN_TYPE::TT_IDENTIFIER,
			E_TOKEN_TYPE::TT_IDENTIFIER,
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

		REQUIRE(lexer.GetNextToken().mType == E_TOKEN_TYPE::TT_EOF);
	}

	SECTION("TestGetNextToken_PassIgnoreBlockWithoutBeginMark_ReturnsAllTokensWithEndMarker")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"id",
				"END_IGNORE_META_SECTION",
				"id2",
				"id3",
			} } };

		Lexer lexer(*stream);


		const TToken* pCurrToken = nullptr;

		uint32_t currExpectedTokenIndex = 0;

		std::vector<E_TOKEN_TYPE> expectedTokens
		{
			E_TOKEN_TYPE::TT_IDENTIFIER,
			E_TOKEN_TYPE::TT_END_IGNORE_SECTION,
			E_TOKEN_TYPE::TT_IDENTIFIER,
			E_TOKEN_TYPE::TT_IDENTIFIER,
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

		REQUIRE(lexer.GetNextToken().mType == E_TOKEN_TYPE::TT_EOF);
	}

	SECTION("TestGetNextToken_PassIgnoreBlockWithoutEndMark_ReturnsNothingAfterBeginningMarker")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"id",
				"BEGIN_IGNORE_META_SECTION",
				"id2",
				"id3",
			} } };

		Lexer lexer(*stream);
		
		REQUIRE(lexer.GetNextToken().mType == E_TOKEN_TYPE::TT_IDENTIFIER);
		REQUIRE(lexer.GetNextToken().mType == E_TOKEN_TYPE::TT_EOF);
	}
}