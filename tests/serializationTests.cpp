#include <symtable.h>
#include <common.h>
#include "../deps/archive/archive.h"
#include <catch2/catch.hpp>
#include <fstream>


using namespace TDEngine2;


const std::string TestSerializationFilename = "serialization_test";


TEST_CASE("Serialization tests")
{
	SECTION("TestEnumTypeSerializationDeserialization")
	{
		const std::string enumName = "Test";

		// Serialization
		{
			std::unique_ptr<TEnumType> pType = std::make_unique<TEnumType>();
			pType->mId = enumName;
			pType->mIsIntrospectable = true;
			pType->mEnumerators.push_back("FIRST");
			pType->mEnumerators.push_back("SECOND");
			pType->mEnumerators.push_back("THIRD");

			std::ofstream outfile(TestSerializationFilename);
			Archive<std::ofstream> archive(outfile);

			REQUIRE(pType->Save(archive));

			outfile.close();
		}
		
		// Deserialization
		{
			std::ifstream infile(TestSerializationFilename);
			Archive<std::ifstream> archive(infile);

			std::unique_ptr<TType> pType = TType::Deserialize(archive);
			REQUIRE(pType);

			TEnumType* pEnumType = dynamic_cast<TEnumType*>(pType.get());
			REQUIRE((pEnumType && pEnumType->mId == enumName && pEnumType->mEnumerators.size() == 3));

			infile.close();
		}		
	}

	SECTION("TestClassTypeSerializationDeserialization")
	{
		const std::string className = "Test";

		// Serialization
		{
			std::unique_ptr<TClassType> pType = std::make_unique<TClassType>();
			pType->mId = className;
			pType->mIsFinal = true;
			pType->mBaseClasses.push_back({ "A" });
			pType->mBaseClasses.push_back({ "B" });

			std::ofstream outfile(TestSerializationFilename);
			Archive<std::ofstream> archive(outfile);

			REQUIRE(pType->Save(archive));

			outfile.close();
		}

		// Deserialization
		{
			std::ifstream infile(TestSerializationFilename);
			Archive<std::ifstream> archive(infile);

			std::unique_ptr<TType> pType = TType::Deserialize(archive);
			REQUIRE(pType);

			TClassType* pClassType = dynamic_cast<TClassType*>(pType.get());
			REQUIRE((pClassType && pClassType->mId == className && pClassType->mBaseClasses.size() == 2));
			
			if (pClassType)
			{
				REQUIRE(pClassType->mBaseClasses[0].mFullName == "A");
				REQUIRE(pClassType->mBaseClasses[1].mFullName == "B");
			}

			infile.close();
		}
	}

	SECTION("TestNamespaceTypeSerializationDeserialization")
	{
		const std::string namespaceName = "Test";

		// Serialization
		{
			std::unique_ptr<TNamespaceType> pType = std::make_unique<TNamespaceType>();
			pType->mId = namespaceName;

			std::ofstream outfile(TestSerializationFilename);
			Archive<std::ofstream> archive(outfile);

			REQUIRE(pType->Save(archive));

			outfile.close();
		}

		// Deserialization
		{
			std::ifstream infile(TestSerializationFilename);
			Archive<std::ifstream> archive(infile);

			std::unique_ptr<TType> pType = TType::Deserialize(archive);
			REQUIRE(pType);

			TNamespaceType* pNamespaceType = dynamic_cast<TNamespaceType*>(pType.get());
			REQUIRE((pNamespaceType && pNamespaceType->mId == namespaceName));

			infile.close();
		}
	}

	SECTION("TestUnknownTypeSerializationDeserialization")
	{
		// Serialization
		{
			std::ofstream outfile(TestSerializationFilename);
			Archive<std::ofstream> archive(outfile);

			REQUIRE(TType::SafeSerialize(archive, nullptr));

			outfile.close();
		}

		// Deserialization
		{
			std::ifstream infile(TestSerializationFilename);
			Archive<std::ifstream> archive(infile);

			std::unique_ptr<TType> pType = TType::Deserialize(archive);
			REQUIRE(!pType);

			infile.close();
		}
	}

	SECTION("TestScopeEntitySerializationDeserialization")
	{
		// Serialization
		{
			std::unique_ptr<SymTable::TScopeEntity> pScope = std::make_unique<SymTable::TScopeEntity>();
			pScope->mIndex = 2;
			pScope->mVariables.push_back({ "a" });
			pScope->mVariables.push_back({ "b" });

			std::ofstream outfile(TestSerializationFilename);
			Archive<std::ofstream> archive(outfile);

			REQUIRE(pScope->Save(archive));

			outfile.close();
		}

		// Deserialization
		{
			std::ifstream infile(TestSerializationFilename);
			Archive<std::ifstream> archive(infile);

			std::unique_ptr<SymTable::TScopeEntity> pScope = std::make_unique<SymTable::TScopeEntity>();
			REQUIRE(pScope->Load(archive));

			REQUIRE(pScope->mIndex == 2);
			REQUIRE(pScope->mVariables.size() == 2);
			REQUIRE(pScope->mVariables[0].mName == "a");
			REQUIRE(pScope->mVariables[1].mName == "b");
			REQUIRE(pScope->mpNamedScopes.empty());
			REQUIRE(pScope->mpNestedScopes.empty());

			infile.close();
		}
	}

	SECTION("TestComplexScopeEntitySerializationDeserialization")
	{
		// Serialization
		{
			std::unique_ptr<SymTable::TScopeEntity> pScope = std::make_unique<SymTable::TScopeEntity>();
			pScope->mIndex = 2;
			pScope->mVariables.push_back({ "a" });
			pScope->mVariables.push_back({ "b" });
			pScope->mpNamedScopes["A"] = std::make_unique<SymTable::TScopeEntity>();
			pScope->mpNamedScopes["A"]->mIndex = 0;
			pScope->mpNamedScopes["A"]->mVariables.push_back({ "c" });
			pScope->mpNamedScopes["A"]->mVariables.push_back({ "d" });

			std::ofstream outfile(TestSerializationFilename);
			Archive<std::ofstream> archive(outfile);

			REQUIRE(pScope->Save(archive));

			outfile.close();
		}

		// Deserialization
		{
			std::ifstream infile(TestSerializationFilename);
			Archive<std::ifstream> archive(infile);

			std::unique_ptr<SymTable::TScopeEntity> pScope = std::make_unique<SymTable::TScopeEntity>();
			REQUIRE(pScope->Load(archive));

			REQUIRE(pScope->mIndex == 2);
			REQUIRE(pScope->mVariables.size() == 2);
			REQUIRE(pScope->mVariables[0].mName == "a");
			REQUIRE(pScope->mVariables[1].mName == "b");
			REQUIRE(pScope->mpNamedScopes.size() == 1);
			REQUIRE(pScope->mpNestedScopes.empty());

			SymTable::TScopeEntity* pNestedScope = pScope->mpNamedScopes["A"].get();

			REQUIRE(pNestedScope->mIndex == 0);
			REQUIRE(pNestedScope->mVariables.size() == 2);
			REQUIRE(pNestedScope->mVariables[0].mName == "c");
			REQUIRE(pNestedScope->mVariables[1].mName == "d");
			REQUIRE(pNestedScope->mpNamedScopes.empty());
			REQUIRE(pNestedScope->mpNestedScopes.empty());

			infile.close();
		}
	}
}
