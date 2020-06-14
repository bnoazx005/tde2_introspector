#pragma once


#include "symtable.h"
#include <functional>


namespace TDEngine2
{
	class IOutputStream;
	class EnumsMetaExtractor;


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

			bool WriteEnumsMetaData(const EnumsMetaExtractor& enumsMeta);
		private:
			void _writeHeaderPrelude();
			void _writeSourcePrelude();
		private:
			std::unique_ptr<IOutputStream> mpHeaderOutputStream;
			std::unique_ptr<IOutputStream> mpSourceOutputStream;

			TOutputStreamFactoryFunctor    mOutputStreamFactoryFunctor;

			std::string                    mOutputFilenamesName;

			static const std::string       mEnumTraitTemplateSpecializationHeaderPattern;
			static const std::string       mEnumTraitTemplateSpecializationSourcePattern;
			static const std::string       mEnumeratorFieldPattern;

			static const std::string       mTrueConstant;
			static const std::string       mFalseConstant;
	};
}