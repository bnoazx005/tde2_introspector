#pragma once


#include "symtable.h"
#include "common.h"
#include <functional>
#include <set>


namespace TDEngine2
{
	class IOutputStream;
	template <typename T> class MetaExtractor;


	class CodeGenerator: public ITypeVisitor
	{
		public:
			using TOutputStreamFactoryFunctor = std::function<std::unique_ptr<IOutputStream>(const std::string)>;
		public:
			CodeGenerator();
			~CodeGenerator();

			bool Init(const TOutputStreamFactoryFunctor& outputStreamsFactory, const std::string& outputFilename);

			void VisitBaseType(const TType& type) override;
			void VisitEnumType(const TEnumType& type) override;
			void VisitNamespaceType(const TNamespaceType& type) override;
			void VisitClassType(const TClassType& type) override;

			template <typename T>
			bool WriteMetaData(const std::string& comment, const MetaExtractor<T>& metaExtractor)
			{
				mpHeaderOutputStream->WriteString(comment);

				auto&& entities = metaExtractor.GetTypesInfo();

				std::set<std::string> neededIncludes;

				for (auto currMetaEntity : entities)
				{
					neededIncludes.insert(StringUtils::Format("#include \"{0}\"\n", StringUtils::ReplaceAll(currMetaEntity->mpOwner->GetSourceFilename(), "\\", "/")));
				}

				for (const std::string& currIncludeFilename : neededIncludes)
				{
					mpHeaderOutputStream->WriteString(currIncludeFilename);
				}

				for (auto currMetaEntity : entities)
				{
					currMetaEntity->Visit(*this);
				}

				return true;
			}
		private:
			void _writeHeaderPrelude();

			static std::vector<std::string> _getParentClasses(const TClassType& classType);
			static std::string _vectorToString(const std::vector<std::string>& types);

		private:
			std::unique_ptr<IOutputStream> mpHeaderOutputStream;
			//std::unique_ptr<IOutputStream> mpSourceOutputStream;

			TOutputStreamFactoryFunctor    mOutputStreamFactoryFunctor;

			std::string                    mOutputFilenamesName;

			static const std::string       mEnumTraitTemplateSpecializationHeaderPattern;
			static const std::string       mEnumeratorFieldPattern;

			static const std::string       mClassTraitTemplateSpecializationHeaderPattern;

			static const std::string       mTrueConstant;
			static const std::string       mFalseConstant;
	};
}