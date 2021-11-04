#include "../include/parser.h"
#include "../include/lexer.h"
#include "../include/tokens.h"
#include "../include/symtable.h"
#include "../include/common.h"
#define STR_UTILS_IMPLEMENTATION
#include "../deps/Wrench/source/stringUtils.hpp"
#include <cassert>


namespace TDEngine2
{
	std::string TParserError::ToString() const
	{
		std::string result = "";
		
		result
			.append("(")
			.append(std::to_string(std::get<1>(mPos)))
			.append(":")
			.append(std::to_string(std::get<0>(mPos)))
			.append(") ")
			.append(ParserErrorToString(mCode))
			.append(". ");

		switch (mCode)
		{
			case TParserError::E_PARSER_ERROR_CODE::UNEXPECTED_SYMBOL:
			{
				result
					.append("Expected \'")
					.append(TokenTypeToString(mData.mUnexpectedTokenErrData.mExpectedToken))
					.append("\', but found \'")
					.append(TokenTypeToString(mData.mUnexpectedTokenErrData.mActualToken))
					.append("\'");
			}
				break;
		}

		return result;
	}

	std::string ParserErrorToString(TParserError::E_PARSER_ERROR_CODE errorCode)
	{
		switch (errorCode)
		{
			case TParserError::E_PARSER_ERROR_CODE::UNEXPECTED_SYMBOL:
				return "Unexpected symbol found";
		}

		return "";
	}


	Parser::Parser(Lexer& lexer, SymTable& symTable, const TOnErrorCallback& onErrorCallback):
		mpLexer(&lexer), mpSymTable(&symTable), mOnErrorCallback(onErrorCallback)
	{
	}

	void Parser::Parse()
	{
		_parseDeclarationSequence();
	}


	inline Parser::E_DECL_TYPE operator& (Parser::E_DECL_TYPE left, Parser::E_DECL_TYPE right)
	{
		return static_cast<Parser::E_DECL_TYPE>(static_cast<uint32_t>(left) & static_cast<uint32_t>(right));
	}

	inline Parser::E_DECL_TYPE operator| (Parser::E_DECL_TYPE left, Parser::E_DECL_TYPE right)
	{
		return static_cast<Parser::E_DECL_TYPE>(static_cast<uint32_t>(left) | static_cast<uint32_t>(right));
	}


	bool Parser::_parseDeclarationSequence(bool isInvokedFromTemplateDecl, E_DECL_TYPE allowedDeclTypes)
	{
		const TToken* pCurrToken = nullptr;

		bool result = true;

		while ((pCurrToken = &mpLexer->GetCurrToken())->mType != E_TOKEN_TYPE::TT_EOF)
		{
			switch (pCurrToken->mType)
			{
				case E_TOKEN_TYPE::TT_NAMESPACE:
					if ((E_DECL_TYPE::NAMESPACE & allowedDeclTypes) != E_DECL_TYPE::NAMESPACE)
					{
						return false;
					}

					result = _parseNamespaceDefinition();
					break;
				case E_TOKEN_TYPE::TT_TEMPLATE:
					if ((E_DECL_TYPE::TEMPLATE & allowedDeclTypes) != E_DECL_TYPE::TEMPLATE)
					{
						return false;
					}

					result = _parseTemplateDeclaration();
					break;
				case E_TOKEN_TYPE::TT_ENUM:
					if ((E_DECL_TYPE::ENUM_TYPE & allowedDeclTypes) != E_DECL_TYPE::ENUM_TYPE)
					{
						return false;
					}

					result = _parseEnumDeclaration(E_ACCESS_SPECIFIER_TYPE::PUBLIC);
					break;
				case E_TOKEN_TYPE::TT_CLASS:
				case E_TOKEN_TYPE::TT_STRUCT:
					result = _parseClassDeclaration(isInvokedFromTemplateDecl);
					break;
				case E_TOKEN_TYPE::TT_OPEN_BRACE:
					result = _parseCompoundStatement(); // \fixme temprorary solution to skip any { .. } compound block in listings
					break;
				case E_TOKEN_TYPE::TT_CLOSE_BRACE: // there are some tokens that we can skip in this method
					return true;
				default:
					mpLexer->GetNextToken(); // just skip unknown tokens
					break;
			}

			if (isInvokedFromTemplateDecl)
			{
				return result;
			}
		}

		return result;
	}

