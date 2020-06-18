#pragma once


#include <string>
#include <memory>
#include <vector>
#include <unordered_map>


namespace TDEngine2
{
	class ITypeVisitor;
	class SymTable;


	struct TType
	{
		using UniquePtr = std::unique_ptr<TType>;

		virtual ~TType() = default;

		virtual void Visit(ITypeVisitor& visitor) const;

		std::string mId;

		SymTable* mpOwner = nullptr;
	};

	
	struct TNamespaceType : TType
	{
		virtual ~TNamespaceType() = default;

		void Visit(ITypeVisitor& visitor)  const override;
	};


	struct TEnumType: TType
	{
		virtual ~TEnumType() = default;

		void Visit(ITypeVisitor& visitor) const override;

		bool                     mIsStronglyTyped = false;
		bool                     mIsIntrospectable = false;

		std::string              mMangledId; // \note contains full path to enum Namespace..ClassName@Enum
		std::string              mUnderlyingTypeStr = "int";

		std::vector<std::string> mEnumerators;
	};


	class ISymTableVisitor;


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

			void Visit(ISymTableVisitor& visitor);

			bool CreateScope(const std::string& name = "");
			bool EnterScope(const std::string& name = "");
			void ExitScope();

			void AddSymbol(TSymbolDesc&& desc);
			bool RemoveSymbol(const std::string& id);

			const TSymbolDesc& LookUpSymbol(const std::string& id) const;

			TScopeEntity* LookUpNamedScope(const std::string& name);

			void SetSourceFilename(const std::string& filename);

			std::string GetMangledNameForNamedScope(const std::string& id);

			const std::string& GetSourceFilename() const;
		private:
			bool _createAnonymousScope();
			bool _createNamedScope(const std::string& name);

			bool _visitAnonymousScope();
			bool _visitNamedScope(const std::string& name);

			const TSymbolDesc& _lookUpInternal(const std::string& id) const;
		private:
			TScopeEntity* mpGlobalScope;
			TScopeEntity* mpCurrScope;
			TScopeEntity* mpPrevScope;

			bool          mIsReadOnlyMode = false; // \note It's set to true if EnterScope is used

			int32_t       mLastVisitedScopeIndex = -1;
			int32_t       mPrevVisitedScopeIndex; ///< \note The field is only updated when visiting VisitNamedScope

			std::string   mSourceFilename;
	};


	/*!
		interface ITypeVisitor

		\brief Use this interface to implement own visitors of types that are stored within symbol tables
	*/

	class ITypeVisitor
	{
	public:
		virtual ~ITypeVisitor() = default;

		virtual void VisitBaseType(const TType& type) = 0;
		virtual void VisitEnumType(const TEnumType& type) = 0;
		virtual void VisitNamespaceType(const TNamespaceType& type) = 0;
	};


	/*!
		interface ISymTableVisitor

		\brief Use this interface to implement own visitors of symbol tables to extract information from them
	*/

	class ISymTableVisitor
	{
		public:
			virtual ~ISymTableVisitor() = default;

			virtual void VisitScope(const SymTable::TScopeEntity& scope) = 0;
			virtual void VisitNamedScope(const SymTable::TScopeEntity& namedScope) = 0;
	};


	/*!
		class EnumsMetaExtractor

		\brief The class extracts all enumerations declarations from symbol table and puts them in single contiguous array
	*/

	class EnumsMetaExtractor : public ISymTableVisitor, public ITypeVisitor
	{
		public:
			using TEnumsArray = std::vector<const TEnumType*>;
			using TEnumsHashMap = std::unordered_map<std::string, uint32_t>; // key is a full name of an enum which is consists of mangled name like the following Name@..@EnumName
		public:
			EnumsMetaExtractor() = default;
			virtual ~EnumsMetaExtractor() = default;

			void VisitScope(const SymTable::TScopeEntity& scope) override;
			void VisitNamedScope(const SymTable::TScopeEntity& namedScope) override;

			void VisitBaseType(const TType& type) override;
			void VisitEnumType(const TEnumType& type) override;
			void VisitNamespaceType(const TNamespaceType& type) override;

			const TEnumsArray& GetEnums() const;
		private:
			TEnumsHashMap mEnumsHashTable;
			TEnumsArray   mpEnums;
	};
}