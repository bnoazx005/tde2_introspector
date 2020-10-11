#include <vector>
#include <string>
#include <parser.h>
#include <symtable.h>
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
		SymTable symTable;

		Parser(lexer, symTable, [](auto&&)
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
		SymTable symTable;

		Parser(lexer, symTable, [](auto&&)
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
		SymTable symTable;

		Parser(lexer, symTable, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();
	}

	SECTION("TestParse_PassEnumDeclaration_ProcessWithoutErrors")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"enum TEST;",
				"enum class TEST2;",
				"enum class TEST3 : unsigned int;",
			} } };

		Lexer lexer(*stream);
		SymTable symTable;

		Parser(lexer, symTable, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();
	}

	SECTION("TestParse_PassEnumDefinition_ProcessWithoutErrors")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"enum TEST { FIRST, SECOND, THIRD };",
				"enum class TEST2 { FIRST, SECOND = 0x2, THIRD };",
			} } };

		Lexer lexer(*stream);
		SymTable symTable;

		Parser(lexer, symTable, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();
	}

	SECTION("TestParse_PassEnumDefinition_ProcessWithoutErrors")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"enum class Test { Invalid = 0x0 };",
				"enum TEST { FIRST, SECOND, THIRD };",
			} } };

		Lexer lexer(*stream);
		SymTable symTable;

		Parser(lexer, symTable, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();

		auto pTestEnumScope = symTable.LookUpNamedScope("TEST");
		REQUIRE(pTestEnumScope);

		TEnumType* pTypeDesc = dynamic_cast<TEnumType*>(pTestEnumScope->mpType.get());
		REQUIRE(pTypeDesc);

		auto&& enumerators = pTypeDesc->mEnumerators;

		REQUIRE((enumerators.size() == 3 &&
			enumerators[0] == "FIRST" &&
			enumerators[1] == "SECOND" &&
			enumerators[2] == "THIRD"));
	}

	SECTION("TestParse_PassClassDeclaration_ProcessWithoutErrors")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"class A;",
				"class B { };",
				"class C: public B { };",
				"class D final : public B { };",
				"class E : D { };",
				"class F : protected C, public A { };",
			} } };

		Lexer lexer(*stream);
		SymTable symTable;

		Parser(lexer, symTable, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();
	}

	SECTION("TestParse_PassClassThatAppearedTwoTimes_CorrectlyRegisterForwardDeclarationAndUpdatesItLater")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"class A;",
				"class A { };"
			} } };

		Lexer lexer(*stream);
		SymTable symTable;

		Parser(lexer, symTable, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();
	}

	SECTION("TestParse_PassClassBody_CorrectlyProcessMembers")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"class A {",
				" ",
				"};"
			} } };

		Lexer lexer(*stream);
		SymTable symTable;

		Parser(lexer, symTable, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();
	}
}
