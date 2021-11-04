#include "../include/symtable.h"
#include "../deps/archive/archive.h"
#include <algorithm>
#include <cassert>


namespace TDEngine2
{
	bool TType::SafeSerialize(FileWriterArchive& archive, TType* pType)
	{
		if (!pType)
		{
			archive << static_cast<uint32_t>(E_SUBTYPE::UNKNOWN);
			return true;
		}

		return pType->Save(archive);
	}

	std::unique_ptr<TType> TType::Deserialize(FileReaderArchive& archive, SymTable* pSymTable)
	{
		uint32_t subtypeValue = 0;
		archive >> subtypeValue;

		std::unique_ptr<TType> pType = nullptr;

		switch (static_cast<TType::E_SUBTYPE>(subtypeValue))
		{
			case TType::E_SUBTYPE::BASE:
				pType = std::make_unique<TType>();
				break;
			case TType::E_SUBTYPE::ENUM:
				pType = std::make_unique<TEnumType>();
				break;
			case TType::E_SUBTYPE::CLASS:
				pType = std::make_unique<TClassType>();
				break;
			case TType::E_SUBTYPE::NAMESPACE:
				pType = std::make_unique<TNamespaceType>();
				break;
			case TType::E_SUBTYPE::UNKNOWN:
				break;
		}

		if (!pType)
		{
			return nullptr;
		}

		pType->Load(archive);
		pType->mpOwner = pSymTable;

		return pType;
	}

	bool TType::Load(FileReaderArchive& archive)
	{
		archive >> mId >> mMangledId;
		return true;
	}

	bool TType::Save(FileWriterArchive& archive)
	{
		archive << static_cast<uint32_t>(GetSubtype());
		archive << mId << mMangledId;

		return true;
	}

	void TType::Visit(ITypeVisitor& visitor) const
	{
		visitor.VisitBaseType(*this);
	}

	void TNamespaceType::Visit(ITypeVisitor& visitor) const
	{
		visitor.VisitNamespaceType(*this);
	}

	bool TEnumType::Load(FileReaderArchive& archive)
	{
		bool result = TType::Load(archive);

		archive >> mIsStronglyTyped >> mIsIntrospectable >> mIsForwardDeclaration;
		archive >> mUnderlyingTypeStr;

		uint32_t enumeratorsCount = 0;
		archive >> enumeratorsCount;

		std::string currEnumeratorValue;

		for (size_t i = 0; i < enumeratorsCount; ++i)
		{
			archive >> currEnumeratorValue;
			mEnumerators.push_back(currEnumeratorValue);
		}

		return result;
	}

	bool TEnumType::Save(FileWriterArchive& archive)
	{
		bool result = TType::Save(archive);

		archive << mIsStronglyTyped << mIsIntrospectable << mIsForwardDeclaration;
		archive << mUnderlyingTypeStr;

		archive << static_cast<uint32_t>(mEnumerators.size());

		for (auto&& currEnumeratorStr : mEnumerators)
		{
			archive << currEnumeratorStr;
		}

		return result;
	}

	void TEnumType::Visit(ITypeVisitor& visitor) const
	{
		visitor.VisitEnumType(*this);
	}

	bool TClassType::Load(FileReaderArchive& archive)
	{
		bool result = TType::Load(archive);

		archive >> mIsFinal >> mIsForwardDeclaration >> mIsStruct >> mIsTemplate;

		size_t baseClassesCount = 0;
		archive >> baseClassesCount;

		std::string fullNameStr;
		bool isVirtualInherited;
		uint32_t accessSpecifier;

		for (size_t i = 0; i < baseClassesCount; ++i)
		{
			archive >> fullNameStr;
			archive >> isVirtualInherited;
			archive >> accessSpecifier;

			mBaseClasses.push_back({ fullNameStr, isVirtualInherited, static_cast<E_ACCESS_SPECIFIER_TYPE>(accessSpecifier) });
		}

		return result;
	}

