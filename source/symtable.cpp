#include "../include/symtable.h"
#include <algorithm>
#include <cassert>


namespace TDEngine2
{
	const TSymbolDesc TSymbolDesc::mInvalid { "", nullptr };

	bool operator!= (const TSymbolDesc& leftSymbol, const TSymbolDesc& rightSymbol)
	{
		return (leftSymbol.mName != rightSymbol.mName); // \todo add comparison of types
	}

	bool operator== (const TSymbolDesc& leftSymbol, const TSymbolDesc& rightSymbol)
	{
		return (leftSymbol.mName == rightSymbol.mName); // \todo add comparison of types
	}


	SymTable::SymTable():
		mpGlobalScope(new TScopeEntity), mpCurrScope(mpGlobalScope), mpPrevScope(nullptr)
	{
		mpGlobalScope->mpParentScope = nullptr;
	}

	SymTable::~SymTable()
	{
		// \todo add deletion
	}

	void SymTable::AddSymbol(TSymbolDesc&& desc)
	{
		assert(mpCurrScope);

		auto& symbols = mpCurrScope->mVariables;

		auto iter = std::find_if(symbols.begin(), symbols.end(), [&desc](const TSymbolDesc& entity) { return desc.mName == entity.mName; });
		if (iter != symbols.end()) // \note there is already symbol with the same name than update it
		{
			iter->mType = std::move(desc.mType);
			return;
		}

		symbols.emplace_back(std::forward<TSymbolDesc>(desc));
	}

	bool SymTable::CreateScope(const std::string& name)
	{
		assert(mpCurrScope);

		mIsReadOnlyMode = false;

		return name.empty() ? _createAnonymousScope() : _createNamedScope(name);
	}

	bool SymTable::EnterScope(const std::string& name)
	{
		assert(mpCurrScope);

		mIsReadOnlyMode = true;

		return name.empty() ? _visitAnonymousScope() : _visitNamedScope(name);
	}

	void SymTable::ExitScope()
	{
		int32_t currScopeIndex = mpCurrScope->mIndex;

		bool isUnnamedScope = currScopeIndex >= 0;

		mpCurrScope = mpCurrScope->mpParentScope;

		// \note move to next neighbour scope if we currently stay in unnamed one and there is this next neighbour
		if (mIsReadOnlyMode && isUnnamedScope && (mpCurrScope->mpNestedScopes.size() > currScopeIndex))
		{
			mLastVisitedScopeIndex = currScopeIndex;
		}

		// \note restore previous pointer when came back from VisitNamedScope
		if (mIsReadOnlyMode && !isUnnamedScope)
		{
			mLastVisitedScopeIndex = mPrevVisitedScopeIndex;
			mPrevVisitedScopeIndex = -1;
		}
	}

	const TSymbolDesc& SymTable::LookUpSymbol(const std::string& id) const
	{
		auto&& symbols = mpCurrScope->mVariables;

		auto&& iter = std::find_if(symbols.begin(), symbols.end(), [id](const TSymbolDesc& entity) { return id == entity.mName; });

		return (iter != symbols.cend()) ? *iter : TSymbolDesc::mInvalid;
	}

	SymTable::TScopeEntity* SymTable::LookUpNamedScope(const std::string& name)
	{
		auto&& namedScopes = mpCurrScope->mpNamedScopes;

		auto&& iter = namedScopes.find(name);
		if (iter == namedScopes.cend())
		{
			return nullptr;
		}

		return iter->second;
	}

	bool SymTable::_createAnonymousScope()
	{
		int32_t nextScopeIndex = static_cast<int32_t>(mpCurrScope->mpNestedScopes.size());

		TScopeEntity* pNewScope = new TScopeEntity;
		mpCurrScope->mpNestedScopes.push_back(pNewScope);
		
		pNewScope->mpParentScope = mpCurrScope;
		pNewScope->mIndex        = nextScopeIndex;

		mpCurrScope = pNewScope;

		return true;
	}

	bool SymTable::_createNamedScope(const std::string& name)
	{
		TScopeEntity* pNewScope = new TScopeEntity;

		if (mpCurrScope->mpNamedScopes.find(name) != mpCurrScope->mpNamedScopes.cend())
		{
			return false;
		}

		mpCurrScope->mpNamedScopes[name] = pNewScope;

		pNewScope->mpParentScope = mpCurrScope;
		pNewScope->mIndex        = -1;

		mpCurrScope = pNewScope;

		return true;
	}

	bool SymTable::_visitAnonymousScope()
	{
		mpPrevScope = mpCurrScope;
		mpCurrScope = mpCurrScope->mpNestedScopes[mLastVisitedScopeIndex + 1];

		mLastVisitedScopeIndex = -1;

		return true;
	}

	bool SymTable::_visitNamedScope(const std::string& name)
	{
		mpPrevScope = mpCurrScope;
		
		if (mpCurrScope->mIndex >= 0)
		{
			mPrevVisitedScopeIndex = mLastVisitedScopeIndex;
		}

		auto&& namedScopes = mpCurrScope->mpNamedScopes;

		auto&& iter = namedScopes.find(name);
		if (iter == namedScopes.cend())
		{
			return false;
		}

		mpCurrScope = iter->second;

		mLastVisitedScopeIndex = -1;

		return true;
	}
}