	bool Parser::_parseNamespaceDefinition()
	{
		if (mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_NAMESPACE)
		{
			return false;
		}

		mpLexer->GetNextToken();

		return _parseAnonymusNamespaceDefinition() || _parseNamedNamespaceDefinition();
	}

	bool Parser::_parseNamedNamespaceDefinition()
	{
		if (!_expect(E_TOKEN_TYPE::TT_IDENTIFIER, mpLexer->GetCurrToken()))
		{
			return false;
		}

		std::string namespaceId = dynamic_cast<const TIdentifierToken&>(mpLexer->GetCurrToken()).mId;
		if (!mpSymTable->CreateScope(namespaceId))
		{
			assert(false);
			return false;
		}

		mpLexer->GetNextToken();

		if (!_expect(E_TOKEN_TYPE::TT_OPEN_BRACE, mpLexer->GetCurrToken()))
		{
			return false;
		}

		if (auto pNamespaceScope = mpSymTable->LookUpNamedScope(namespaceId))
		{
			if (auto pNamespaceType = std::make_unique<TNamespaceType>())
			{
				pNamespaceType->mId = namespaceId;

				pNamespaceScope->mpType = std::move(pNamespaceType);
			}			
		}

		mpLexer->GetNextToken();

		// \note parse the body of the namespace
		if (!_parseDeclarationSequence())
		{
			return false;
		}

		if (!_expect(E_TOKEN_TYPE::TT_CLOSE_BRACE, mpLexer->GetCurrToken()))
		{
			return false;
		}

		mpSymTable->ExitScope();

		mpLexer->GetNextToken();

		return true;
	}

	bool Parser::_parseAnonymusNamespaceDefinition()
	{
		if (mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_OPEN_BRACE)
		{
			return false;
		}

		auto&& currToken = mpLexer->GetNextToken(); // eat {

		// \note parse the body of the namespace
		if (!_parseDeclarationSequence())
		{
			return false;
		}

		if (!_expect(E_TOKEN_TYPE::TT_CLOSE_BRACE, mpLexer->GetCurrToken()))
		{
			return false;
		}

		mpLexer->GetNextToken();

		return true;
	}

	bool Parser::_parseBlockDeclaration()
	{
		return _parseEnumDeclaration(E_ACCESS_SPECIFIER_TYPE::PUBLIC);
	}

	bool Parser::_parseTemplateDeclaration()
	{
		auto checkAndEat = [this](E_TOKEN_TYPE type)
		{
			if (!_expect(type, mpLexer->GetCurrToken()))
			{
				return false;
			}

			mpLexer->GetNextToken();
			return true;
		};

		if (!checkAndEat(E_TOKEN_TYPE::TT_TEMPLATE) ||
			!checkAndEat(E_TOKEN_TYPE::TT_LESS))
		{
			return false;
		}

		const TToken* pCurrToken = nullptr;

		while ((E_TOKEN_TYPE::TT_GREAT != (pCurrToken = &mpLexer->GetCurrToken())->mType) && (E_TOKEN_TYPE::TT_EOF != pCurrToken->mType))
		{
			mpLexer->GetNextToken();
		}

		if (!checkAndEat(E_TOKEN_TYPE::TT_GREAT))
		{
			return false;
		}

		return _parseDeclarationSequence(true, E_DECL_TYPE::TEMPLATE | E_DECL_TYPE::TYPE);
	}

