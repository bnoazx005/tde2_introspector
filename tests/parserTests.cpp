#include <vector>
#include <string>
#include <parser.h>
#include "mockInputStream.h"
#include <catch2/catch.hpp>


using namespace TDEngine2;



TEST_CASE("Parser tests")
{
	SECTION("TestParse_PassEmptyProgram_ProcessWithoutErrors")
	{		
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				""
			} } };

		Lexer lexer(*stream);
		Parser(lexer, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();
	}

	SECTION("TestParse_PassProgramWithEmptyNamedNamespace_ProcessWithoutErrors")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"namespace Test {",
				"}"
			} } };

		Lexer lexer(*stream);
		Parser(lexer, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();
	}

	SECTION("TestParse_PassProgramWithEmptyAnonymusNamespace_ProcessWithoutErrors")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"namespace {",
				"}"
			} } };

		Lexer lexer(*stream);
		Parser(lexer, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();
	}
}