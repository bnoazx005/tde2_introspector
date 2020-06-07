#pragma once


#include <string>
#include <memory>
#include <vector>
#include <unordered_map>


namespace TDEngine2
{
	struct TType
	{
		using UniquePtr = std::unique_ptr<TType>;

		std::string mId;
	};

	struct TEnumType: TType
	{
		std::vector<std::string> mEnumerators;
	};


	struct TSymbolDesc
	{
		std::string              mName;

		TType::UniquePtr         mType = nullptr;

		static const TSymbolDesc mInvalid;
	};

	bool operator!= (const TSymbolDesc& leftSymbol, const TSymbolDesc& rightSymbol);
	bool operator== (const TSymbolDesc& leftSymbol, const TSymbolDesc& rightSymbol);


	class SymTable
	{
		public:
			struct TScopeEntity
			{
				TScopeEntity* mpParentScope;

				std::vector<TScopeEntity*> mpNestedScopes;

				std::unordered_map<std::string, TScopeEntity*> mpNamedScopes;

				std::vector<TSymbolDesc> mVariables;

				int32_t mIndex = -1; // -1 for all named scopes

				std::unique_ptr<TType> mpType; // for anonymous scopes it's nullptr
			};
		public:
			SymTable();
			~SymTable();

			bool CreateScope(const std::string& name = "");
			bool EnterScope(const std::string& name = "");
			void ExitScope();

			void AddSymbol(TSymbolDesc&& desc);
			bool RemoveSymbol(const std::string& id);

			const TSymbolDesc& LookUpSymbol(const std::string& id) const;

			TScopeEntity* LookUpNamedScope(const std::string& name);
		private:
			bool _createAnonymousScope();
			bool _createNamedScope(const std::string& name);

			bool _visitAnonymousScope();
			bool _visitNamedScope(const std::string& name);
		private:
			TScopeEntity* mpGlobalScope;
			TScopeEntity* mpCurrScope;
			TScopeEntity* mpPrevScope;

			bool          mIsReadOnlyMode = false; // \note It's set to true if EnterScope is used

			int32_t       mLastVisitedScopeIndex = -1;
			int32_t       mPrevVisitedScopeIndex; ///< \note The field is only updated when visiting VisitNamedScope
	};
}