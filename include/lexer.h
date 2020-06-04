#pragma once


#include <string>
#include <fstream>
#include <unordered_map>
#include "tokens.h"
#include "common.h"


namespace TDEngine2
{
	class IInputStream
	{
		public:
			IInputStream() TDE2_NOEXCEPT = default;
			virtual ~IInputStream() TDE2_NOEXCEPT = default;

			virtual bool Open() TDE2_NOEXCEPT = 0;
			virtual bool Close() TDE2_NOEXCEPT = 0;

			virtual std::string ReadLine() TDE2_NOEXCEPT = 0;
	};


	class FileInputStream : public IInputStream
	{
		public:
			FileInputStream(const std::string& filename) TDE2_NOEXCEPT;
			virtual ~FileInputStream() TDE2_NOEXCEPT;

			bool Open() TDE2_NOEXCEPT override;
			bool Close() TDE2_NOEXCEPT override;

			std::string ReadLine() TDE2_NOEXCEPT override;
		protected:
			FileInputStream() TDE2_NOEXCEPT = default;
		protected:
			static const std::string mEmptyStr;

			std::string              mFilename;
			std::ifstream            mFileStream;
	};


	class Lexer
	{
		public:
			using TKeywordsMap = std::unordered_map<std::string, E_TOKEN_TYPE>;
		public:
			Lexer(IInputStream& streamSource);
			~Lexer() = default;

			const TToken& GetCurrToken() const;
			const TToken& GetNextToken();
			const TToken& PeekToken(uint32_t offset = 1);
		private:
			TToken _scanToken();

			char _getCurrChar() const;
			char _getNextChar();
			char _peekNextChar(uint32_t offset);
		private:
			IInputStream*             mpStream;

			std::string               mCurrProcessedText;

			std::vector<TToken>       mTokensQueue;

			static const TKeywordsMap mReservedTokens;
	};
}