#include <vector>
#include <string>
#include <lexer.h>

#pragma once


class MockInputStream : public TDEngine2::IInputStream
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

			std::string line = std::move(mStreamData.front()).append("\n");
			mStreamData.erase(mStreamData.begin());

			return std::move(line);
		}
	private:
		std::vector<std::string> mStreamData;
};