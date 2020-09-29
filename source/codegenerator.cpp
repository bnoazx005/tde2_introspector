#include "../include/codegenerator.h"
#include "../include/common.h"
#include <set>


namespace TDEngine2
{
	const std::string CodeGenerator::mEnumTraitTemplateSpecializationHeaderPattern = R"(
template <>
struct EnumTrait<{0}>
{
	using UnderlyingType = typename std::underlying_type<{0}>::type;

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

	static {0} FromString(const std::string& value)
	{
		static auto&& elements = GetFields();

		for (auto&& currElement : elements)
		{
			if (currElement.name == value)
			{
				return currElement.value;
			}
		}

		return (elements.size() > 0) ? elements[0].value : static_cast<{0}>(0);
	}
};
)";

	const std::string CodeGenerator::mClassTraitTemplateSpecializationHeaderPattern = R"(
template <>
struct ClassTrait<{0}>
{
	static const std::string name = "{0}";
	static constexpr TypeID  typeID = TYPEID({0});

	static const bool isInterface = {1};
	static const bool isAbstract = {2};

	static const std::array<TypeID, {3}> interfaces
	{
		{5}
	};

	static const std::array<TypeID, {4}> parentClasses
	{
		{6}
	};
};

template struct Type<TYPEID({0})> { using Value = {0}; }; /// {0}
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

		mpHeaderOutputStream = outputStreamsFactory(outputFilename);

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

	void CodeGenerator::VisitClassType(const TClassType& type)
	{
		std::string fullClassIdentifier = StringUtils::ReplaceAll(type.mMangledId, "@", "::");

		auto&& parentClasses = _getParentClasses(type);

		mpHeaderOutputStream->WriteString(StringUtils::Format(mClassTraitTemplateSpecializationHeaderPattern, 
															  fullClassIdentifier,
															  mFalseConstant,
															  mFalseConstant,
															  0, 
															  parentClasses.size(), 
															  "",
															  _vectorToString(parentClasses)));
	}

	void CodeGenerator::_writeHeaderPrelude()
	{
		mpHeaderOutputStream->WriteString("#pragma once\n\n");
		mpHeaderOutputStream->WriteString(GeneratedHeaderPrelude);
	}

	std::vector<std::string> CodeGenerator::_getParentClasses(const TClassType& classType)
	{
		std::vector<std::string> parentClasses;

		for (auto&& currBaseClassInfo : classType.mBaseClasses)
		{
			parentClasses.push_back(StringUtils::Format("TYPEID({0})", currBaseClassInfo.mFullName));
		}

		return parentClasses;
	}

	std::string CodeGenerator::_vectorToString(const std::vector<std::string>& types)
	{
		std::string outputString{};

		for (auto&& currTypeId : types)
		{
			outputString.append(currTypeId).append(", ");
		}

		return outputString;
	}
}