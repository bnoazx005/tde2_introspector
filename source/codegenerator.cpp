#include "../include/codegenerator.h"
#include "../include/common.h"
#include <set>


namespace TDEngine2
{
	const std::string CodeGenerator::mEnumTraitTemplateSpecializationHeaderPattern = R"(
template <>
struct EnumTrait<{0}>
{
	static const bool         isOpaque = {1};
	static const unsigned int elementsCount = {2};

	static const std::array<EnumFieldInfo<{0}>, elementsCount> GetFields() 
	{ 
		static const std::array<EnumFieldInfo<{0}>, {2}> fields
		{
			{3}
		};

		return fields;
	}

	static std::string ToString({0} value)
	{
		static auto&& elements = GetFields();

		for (auto&& currElement : elements)
		{
			if (currElement.value == value)
			{
				return currElement.name;
			}
		}

		return "";
	}
};
)";

	const std::string CodeGenerator::mEnumeratorFieldPattern = "EnumFieldInfo<{0}> { {1}, \"{2}\" }";

	const std::string CodeGenerator::mTrueConstant = "true";
	const std::string CodeGenerator::mFalseConstant = "false";

	CodeGenerator::CodeGenerator()
	{
	}
	
	CodeGenerator::~CodeGenerator()
	{
		if (mpHeaderOutputStream)
		{
			mpHeaderOutputStream->Close();
		}
	}

	bool CodeGenerator::Init(const TOutputStreamFactoryFunctor& outputStreamsFactory, const std::string& outputFilename)
	{
		if (!outputStreamsFactory)
		{
			return false;
		}

		mOutputStreamFactoryFunctor = outputStreamsFactory;
		mOutputFilenamesName = outputFilename;

		mpHeaderOutputStream = outputStreamsFactory(outputFilename + ".h");

		if (!mpHeaderOutputStream)
		{
			return false;
		}

		if (!mpHeaderOutputStream->Open())
		{
			return false;
		}

		_writeHeaderPrelude();

		return true;
	}

	void CodeGenerator::VisitBaseType(const TType& type)
	{

	}

	void CodeGenerator::VisitEnumType(const TEnumType& type)
	{
		std::string fullEnumName = StringUtils::ReplaceAll(type.mMangledId, "@", "::");

		uint32_t enumeratorsCount = type.mEnumerators.size();

		std::string fieldsStr = "";

		auto&& enumerators = type.mEnumerators;

		for (uint32_t i = 0; i < enumeratorsCount; ++i)
		{
			auto&& currEnumerator = enumerators[i];

			fieldsStr
				.append(StringUtils::Format(mEnumeratorFieldPattern, fullEnumName, fullEnumName + "::" + currEnumerator, currEnumerator))
				.append(i + 1 < enumeratorsCount ? "," : "").append("\n\t\t\t");
		}

		mpHeaderOutputStream->WriteString(StringUtils::Format(mEnumTraitTemplateSpecializationHeaderPattern,
															  fullEnumName,
															  (type.mIsStronglyTyped ? mTrueConstant : mFalseConstant),
															  enumeratorsCount, fieldsStr));
	}

	void CodeGenerator::VisitNamespaceType(const TNamespaceType& type)
	{

	}

	bool CodeGenerator::WriteEnumsMetaData(const EnumsMetaExtractor& enumsMeta)
	{
		mpHeaderOutputStream->WriteString(R"(
/*
	enums' meta declarations
*/

)");

		auto&& enums = enumsMeta.GetEnums();

		std::set<std::string> neededIncludes;

		for (auto currEnumMeta : enums)
		{
			neededIncludes.insert(StringUtils::Format("#include \"{0}\"\n", currEnumMeta->mpOwner->GetSourceFilename()));
		}

		for (const std::string& currIncludeFilename : neededIncludes)
		{
			mpHeaderOutputStream->WriteString(currIncludeFilename);
		}

		for (auto currEnumMeta : enums)
		{
			currEnumMeta->Visit(*this);
		}

		return true;
	}

	void CodeGenerator::_writeHeaderPrelude()
	{
		mpHeaderOutputStream->WriteString("#pragma once\n\n");
		mpHeaderOutputStream->WriteString(GeneratedHeaderPrelude);
	}
}