	bool Parser::_parseEnumDeclaration(E_ACCESS_SPECIFIER_TYPE accessModifier)
	{
		if (E_TOKEN_TYPE::TT_ENUM != mpLexer->GetCurrToken().mType)
		{
			return false;
		}

		mpLexer->GetNextToken();

		bool isStronglyTypedEnum = false;

		// \note enum class case
		if (mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_CLASS || mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_STRUCT)
		{
			isStronglyTypedEnum = true;
			mpLexer->GetNextToken();
		}

		if (!_expect(E_TOKEN_TYPE::TT_IDENTIFIER, mpLexer->GetCurrToken()))
		{
			return false;
		}

		std::string enumName = dynamic_cast<const TIdentifierToken&>(mpLexer->GetCurrToken()).mId;

		bool result = mpSymTable->CreateScope(enumName);
		assert(result);

		mpLexer->GetNextToken();

		std::string underlyingTypeStr;

		// \note parse enumeration's underlying type
		if (mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_COLON)
		{
			mpLexer->GetNextToken();

			// \note Parse base type of the enumeration
			/*if (!_parseTypeSpecifiers())
			{
				return false;
			}*/
			// \todo replace it
			while (mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_SEMICOLON && mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_OPEN_BRACE)
			{
				mpLexer->GetNextToken();
			}
		}

		if (auto pEnumScopeEntity = mpSymTable->LookUpNamedScope(enumName))
		{			
			auto pEnumTypeDesc = std::make_unique<TEnumType>();

			pEnumTypeDesc->mId = enumName;
			pEnumTypeDesc->mMangledId = mpSymTable->GetMangledNameForNamedScope(enumName);
			pEnumTypeDesc->mIsStronglyTyped = isStronglyTypedEnum;
			pEnumTypeDesc->mIsForwardDeclaration = (mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_SEMICOLON);
			pEnumTypeDesc->mpOwner = mpSymTable;
			pEnumTypeDesc->mpParentType = mpSymTable->GetCurrScopeType();
			pEnumTypeDesc->mAccessModifier = accessModifier;

			if (mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_OPEN_BRACE)
			{
				mpLexer->GetNextToken(); // eat {

				_parseEnumBody(pEnumTypeDesc.get());

				if (!_expect(E_TOKEN_TYPE::TT_CLOSE_BRACE, mpLexer->GetCurrToken()))
				{
					return false;
				}

				mpLexer->GetNextToken(); // eat }
			}

			pEnumScopeEntity->mpType = std::move(pEnumTypeDesc);
		}

		if (!_expect(E_TOKEN_TYPE::TT_SEMICOLON, mpLexer->GetCurrToken()))
		{
			return false;
		}

		mpSymTable->ExitScope();

		mpLexer->GetNextToken();

		return true;
	}

	bool Parser::_parseEnumBody(TEnumType* pEnumType)
	{
		while (_parseEnumeratorDefinition(pEnumType) && mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_COMMA) 
		{
			mpLexer->GetNextToken(); // eat ',' token
		}

		return true;
	}

	bool Parser::_parseEnumeratorDefinition(TEnumType* pEnumType)
	{
		if (E_TOKEN_TYPE::TT_IDENTIFIER != mpLexer->GetCurrToken().mType)
		{
			return false;
		}

		if (pEnumType)
		{
			pEnumType->mEnumerators.emplace_back(dynamic_cast<const TIdentifierToken&>(mpLexer->GetCurrToken()).mId);
		}

		mpLexer->GetNextToken();

		if (mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_ASSIGN_OP) /// \note try to parse value of the enumerator
		{
			mpLexer->GetNextToken();

			// \todo for now we just skip this part
			while (mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_COMMA &&
				   mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_CLOSE_BRACE &&
				   mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_EOF)
			{
				if (mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_CLOSE_BRACE) // \note the close brase will be eaten in caller method
				{
					mpLexer->GetNextToken();
				}
			}

			if (mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_EOF)
			{
				mOnErrorCallback({}); // \todo add correct error handling here
				return false;
			}
		}

		return true;
	}

	std::unique_ptr<TType> Parser::_parseTypeSpecifiers()
	{
		// \todo replace it
		while (mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_SEMICOLON && mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_OPEN_BRACE)
		{
			mpLexer->GetNextToken();
		}

		return nullptr;
	}

