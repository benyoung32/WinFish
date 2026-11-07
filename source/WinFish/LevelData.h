#ifndef __LEVELDATA_H__
#define __LEVELDATA_H__

#include <stdexcept>

namespace Sexy
{
	// Represents a valid adventure tank and level (Tanks 1-4 with Levels 1-5, or Tank 5 Level 1).
	struct LevelData
	{
		int mTank;
		int mLevel;

		LevelData(int tank, int level = 1) :
			mTank(tank),
			mLevel(level)
		{
			if (!IsValid(tank, level))
				throw std::invalid_argument("Invalid level: tank must be 1-4 (levels 1-5) or tank 5 (level 1)");
		}

		static bool IsValid(int tank, int level = 1)
		{
			if (tank >= 1 && tank <= 4 && level >= 1 && level <= 5)
				return true;
			if (tank == 5 && level == 1)
				return true;
			return false;
		}

		bool operator<(const LevelData& other) const
		{
			if (mTank != other.mTank)
				return mTank < other.mTank;
			return mLevel < other.mLevel;
		}

		bool operator==(const LevelData& other) const
		{
			return mTank == other.mTank && mLevel == other.mLevel;
		}
	};
}

#endif // __LEVELDATA_H__

