#include "../include/symtable.h"
#include <algorithm>
#include <cassert>


namespace TDEngine2
{

	void TType::Visit(ITypeVisitor& visitor) const
	{
		visitor.VisitBaseType(*this);
	}

	void TNamespaceType::Visit(ITypeVisitor& visitor) const
	{
		visitor.VisitNamespaceType(*this);
	}

	void TEnumType::Visit(ITypeVisitor& visitor) const
	{
		visitor.VisitEnumType(*this);
	}

	void TClassType::Visit(ITypeVisitor& visitor) const
	{
		visitor.VisitClassType(*this);
	}


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

	void SymTable::Visit(ISymTableVisitor& visitor)
	{
		visitor.VisitScope(*mpGlobalScope);
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
		return _lookUpInternal(id);
	}

	SymTable::TScopeEntity* SymTable::LookUpNamedScope(const std::string& name)
	{
		auto findScope = [name](TScopeEntity* pScope) -> TScopeEntity*
		{
			auto&& namedScopes = pScope->mpNamedScopes;

			auto&& iter = namedScopes.find(name);
			if (iter == namedScopes.cend())
			{
				return nullptr;
			}

			return iter->second;
		};
		
		if (TScopeEntity* pResult = findScope(mpCurrScope))
		{
			return pResult;
		}
		
		TScopeEntity* pCurrScope = mpCurrScope;
		while (pCurrScope->mpParentScope)
		{
			pCurrScope = pCurrScope->mpParentScope;

			if (TScopeEntity* pResult = findScope(pCurrScope))
			{
				return pResult;
			}
		}

		return nullptr;
	}

	std::string SymTable::GetMangledNameForNamedScope(const std::string& id)
	{
		TScopeEntity* pNamedScope = LookUpNamedScope(id);
		if (!pNamedScope)
		{
			return "";
		}

		std::string mangledId = id;

		TScopeEntity* pCurrScope = pNamedScope;
		while (pCurrScope->mpParentScope)
		{
			pCurrScope = pCurrScope->mpParentScope;

			if (pCurrScope->mIndex != -1 || !pCurrScope->mpType) // \note only named scopes influences onto the result id
			{
				continue;
			}

			if (auto pType = pCurrScope->mpType.get())
			{
				mangledId = pType->mId + "@" + mangledId;
			}
		}

		return mangledId;
	}

	void SymTable::SetSourceFilename(const std::string& filename)
	{
		mSourceFilename = filename;
	}

	const std::string& SymTable::GetSourceFilename() const
	{
		return mSourceFilename;
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

	const TSymbolDesc& SymTable::_lookUpInternal(const std::string& id) const
	{
		auto lookUp = [id](TScopeEntity* pScope) -> const TSymbolDesc*
		{
			auto&& symbols = pScope->mVariables;

			auto&& iter = std::find_if(symbols.begin(), symbols.end(), [id](const TSymbolDesc& entity) { return id == entity.mName; });

			if (iter != symbols.cend())
			{
				return &(*iter);
			}

			return nullptr;
		};
		
		if (auto pResult = lookUp(mpCurrScope))
		{
			return *pResult;
		}

		TScopeEntity* pCurrScope = mpCurrScope;

		while (pCurrScope->mpParentScope)
		{
			pCurrScope = pCurrScope->mpParentScope;

			if (auto pResult = lookUp(pCurrScope))
			{
				return *pResult;
			}
		}

		return TSymbolDesc::mInvalid;
	}


	/*!
		\brief EnumsExtractor's definition
	*/

	void EnumsMetaExtractor::VisitEnumType(const TEnumType& type)
	{
		auto iter = mTypesHashTable.find(type.mMangledId);
		if (iter != mTypesHashTable.cend())
		{
			if (const TEnumType* pOtherEnum = mpTypesInfo[iter->second])
			{
				if (!pOtherEnum->mEnumerators.empty()) // \note we can update the enum's meta if it was declared previously not defined
				{
					return;
				}

				mpTypesInfo[iter->second] = &type;
			}

			return;
		}

		mpTypesInfo.push_back(&type);
	}

	void ClassMetaExtractor::VisitClassType(const TClassType& type)
	{
		auto iter = mTypesHashTable.find(type.mMangledId);
		if (iter != mTypesHashTable.cend())
		{
			if (const TClassType* pOtherClass = mpTypesInfo[iter->second])
			{				
				mpTypesInfo[iter->second] = &type;
			}

			return;
		}

		mpTypesInfo.push_back(&type);
	}
}