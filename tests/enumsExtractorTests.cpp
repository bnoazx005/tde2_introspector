#include <symtable.h>
#include <catch2/catch.hpp>


using namespace TDEngine2;

using Scope = SymTable::TScopeEntity;
using ScopePtr = std::unique_ptr<SymTable::TScopeEntity>;


TEST_CASE("EnumExtractor's tests")
{
	SECTION("TestVisitScope_PassRootNode_ExtractsAllEnumsInRootNode")
	{
		ScopePtr pEnumScope = std::make_unique<Scope>();
		pEnumScope->mpType = std::make_unique<TEnumType>();
		if (TEnumType* pEnumType = dynamic_cast<TEnumType*>(pEnumScope->mpType.get()))
		{
			pEnumType->mId = "TestEnum";
			pEnumType->mIsStronglyTyped = true;

			pEnumType->mEnumerators.push_back("FIRST");
			pEnumType->mEnumerators.push_back("SECOND");
			pEnumType->mEnumerators.push_back("THIRD");
		}

		ScopePtr pRootScope = std::make_unique<Scope>();
		pRootScope->mpNamedScopes["TestEnum"] = std::move(pEnumScope);

		EnumsMetaExtractor extractor(E_EMIT_FLAGS::ALL);
		extractor.VisitScope(*pRootScope.get());

		auto&& enums = extractor.GetTypesInfo();
		REQUIRE((enums.size() == 1 && enums[0]->mId == "TestEnum" && enums[0]->mEnumerators.size() == 3));
	}

	
}
