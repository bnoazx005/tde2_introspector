#pragma once


#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <fstream>


template<typename T> class Archive;


namespace TDEngine2
{
	class ITypeVisitor;
	class SymTable;
	using FileReaderArchive = Archive<std::ifstream>;
	using FileWriterArchive = Archive<std::ofstream>;


	struct TType
	{
		enum class E_SUBTYPE: uint32_t
		{
			BASE = 0,
			NAMESPACE,
			ENUM,
			CLASS,
			UNKNOWN
		};

		using UniquePtr = std::unique_ptr<TType>;

		virtual ~TType() = default;

		static bool SafeSerialize(FileWriterArchive& archive, TType* pType);
		static std::unique_ptr<TType> Deserialize(FileReaderArchive& archive, SymTable* pSymTable = nullptr);

		virtual bool Load(FileReaderArchive& archive);
		virtual bool Save(FileWriterArchive& archive);

		virtual void Visit(ITypeVisitor& visitor) const;

		virtual E_SUBTYPE GetSubtype() const { return E_SUBTYPE::BASE; }

		std::string mId;
		std::string mMangledId; // \note contains full path to type Namespace..ClassName@Type

		SymTable* mpOwner = nullptr;
	};

	
	struct TNamespaceType : TType
	{
		virtual ~TNamespaceType() = default;

		void Visit(ITypeVisitor& visitor)  const override;

		E_SUBTYPE GetSubtype() const override { return E_SUBTYPE::NAMESPACE; }
	};


	struct TEnumType: TType
	{
		virtual ~TEnumType() = default;

		bool Load(FileReaderArchive& archive) override;
		bool Save(FileWriterArchive& archive) override;

		void Visit(ITypeVisitor& visitor) const override;

		E_SUBTYPE GetSubtype() const override { return E_SUBTYPE::ENUM; }

		bool                     mIsStronglyTyped = false;
		bool                     mIsIntrospectable = false;
		bool                     mIsForwardDeclaration = false;

		std::string              mUnderlyingTypeStr = "int";

		std::vector<std::string> mEnumerators;
	};


	struct TClassType : TType
	{
		enum class E_ACCESS_SPECIFIER_TYPE : uint8_t
		{
			PUBLIC,
			PROTECTED,
			PRIVATE
		};

		struct TBaseClassInfo
		{
			std::string mFullName; /// Includes full path with namespaces 

			bool mIsVirtualInherited = false;
			
			E_ACCESS_SPECIFIER_TYPE mAccessSpecifier = E_ACCESS_SPECIFIER_TYPE::PRIVATE; // default access for classes is private as described in specification
		};

		virtual ~TClassType() = default;

		bool Load(FileReaderArchive& archive) override;
		bool Save(FileWriterArchive& archive) override;

		void Visit(ITypeVisitor& visitor) const override;

		E_SUBTYPE GetSubtype() const override { return E_SUBTYPE::CLASS; }

		bool mIsFinal = false;
		bool mIsStruct = false;

		std::vector<TBaseClassInfo> mBaseClasses;
	};


	class ISymTableVisitor;


	struct TSymbolDesc
	{
		std::string              mName;

		TType::UniquePtr         mpType = nullptr;

		static const TSymbolDesc mInvalid;
	};

	bool operator!= (const TSymbolDesc& leftSymbol, const TSymbolDesc& rightSymbol);
	bool operator== (const TSymbolDesc& leftSymbol, const TSymbolDesc& rightSymbol);


	class SymTable
	{
		public:
			struct TScopeEntity
			{
				using Ptr = std::unique_ptr<TScopeEntity>;

				bool Save(FileWriterArchive& archive);
				bool Load(FileReaderArchive& archive, SymTable* symTable);

				TScopeEntity* mpParentScope;

				std::vector<Ptr> mpNestedScopes;

				std::unordered_map<std::string, Ptr> mpNamedScopes;

				std::vector<TSymbolDesc> mVariables;

				int32_t mIndex = -1; // -1 for all named scopes

				std::unique_ptr<TType> mpType; // for anonymous scopes it's nullptr
			};
		public:
			SymTable();
			~SymTable();

			bool Save(FileWriterArchive& archive);
			bool Load(FileReaderArchive& archive);

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
			void _reset();

			bool _createAnonymousScope();
			bool _createNamedScope(const std::string& name);

			bool _visitAnonymousScope();
			bool _visitNamedScope(const std::string& name);

			const TSymbolDesc& _lookUpInternal(const std::string& id) const;
		private:
			TScopeEntity::Ptr mpGlobalScope;
			TScopeEntity*     mpCurrScope;
			TScopeEntity*     mpPrevScope;

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
		virtual void VisitClassType(const TClassType& type) = 0;
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


	template <typename Type>
	class MetaExtractor: public ISymTableVisitor, public ITypeVisitor
	{
		public:
			using TTypesArray = std::vector<const Type*>;
			using TTypesHashMap = std::unordered_map<std::string, uint32_t>; // key is a full name of an entity which is consists of mangled name like the following Name@..@TypeName
		
		public:
			MetaExtractor() = default;
			virtual ~MetaExtractor() = default;

			void VisitScope(const SymTable::TScopeEntity& scope) override
			{
				for (auto&& pCurrScope : scope.mpNestedScopes)
				{
					VisitScope(*pCurrScope);
				}

				for (auto&& currNamedScope : scope.mpNamedScopes)
				{
					VisitNamedScope(*currNamedScope.second);
				}
			}

			void VisitNamedScope(const SymTable::TScopeEntity& namedScope) override
			{
				if (TType* pScopeType = namedScope.mpType.get()) // \note this scope isn't namespace probably class, struct or enum
				{
					pScopeType->Visit(*this);
				}

				for (auto&& pCurrScope : namedScope.mpNestedScopes)
				{
					VisitScope(*pCurrScope);
				}

				for (auto&& currNamedScope : namedScope.mpNamedScopes)
				{
					VisitNamedScope(*currNamedScope.second);
				}
			}

			void VisitBaseType(const TType& type) override {}
			void VisitEnumType(const TEnumType& type) override {}
			void VisitNamespaceType(const TNamespaceType& type) override {}
			void VisitClassType(const TClassType& type) override {}

			const TTypesArray& GetTypesInfo() const { return mpTypesInfo; }

		protected:
			TTypesHashMap mTypesHashTable;
			TTypesArray   mpTypesInfo;
	};


	/*!
		class EnumsMetaExtractor

		\brief The class extracts all enumerations declarations from symbol table and puts them in single contiguous array
	*/

	class EnumsMetaExtractor : public MetaExtractor<TEnumType>
	{
		public:
			EnumsMetaExtractor() = default;
			virtual ~EnumsMetaExtractor() = default;

			void VisitEnumType(const TEnumType& type) override;
	};


	/*!
		\brief The class extracts information about declared classes
	*/

	class ClassMetaExtractor : public MetaExtractor<TClassType>
	{
		public:
			ClassMetaExtractor() = default;
			virtual ~ClassMetaExtractor() = default;

			void VisitClassType(const TClassType& type) override;
	};
}