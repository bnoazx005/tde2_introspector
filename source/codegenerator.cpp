#include "../include/codegenerator.h"
#include "../include/common.h"


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
};
)";

	const std::string CodeGenerator::mEnumTraitTemplateSpecializationSourcePattern = R"(

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

		if (mpSourceOutputStream)
		{
			mpSourceOutputStream->Close();
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
		mpSourceOutputStream = outputStreamsFactory(outputFilename + ".cpp");

		if (!mpHeaderOutputStream || !mpSourceOutputStream)
		{
			return false;
		}

		if (!mpHeaderOutputStream->Open() || !mpSourceOutputStream->Open())
		{
			return false;
		}

		_writeHeaderPrelude();
		_writeSourcePrelude();

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

		/*mpSourceOutputStream->WriteString(StringUtils::Format(mEnumTraitTemplateSpecializationSourcePattern,
															  fullEnumName,
															  fieldsStr,
															  enumeratorsCount));*/
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

	void CodeGenerator::_writeSourcePrelude()
	{
		mpSourceOutputStream->WriteString("#include \"metadata.h\"\n");
	}
}