	bool Parser::_parseClassDeclaration(bool isTemplateDeclaration)
	{
		const bool isStruct = (E_TOKEN_TYPE::TT_STRUCT == mpLexer->GetCurrToken().mType);

		if (E_TOKEN_TYPE::TT_CLASS != mpLexer->GetCurrToken().mType && !isStruct)
		{
			return false;
		}

		mpLexer->GetNextToken();

		std::string className = _parseClassIdentifier();

		if (className.empty())
		{
			mOnErrorCallback({});
			return false;
		}

		if (!mpSymTable->CreateScope(className))
		{
			// This case assumes that we've already met forward declaration earlier
			if (!mpSymTable->EnterScope(className))
			{
				assert(false);
			}
		}

		DEFER([this]
		{
			_expect(E_TOKEN_TYPE::TT_SEMICOLON, mpLexer->GetCurrToken());
			mpSymTable->ExitScope();

			mpLexer->GetNextToken();
		});

		if (!_parseClassHeader(className, isStruct, isTemplateDeclaration) ||
			!_parseClassBody(className))
		{
			return false;
		}

		return true;
	}

	bool Parser::_parseClassHeader(const std::string& className, bool isStruct, bool isTemplate)
	{
		auto pClassScopeEntity = mpSymTable->LookUpNamedScope(className);
		if (!pClassScopeEntity)
		{
			return false;
		}

		auto pClassTypeDesc = std::make_unique<TClassType>();

		pClassTypeDesc->mId         = className;
		pClassTypeDesc->mMangledId  = mpSymTable->GetMangledNameForNamedScope(className);
		pClassTypeDesc->mpOwner     = mpSymTable;
		pClassTypeDesc->mIsStruct   = isStruct;
		pClassTypeDesc->mIsTemplate = isTemplate;

		// \note 'final' specifier parsing
		pClassTypeDesc->mIsFinal = (E_TOKEN_TYPE::TT_FINAL == mpLexer->GetCurrToken().mType);
		if (pClassTypeDesc->mIsFinal)
		{
			mpLexer->GetNextToken();
		}

		DEFER([pClassScopeEntity, &pClassTypeDesc] 
		{
			pClassScopeEntity->mpType = std::move(pClassTypeDesc);
		});

		if (E_TOKEN_TYPE::TT_COLON != mpLexer->GetCurrToken().mType)
		{
			return true;
		}

		mpLexer->GetNextToken();

		/// \note Parse information about base classes

		auto _parseSingleBaseSpecifierClause = [this, &pClassTypeDesc]
		{
			TClassType::TBaseClassInfo info;

			bool changed = false;

			// \note Parse access specifiers
			for (uint8_t i = 0; i < 2; ++i)
			{
				changed = true;

				switch (mpLexer->GetCurrToken().mType)
				{
					case E_TOKEN_TYPE::TT_PUBLIC:
						info.mAccessSpecifier = E_ACCESS_SPECIFIER_TYPE::PUBLIC;
						break;
					case E_TOKEN_TYPE::TT_PROTECTED:
						info.mAccessSpecifier = E_ACCESS_SPECIFIER_TYPE::PROTECTED;
						break;
					case E_TOKEN_TYPE::TT_PRIVATE:
						info.mAccessSpecifier = E_ACCESS_SPECIFIER_TYPE::PRIVATE;
						break;
					case E_TOKEN_TYPE::TT_VIRTUAL:
						info.mIsVirtualInherited = true;
						break;
					default:
						changed = false;
						break;
				}

				if (changed)
				{
					mpLexer->GetNextToken();
				}
			}

			// \note Parse base class's identifier 
			// \todo Refactor this to correctly parse names, now it just eats tokens and assumes that it's correct name
			std::string& baseClassId = info.mFullName;
			baseClassId.append(_parseClassIdentifier());

			pClassTypeDesc->mBaseClasses.emplace_back(info);
			
			return true;
		};

		do
		{
			if (!pClassTypeDesc->mBaseClasses.empty() && (E_TOKEN_TYPE::TT_COMMA == mpLexer->GetCurrToken().mType))
			{
				mpLexer->GetNextToken(); // eat ,
			}

			_parseSingleBaseSpecifierClause();
		} while (E_TOKEN_TYPE::TT_COMMA == mpLexer->GetCurrToken().mType);

		if (!_expect(E_TOKEN_TYPE::TT_OPEN_BRACE, mpLexer->GetCurrToken()))
		{
			return false;
		}

		return true;
	}

