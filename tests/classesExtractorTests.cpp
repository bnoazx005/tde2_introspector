#include <symtable.h>
#include <catch2/catch.hpp>


using namespace TDEngine2;

using Scope = SymTable::TScopeEntity;
using ScopePtr = std::unique_ptr<SymTable::TScopeEntity>;


TEST_CASE("ClassesExtractor's tests")
{
	SECTION("TestVisitScope_PassClassThatHasNonpublicAccessModifier_ExtractorHasNoOutput")
	{
		ScopePtr pClassScope = std::make_unique<Scope>();
		pClassScope->mpType = std::make_unique<TClassType>();
		if (TClassType* pEnumType = dynamic_cast<TClassType*>(pClassScope->mpType.get()))
		{
			pEnumType->mId = "TestClass";
			pEnumType->mAccessModifier = E_ACCESS_SPECIFIER_TYPE::PROTECTED;
		}

		ScopePtr pRootScope = std::make_unique<Scope>();
		pRootScope->mpNamedScopes["TestClass"] = std::move(pClassScope);

		ClassMetaExtractor extractor(E_EMIT_FLAGS::ALL);
		extractor.VisitScope(*pRootScope.get());

		auto&& classes = extractor.GetTypesInfo();
		REQUIRE(classes.empty());
	}
}
