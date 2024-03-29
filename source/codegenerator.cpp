#include "../include/codegenerator.h"
#include "../include/common.h"
#include <set>


namespace TDEngine2
{
	static const std::string mEnumTraitTemplateSpecializationHeaderPattern = R"(
template <>
struct EnumTrait<{0}>
{
	using UnderlyingType = typename std::underlying_type<{0}>::type;

	static constexpr bool     isOpaque = {1};
	static constexpr uint32_t elementsCount = {2};

	static constexpr std::array<EnumFieldInfo<{0}>, {2}> fields
	{
		{3}
	};

	static constexpr const char* ToString({0} value)
	{
{4}
		return "";
	}

	static {0} FromString(const std::string& value)
	{
		for (auto&& currElement : fields)
		{
			if (currElement.name == value)
			{
				return currElement.value;
			}
		}

		return (fields.size() > 0) ? fields[0].value : static_cast<{0}>(0);
	}
};

#ifdef META_IMPLEMENTATION
constexpr std::array<EnumFieldInfo<{0}>, {2}> EnumTrait<{0}>::fields;
#endif
)";


	const std::string CodeGenerator::mClassTraitTemplateSpecializationHeaderPattern = R"(
template <>
struct ClassTrait<{0}>
{
	static const std::string name;
	static constexpr TypeID  typeID = TYPEID({0});

	static const bool isInterface = {1};
	static const bool isAbstract = {2};

	static const std::array<TypeID, {3}> interfaces;
	static const std::array<TypeID, {4}> parentClasses;
};

#if defined(META_IMPLEMENTATION)

const std::string ClassTrait<{0}>::name = "{0}";

const std::array<TypeID, {3}> ClassTrait<{0}>::interfaces {5};

const std::array<TypeID, {4}> ClassTrait<{0}>::parentClasses {6};

template<> struct Type<TYPEID({0})> { using Value = {0}; }; /// {0}