	bool Parser::_parseClassBody(const std::string& className)
	{
		auto pClassScopeEntity = mpSymTable->LookUpNamedScope(className);
		if (!pClassScopeEntity)
		{
			return false;
		}

		if (!pClassScopeEntity->mpType)
		{
			pClassScopeEntity->mpType = std::make_unique<TClassType>();
		}

		TClassType* pClassTypeDesc =  dynamic_cast<TClassType*>(pClassScopeEntity->mpType.get());
		
		// \note Try to parse body, it starts from {
		if (E_TOKEN_TYPE::TT_OPEN_BRACE != mpLexer->GetCurrToken().mType)
		{
			if (pClassTypeDesc)
			{
				pClassTypeDesc->mIsForwardDeclaration = true;
			}

			return true;
		}

		mpLexer->GetNextToken();

		const TToken* pCurrToken = &mpLexer->GetCurrToken();

		E_ACCESS_SPECIFIER_TYPE accessModifier = (pClassTypeDesc && pClassTypeDesc->mIsStruct) ? E_ACCESS_SPECIFIER_TYPE::PUBLIC : E_ACCESS_SPECIFIER_TYPE::PRIVATE;
		uint32_t depth = 1; // \fixme Temporary fix with indentation counter to implement correct recognition without actual parsing

		while (depth > 0)
		{
			pCurrToken = &mpLexer->GetCurrToken();
			depth = (pCurrToken->mType == E_TOKEN_TYPE::TT_OPEN_BRACE) ? (depth + 1) : ((pCurrToken->mType == E_TOKEN_TYPE::TT_CLOSE_BRACE) ? (depth - 1) : depth);

			if (E_TOKEN_TYPE::TT_PUBLIC == pCurrToken->mType ||
				E_TOKEN_TYPE::TT_PROTECTED == pCurrToken->mType ||
				E_TOKEN_TYPE::TT_PRIVATE == pCurrToken->mType)
			{
				switch (pCurrToken->mType)
				{
					case E_TOKEN_TYPE::TT_PUBLIC:
						accessModifier = E_ACCESS_SPECIFIER_TYPE::PUBLIC;
						break;
					case E_TOKEN_TYPE::TT_PROTECTED:
						accessModifier = E_ACCESS_SPECIFIER_TYPE::PROTECTED;
						break;
					case E_TOKEN_TYPE::TT_PRIVATE:
						accessModifier = E_ACCESS_SPECIFIER_TYPE::PRIVATE;
						break;
					}

				mpLexer->GetNextToken();

				if (!_expect(E_TOKEN_TYPE::TT_COLON, mpLexer->GetCurrToken()))
				{
					return false;
				}

				mpLexer->GetNextToken();
				continue;
			}

			if (depth <= 0)
			{
				continue;
			}

			if (E_TOKEN_TYPE::TT_ENUM == pCurrToken->mType)
			{
				_parseEnumDeclaration(accessModifier);
				continue;
			}

			if (E_TOKEN_TYPE::TT_STRUCT == pCurrToken->mType || E_TOKEN_TYPE::TT_CLASS == pCurrToken->mType)
			{
				_parseClassDeclaration();
				continue;
			}

			pCurrToken = &mpLexer->GetNextToken();
		}

		if (!_expect(E_TOKEN_TYPE::TT_CLOSE_BRACE, mpLexer->GetCurrToken()))
		{
			return false;
		}

		mpLexer->GetNextToken();

		return true;
	}

	bool Parser::_parseClassMemberSpecification(const std::string& className, E_ACCESS_SPECIFIER_TYPE accessModifier)
	{
		const TToken* pCurrToken = &mpLexer->GetCurrToken();

		E_ACCESS_SPECIFIER_TYPE nextAccessModifier;

		if (E_TOKEN_TYPE::TT_PUBLIC == pCurrToken->mType ||
			E_TOKEN_TYPE::TT_PROTECTED == pCurrToken->mType ||
			E_TOKEN_TYPE::TT_PRIVATE == pCurrToken->mType)
		{
			switch (pCurrToken->mType)
			{
				case E_TOKEN_TYPE::TT_PUBLIC:
					nextAccessModifier = E_ACCESS_SPECIFIER_TYPE::PUBLIC;
					break;
				case E_TOKEN_TYPE::TT_PROTECTED:
					nextAccessModifier = E_ACCESS_SPECIFIER_TYPE::PROTECTED;
					break;
				case E_TOKEN_TYPE::TT_PRIVATE:
					nextAccessModifier = E_ACCESS_SPECIFIER_TYPE::PRIVATE;
					break;
			}

			mpLexer->GetNextToken();
		}

		return _parseClassMemberDeclaration(className, accessModifier) && _parseClassMemberSpecification(className, accessModifier);
	}

