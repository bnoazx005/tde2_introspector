#include <vector>
#include <string>
#include <lexer.h>
#include <catch2/catch.hpp>


using namespace TDEngine2;


class MockInputStream : public IInputStream
{
	public:
		MockInputStream(const std::vector<std::string>& data) :
			mStreamData(data)
		{
		}

		bool Open() TDE2_NOEXCEPT
		{
			return true;
		}

		bool Close() TDE2_NOEXCEPT
		{
			return true;
		}

		std::string ReadLine() TDE2_NOEXCEPT
		{
			if (mStreamData.empty())
			{
				return "";
			}

			std::string line = std::move(mStreamData.front());
			mStreamData.erase(mStreamData.begin());

			return std::move(line);
		}
	private:
		std::vector<std::string> mStreamData;
};


TEST_CASE("Lexer tests")
{
	SECTION("TestGetNextToken_PassKeywords_ReturnsSequenceOfTokens")
	{
		std::unique_ptr<IInputStream> stream{ new MockInputStream { 
			{
				// lines here
				"namespace{}",
				"::"
			} } };

		Lexer lexer(*stream);

		const TToken* pCurrToken = nullptr;

		uint32_t currExpectedTokenIndex = 0;

		std::vector<E_TOKEN_TYPE> expectedTokens
		{

		};

		while ((pCurrToken = &lexer.GetNextToken())->mType != E_TOKEN_TYPE::TT_EOF)
		{
			REQUIRE(pCurrToken->mType == expectedTokens[currExpectedTokenIndex++]);
		}
	}
}