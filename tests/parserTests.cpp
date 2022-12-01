#include <vector>
#include <string>
#include <parser.h>
#include <symtable.h>
#include "mockInputStream.h"
#include <catch2/catch_test_macros.hpp>


using namespace TDEngine2;



TEST_CASE("Parser tests")
{
	const TIntrospectorOptions mockOptions = {};

	SECTION("TestParse_PassEmptyProgram_ProcessWithoutErrors")
	{		
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				""
			} } };

		Lexer lexer(*stream);
		SymTable symTable;

		Parser(lexer, symTable, mockOptions, [](auto&&)
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

		Parser(lexer, symTable, mockOptions, [](auto&&)
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

		Parser(lexer, symTable, mockOptions, [](auto&&)
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

		Parser(lexer, symTable, mockOptions, [](auto&&)
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

		Parser(lexer, symTable, mockOptions, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();
	}

	SECTION("TestParse_PassTaggedEnumDeclaration_ProcessWithoutErrors")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"ENUM_META() enum TEST;",
				"enum class TEST2;",
				"enum class TEST3 : unsigned int;",
			} } };

		Lexer lexer(*stream);
		SymTable symTable;

		Parser(lexer, symTable, mockOptions, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();

		auto pTestEnumScope = symTable.LookUpNamedScope("TEST");
		REQUIRE(pTestEnumScope);

		TEnumType* pTypeDesc = dynamic_cast<TEnumType*>(pTestEnumScope->mpType.get());
		REQUIRE((pTypeDesc && pTypeDesc->mIsMarkedWithAttribute));

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

		Parser(lexer, symTable, mockOptions, [](auto&&)
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

	SECTION("TestParse_PassEnumWithinNamespace_CorrectlyProcessIt")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"namespace Test {",
				" enum class Enum { };",
				"};"
			} } };

		Lexer lexer(*stream);
		SymTable symTable;

		Parser(lexer, symTable, mockOptions, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();
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

		Parser(lexer, symTable, mockOptions, [](auto&&)
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

		Parser(lexer, symTable, mockOptions, [](auto&&)
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

		Parser(lexer, symTable, mockOptions, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();
	}

	SECTION("TestParse_PassNestedEnum_CorrectlyProcessThisEnum")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"class A {",
				"public:",
				" enum class NestedEnum { First, Second, Third = 0x42 };",
				"private:",
				" enum class Test {};",
				"};"
			} } };

		Lexer lexer(*stream);
		SymTable symTable;

		Parser(lexer, symTable, mockOptions, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();


		symTable.EnterScope("A");
		{
			auto pNestedEnumScope = symTable.LookUpNamedScope("NestedEnum");
			REQUIRE(pNestedEnumScope);

			TEnumType* pTypeDesc = dynamic_cast<TEnumType*>(pNestedEnumScope->mpType.get());
			REQUIRE((pTypeDesc && E_ACCESS_SPECIFIER_TYPE::PUBLIC == pTypeDesc->mAccessModifier));

			auto&& enumerators = pTypeDesc->mEnumerators;

			REQUIRE((enumerators.size() == 3 &&
				enumerators[0] == "First" &&
				enumerators[1] == "Second" &&
				enumerators[2] == "Third"));

			auto pPrivateNestedEnum = symTable.LookUpNamedScope("Test"); 
			REQUIRE(pPrivateNestedEnum);

			pTypeDesc = dynamic_cast<TEnumType*>(pPrivateNestedEnum->mpType.get());
			REQUIRE((pTypeDesc && E_ACCESS_SPECIFIER_TYPE::PRIVATE == pTypeDesc->mAccessModifier));
		}
		symTable.ExitScope();
	}

	SECTION("TestParse_PassNestedEnumWithinNestedStruct_CorrectlyProcessThisEnum")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"struct A {",
				" struct B { enum class NestedEnum { First, Second, Third = 0x42 }; };",
				"};"
			} } };

		Lexer lexer(*stream);
		SymTable symTable;

		Parser(lexer, symTable, mockOptions, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();


		symTable.EnterScope("A");
		{
			symTable.EnterScope("B");
			{
				auto pNestedEnumScope = symTable.LookUpNamedScope("NestedEnum");
				REQUIRE(pNestedEnumScope);

				TEnumType* pTypeDesc = dynamic_cast<TEnumType*>(pNestedEnumScope->mpType.get());
				REQUIRE((pTypeDesc && E_ACCESS_SPECIFIER_TYPE::PUBLIC == pTypeDesc->mAccessModifier));

				auto&& enumerators = pTypeDesc->mEnumerators;

				REQUIRE((enumerators.size() == 3 &&
					enumerators[0] == "First" &&
					enumerators[1] == "Second" &&
					enumerators[2] == "Third"));

				REQUIRE(pNestedEnumScope->mpParentScope);

				TClassType* pParentType = dynamic_cast<TClassType*>(pNestedEnumScope->mpParentScope->mpType.get());
				REQUIRE((pParentType && pParentType->mId == "B"));
			}
			symTable.ExitScope();
		}
		symTable.ExitScope();
	}

	SECTION("TestParse_PassTemplateClasses_CorrectlyParsesTheirDeclarations")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream {
			{
				"class A {",
				" template <typename T> class B { };"
				"};",
				"template <typename T> class C;"
			} } };

		Lexer lexer(*stream);
		SymTable symTable;

		Parser(lexer, symTable, mockOptions, [](auto&&)
		{
			REQUIRE(false);
		}).Parse();

		/// \note Check B type

		symTable.EnterScope("A");
		{
			auto pNestedEnumScope = symTable.LookUpNamedScope("B");
			REQUIRE(pNestedEnumScope);

			auto pType = dynamic_cast<TClassType*>(pNestedEnumScope->mpType.get());
			REQUIRE((pType && pType->mIsTemplate));
		}
		symTable.ExitScope();

		/// \note Check C type
		auto pNestedEnumScope = symTable.LookUpNamedScope("C");
		REQUIRE(pNestedEnumScope);

		auto pType = dynamic_cast<TClassType*>(pNestedEnumScope->mpType.get());
		REQUIRE((pType && pType->mIsTemplate));
	}
}