	bool Parser::_parseClassMemberDeclaration(const std::string& className, E_ACCESS_SPECIFIER_TYPE accessModifier)
	{
		const TToken* pCurrToken = &mpLexer->GetCurrToken();


		return true;
	}

	bool Parser::_parseCompoundStatement()
	{
		if (!_expect(E_TOKEN_TYPE::TT_OPEN_BRACE, mpLexer->GetCurrToken().mType))
		{
			return false;
		}

		mpLexer->GetNextToken();

		const TToken* pCurrToken = nullptr;

		while (E_TOKEN_TYPE::TT_CLOSE_BRACE != (pCurrToken = &mpLexer->GetCurrToken())->mType && (E_TOKEN_TYPE::TT_EOF != pCurrToken->mType))
		{
			if (E_TOKEN_TYPE::TT_OPEN_BRACE == mpLexer->GetCurrToken().mType)
			{
				if (!_parseCompoundStatement())
				{
					return false;
				}
			}

			mpLexer->GetNextToken();
		}

		mpLexer->GetNextToken(); // eat } token

		return true;
	}

	std::string Parser::_parseClassIdentifier()
	{
		auto&& id = _parseSimpleTemplateIdentifier();

		if (!id.empty())
		{
			return id;
		}

		const TToken& currToken = mpLexer->GetCurrToken();

		if (E_TOKEN_TYPE::TT_IDENTIFIER == currToken.mType) // a simple identifier
		{
			DEFER([this] { mpLexer->GetNextToken(); });

			return dynamic_cast<const TIdentifierToken&>(currToken).mId;
		}
		
		return Wrench::StringUtils::GetEmptyStr();
	}

	std::string Parser::_parseSimpleTemplateIdentifier()
	{
		const TToken& currToken = mpLexer->GetCurrToken();

		// could be simple identifier or simple template one
		if (E_TOKEN_TYPE::TT_IDENTIFIER == currToken.mType)
		{
			const TIdentifierToken& idToken = dynamic_cast<const TIdentifierToken&>(currToken);

			std::string templateIdentifier = idToken.mId;

			if (E_TOKEN_TYPE::TT_LESS == mpLexer->PeekToken().mType) // a template identifier
			{
				mpLexer->GetNextToken(); // eat <

				templateIdentifier.push_back('<');

				// \note \todo Parse template's arguments list
				while (mpLexer->GetCurrToken().mType != E_TOKEN_TYPE::TT_GREAT)
				{
					mpLexer->GetNextToken();
				}

				if (!_expect(E_TOKEN_TYPE::TT_GREAT, mpLexer->GetCurrToken().mType))
				{
					return Wrench::StringUtils::GetEmptyStr();
				}

				templateIdentifier.push_back('>');

				return templateIdentifier;
			}

			return Wrench::StringUtils::GetEmptyStr();
		}

		return Wrench::StringUtils::GetEmptyStr();
	}

	bool Parser::_eatUnknownTokens()
	{
		while (mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_UNKNOWN)
		{
			mpLexer->GetNextToken();
		}

		return mpLexer->GetCurrToken().mType == E_TOKEN_TYPE::TT_EOF;
	}

	bool Parser::_expect(E_TOKEN_TYPE expectedType, const TToken& token)
	{
		if (expectedType == token.mType)
		{
			return true;
		}
		
		if (mOnErrorCallback)
		{
			TParserError error;
			error.mCode = TParserError::E_PARSER_ERROR_CODE::UNEXPECTED_SYMBOL;
			error.mPos  = token.mPos;
			error.mData.mUnexpectedTokenErrData = { token.mType, expectedType };

			mOnErrorCallback(error);
		}

		return false;
	}
}