#endif


)";

	const std::string CodeGenerator::mEnumeratorFieldPattern = "EnumFieldInfo<{0}> { {1}, \"{2}\" }";

	const std::string EnumValueToStringConverterPattern = "\t\tif ({0} == value) { return \"{1}\"; }\n";

	const std::string CodeGenerator::mTrueConstant = "true";
	const std::string CodeGenerator::mFalseConstant = "false";

	CodeGenerator::CodeGenerator()
	{
	}
	
	CodeGenerator::~CodeGenerator()
	{
		if (mpHeaderOutputStream)
		{
			mpHeaderOutputStream->WriteString("\n}");
			mpHeaderOutputStream->Close();
		}
	}

	bool CodeGenerator::Init(const TOutputStreamFactoryFunctor& outputStreamsFactory, const std::string& outputFilename, const E_EMIT_FLAGS& flags,
							const std::vector<std::regex>& excludeTypenamePatterns, bool isTaggedOnlyModeEnabled)
	{
		if (!outputStreamsFactory)
		{
			return false;
		}

		mOutputStreamFactoryFunctor = outputStreamsFactory;
		mOutputFilenamesName = outputFilename;

		mpHeaderOutputStream = outputStreamsFactory(outputFilename);

		mEmitFlags = flags;

		mTypenamesToHidePatterns = excludeTypenamePatterns;

		mIsTaggedOnlyMode = isTaggedOnlyModeEnabled;

		if (!mpHeaderOutputStream)
		{
			return false;
		}

		if (!mpHeaderOutputStream->Open())
		{
			return false;
		}

		return true;
	}

	bool CodeGenerator::Generate(TSymbolTablesArray&& symbolTablesPerFile)
	{
		EnumsMetaExtractor enumsExtractor(mEmitFlags);
		ClassMetaExtractor classesExtractor(mEmitFlags);

		// \note Collect data from symbol tables
		for (auto&& pCurrSymbolTable : symbolTablesPerFile)
		{
			if (!pCurrSymbolTable)
			{
				continue;
			}

			pCurrSymbolTable->Visit(enumsExtractor);
			pCurrSymbolTable->Visit(classesExtractor);
		}

		std::string dependenciesInclusionsStr = WriteInclusions(enumsExtractor);
		dependenciesInclusionsStr.append(WriteInclusions(classesExtractor));

		_writeHeaderPrelude(dependenciesInclusionsStr);

		const bool enumsWriteResult   = WriteMetaData("\n/*\n\tEnum's meta\n*/\n\n", enumsExtractor);
		const bool classesWriteResult = WriteMetaData("\n/*\n\tClasses's meta\n*/\n\n", classesExtractor);

		return enumsWriteResult && classesWriteResult;
	}

	void CodeGenerator::VisitBaseType(const TType& type)
	{

	}

	void CodeGenerator::VisitEnumType(const TEnumType& type)
	{
		if (type.mIsForwardDeclaration ||
			_shouldSkipGeneration(type.mId) || 
			(mIsTaggedOnlyMode && !type.mIsMarkedWithAttribute)) // \note skip forward declarations to prevent duplicates of traits of the same type
		{
			return;
		}

		std::string fullEnumName = "::" + StringUtils::ReplaceAll(type.mMangledId, "@", "::");

		size_t enumeratorsCount = type.mEnumerators.size();

		std::string fieldsStr = Wrench::StringUtils::GetEmptyStr();
		std::string fieldsToStringStr = Wrench::StringUtils::GetEmptyStr();

		auto&& enumerators = type.mEnumerators;

		for (size_t i = 0; i < enumeratorsCount; ++i)
		{
			auto&& currEnumerator = enumerators[i];

			fieldsStr
				.append(Wrench::StringUtils::Format(mEnumeratorFieldPattern, fullEnumName, fullEnumName + "::" + currEnumerator, currEnumerator))
				.append(i + 1 < enumeratorsCount ? "," : "").append("\n\t\t");

			fieldsToStringStr
				.append(Wrench::StringUtils::Format(EnumValueToStringConverterPattern, fullEnumName + "::" + currEnumerator, currEnumerator));
		}

		std::string sectionIdentifier = type.mSectionId.empty() ? "ALL" : type.mSectionId; /// \todo replace DEFAULT with configurable constant
		std::transform(sectionIdentifier.begin(), sectionIdentifier.end(), sectionIdentifier.begin(), ::toupper);	/// \note Convert to upper case

		mpHeaderOutputStream->WriteString(Wrench::StringUtils::Format("\n#ifdef META_EXPORT_{0}_SECTION\n", sectionIdentifier));

		mpHeaderOutputStream->WriteString(Wrench::StringUtils::Format(mEnumTraitTemplateSpecializationHeaderPattern,
															  fullEnumName,
															  (type.mIsStronglyTyped ? mTrueConstant : mFalseConstant),
															  enumeratorsCount, fieldsStr, fieldsToStringStr));

		mpHeaderOutputStream->WriteString("\n#endif\n");
	}

	void CodeGenerator::VisitNamespaceType(const TNamespaceType& type)
	{

	}

	void CodeGenerator::VisitClassType(const TClassType& type)
	{
		if (type.mIsForwardDeclaration ||
			type.mIsTemplate ||
			_shouldSkipGeneration(type.mId) ||
			(mIsTaggedOnlyMode && !type.mIsMarkedWithAttribute)) // \note skip forward declarations to prevent duplicates of traits of the same type
		{
			return;
		}

		std::string fullClassIdentifier = "::" + StringUtils::ReplaceAll(type.mMangledId, "@", "::");

		auto&& parentClasses = _getParentClasses(type);

		mpHeaderOutputStream->WriteString(Wrench::StringUtils::Format(mClassTraitTemplateSpecializationHeaderPattern,
															  fullClassIdentifier,
															  mFalseConstant,
															  mFalseConstant,
															  0, 
															  parentClasses.size(), 
															  "{}",
															  _vectorToString(parentClasses)));
	}

	void CodeGenerator::_writeHeaderPrelude(const std::string& inclusionsPart)
	{
		mpHeaderOutputStream->WriteString("#pragma once\n\n");
		mpHeaderOutputStream->WriteString(Wrench::StringUtils::Format(GeneratedHeaderPrelude, inclusionsPart));
	}

	std::vector<std::string> CodeGenerator::_getParentClasses(const TClassType& classType)
	{
		std::vector<std::string> parentClasses;

		for (auto&& currBaseClassInfo : classType.mBaseClasses)
		{
			parentClasses.push_back(Wrench::StringUtils::Format("TYPEID({0})", currBaseClassInfo.mFullName));
		}

		return parentClasses;
	}

	std::string CodeGenerator::_vectorToString(const std::vector<std::string>& types)
	{
		std::string outputString{ "{ " };
		
		if (!types.empty())
		{
			outputString.push_back('\n');
		}

		for (auto&& currTypeId : types)
		{
			outputString.append(currTypeId).append(", ");
		}

		if (!types.empty())
		{
			outputString.push_back('\n');
		}

		outputString.append(" }");

		return outputString;
	}

	bool CodeGenerator::_shouldSkipGeneration(const std::string& id) const
	{
		for (auto&& currPattern : mTypenamesToHidePatterns)
		{
			if (std::regex_match(id, currPattern))
			{
				return true;
			}
		}

		return false;
	}
}