#include <catch2/catch.hpp>
#include "../include/lexer.h"


using namespace TDEngine2;


class MockInputStream : public IInputStream
{
	public:
		MockInputStream(const std::string& data) :
			mStreamData(data), mPos(0)
		{
		}

	private:
		std::string mStreamData;

		uint32_t    mPos;
};


TEST_CASE("Lexer tests")
{
	SECTION("")
	{

	}
}