	bool TClassType::Save(FileWriterArchive& archive)
	{
		bool result = TType::Save(archive);

		archive << mIsFinal << mIsForwardDeclaration << mIsStruct << mIsTemplate;

		archive << mBaseClasses.size();

		for (auto&& baseClassEntity : mBaseClasses)
		{
			archive << baseClassEntity.mFullName;
			archive << baseClassEntity.mIsVirtualInherited;
			archive << static_cast<uint32_t>(baseClassEntity.mAccessSpecifier);
		}

		return result;
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
		mpGlobalScope(new TScopeEntity), mpCurrScope(mpGlobalScope.get()), mpPrevScope(nullptr)
	{
		mpGlobalScope->mpParentScope = nullptr;
	}

	SymTable::~SymTable()
	{
	}

	bool SymTable::TScopeEntity::Save(FileWriterArchive& archive)
	{
		archive << mpNestedScopes.size();

		for (auto&& pCurrScope : mpNestedScopes)
		{
			if (!pCurrScope)
			{
				continue; 
			}

			pCurrScope->Save(archive);
		}

		archive << mpNamedScopes.size();

		for (auto&& currScopeEntity : mpNamedScopes)
		{
			archive << currScopeEntity.first;

			if (!currScopeEntity.second)
			{
				continue;
			}

			currScopeEntity.second->Save(archive);
		}

		// \note Save variables
		archive << mVariables.size();

		for (auto&& currVariableInfo : mVariables)
		{
			archive << currVariableInfo.mName;
			
			TType::SafeSerialize(archive, currVariableInfo.mpType.get());
		}

		archive << (mIndex >= 0 ? mIndex : (std::numeric_limits<int>::max)());

		TType::SafeSerialize(archive, mpType.get());

		return true;
	}

	bool SymTable::TScopeEntity::Load(FileReaderArchive& archive, SymTable* symTable)
	{
		size_t nestedScopesCount = 0;
		archive >> nestedScopesCount;

		for (size_t i = 0; i < nestedScopesCount; ++i)
		{
			mpNestedScopes.emplace_back(std::make_unique<TScopeEntity>());

			auto pCurrScope = mpNestedScopes.back().get();

			pCurrScope->Load(archive, symTable);
			pCurrScope->mpParentScope = this;
		}

		size_t namedScopesCount = 0;
		archive >> namedScopesCount;

		std::string scopeName;

		for (size_t i = 0; i < namedScopesCount; ++i)
		{
			archive >> scopeName;

			mpNamedScopes.emplace(scopeName, std::make_unique<TScopeEntity>());
			
			auto pCurrScope = mpNamedScopes[scopeName].get();

			pCurrScope->Load(archive, symTable);
			pCurrScope->mpParentScope = this;
		}

		// \note Save variables
		size_t variablesCount = 0;
		archive >> variablesCount;

		mVariables.resize(variablesCount);

		std::string variableId;

		for (size_t i = 0; i < variablesCount; ++i)
		{
			archive >> variableId;

			mVariables[i].mName = variableId;
			mVariables[i].mpType = std::move(TType::Deserialize(archive, symTable));
		}

		archive >> mIndex;

		mIndex = (mIndex == (std::numeric_limits<int>::max)()) ? -1 : mIndex;

		mpType = std::move(TType::Deserialize(archive, symTable));

		return true;
	}

	bool SymTable::Save(FileWriterArchive& archive)
	{
		bool result = mpGlobalScope->Save(archive);

		archive << mSourceFilename;

		return result;
	}

	bool SymTable::Load(FileReaderArchive& archive)
	{
		_reset();

		mpGlobalScope = std::make_unique<TScopeEntity>();
		mpCurrScope = mpGlobalScope.get();

		bool result = mpGlobalScope->Load(archive, this);

		archive >> mSourceFilename;

		return result;
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
			iter->mpType = std::move(desc.mpType);
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

			return (iter->second).get();
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

	TType* SymTable::GetCurrScopeType() const
	{
		return mpCurrScope->mpType.get();
	}

	void SymTable::_reset()
	{
		mpGlobalScope = nullptr;
		mpCurrScope = nullptr;
		mpPrevScope = nullptr;

		mIsReadOnlyMode = false;

		mLastVisitedScopeIndex = -1;
		mPrevVisitedScopeIndex = -1;

		mSourceFilename = "";
	}

	bool SymTable::_createAnonymousScope()
	{
		int32_t nextScopeIndex = static_cast<int32_t>(mpCurrScope->mpNestedScopes.size());

		mpCurrScope->mpNestedScopes.emplace_back(std::make_unique<TScopeEntity>());

		TScopeEntity* pNewScope = mpCurrScope->mpNestedScopes.back().get();
		
		pNewScope->mpParentScope = mpCurrScope;
		pNewScope->mIndex        = nextScopeIndex;

		mpCurrScope = pNewScope;

		return true;
	}

	bool SymTable::_createNamedScope(const std::string& name)
	{
		if (mpCurrScope->mpNamedScopes.find(name) != mpCurrScope->mpNamedScopes.cend())
		{
			return false;
		}

		mpCurrScope->mpNamedScopes.emplace(name, std::make_unique<TScopeEntity>());

		TScopeEntity* pNewScope = mpCurrScope->mpNamedScopes[name].get();

		pNewScope->mpParentScope = mpCurrScope;
		pNewScope->mIndex        = -1;

		mpCurrScope = pNewScope;

		return true;
	}

	bool SymTable::_visitAnonymousScope()
	{
		mpPrevScope = mpCurrScope;
		mpCurrScope = mpCurrScope->mpNestedScopes[mLastVisitedScopeIndex + 1].get();

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

		mpCurrScope = (iter->second).get();

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

	EnumsMetaExtractor::EnumsMetaExtractor(const E_EMIT_FLAGS& flags):
		MetaExtractor(flags)
	{
	}

	void EnumsMetaExtractor::VisitEnumType(const TEnumType& type)
	{
		if ((mEmitFlags & E_EMIT_FLAGS::ENUMS) != E_EMIT_FLAGS::ENUMS || 
			E_ACCESS_SPECIFIER_TYPE::PUBLIC != type.mAccessModifier)
		{
			return;
		}

		if (auto pParentType = dynamic_cast<TClassType*>(type.mpParentType))
		{
			if (pParentType->mIsTemplate)
			{
				return;
			}
		}

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


	/*!
		\brief ClassesExtractor's definition
	*/

	ClassMetaExtractor::ClassMetaExtractor(const E_EMIT_FLAGS& flags) :
		MetaExtractor(flags)
	{
	}

	void ClassMetaExtractor::VisitClassType(const TClassType& type)
	{
		if ((!type.mIsStruct && ((mEmitFlags & E_EMIT_FLAGS::CLASSES) != E_EMIT_FLAGS::CLASSES)) || 
			(type.mIsStruct && ((mEmitFlags & E_EMIT_FLAGS::STRUCTS) != E_EMIT_FLAGS::STRUCTS)) ||
			type.mIsTemplate)
		{
			return;
		}

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