#include <symtable.h>
#include <catch2/catch_test_macros.hpp>


using namespace TDEngine2;


TEST_CASE("SymTable tests")
{
	SECTION("TestParse_PassGlobalVariables_AllowsLookUpsWithoutChangingScope")
	{
		SymTable symTable;

		symTable.AddSymbol({ "a", {} });
		symTable.AddSymbol({ "b", {} });
		symTable.AddSymbol({ "c", {} });

		REQUIRE(symTable.LookUpSymbol("a") != TSymbolDesc::mInvalid);
		REQUIRE(symTable.LookUpSymbol("b") != TSymbolDesc::mInvalid);
		REQUIRE(symTable.LookUpSymbol("c") != TSymbolDesc::mInvalid);
	}

	SECTION("TestParse_PassVariablesInNestedScopes_AllowsLookUpsWhileBeingInSomeScope")
	{
		SymTable symTable;

		symTable.AddSymbol({ "a", {} });

		symTable.CreateScope();
		symTable.AddSymbol({ "x", {} });
		symTable.ExitScope();

		REQUIRE(symTable.LookUpSymbol("a") != TSymbolDesc::mInvalid);
		REQUIRE(symTable.LookUpSymbol("x") == TSymbolDesc::mInvalid);

		symTable.EnterScope();
		REQUIRE(symTable.LookUpSymbol("a") != TSymbolDesc::mInvalid);
		REQUIRE(symTable.LookUpSymbol("x") != TSymbolDesc::mInvalid);
		symTable.ExitScope();
	}

	SECTION("TestParse_PassVariablesInNestedNamedScopes_AllowsLookUpsWhileBeingInSomeScope")
	{
		SymTable symTable;

		symTable.AddSymbol({ "a", {} });

		const std::string scopeName = "TestScope";

		symTable.CreateScope(scopeName);
		symTable.AddSymbol({ "x", {} });
		symTable.ExitScope();

		REQUIRE(symTable.LookUpSymbol("a") != TSymbolDesc::mInvalid);
		REQUIRE(symTable.LookUpSymbol("x") == TSymbolDesc::mInvalid);

		symTable.EnterScope(scopeName);
		REQUIRE(symTable.LookUpSymbol("x") != TSymbolDesc::mInvalid);
		symTable.ExitScope();
	}
}
