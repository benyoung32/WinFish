#ifndef __SLOTDATA_H__
#define __SLOTDATA_H__

#include <string>
#include "APDefinitions.h"

namespace Sexy
{
	struct SlotData
	{
		int				mGoal;                     // 0 = CYRAX, 1 = ALL_TANKS, 2 = ALL_PETS (matches APGoal)
		int				mStartingTank;             // 1, 2, 3, or 4
		bool			mAliensanity;              // defeat each alien for the first time is a check
		int				mUpgradeAvailability;      // 0 = instant, 1 = free in shop, 2 = shop buy
		int				mStartingMoneyBonus;       // starting bonus money if set in slot_data
		std::string		mRawJson;                  // raw JSON content from slot_data.json

		SlotData();
		explicit SlotData(const std::string& theJsonOrPath);

		bool			LoadFromFile(const std::string& theFilePath);
		bool			ParseJson(const std::string& theJsonStr);

		APGoal			GetGoal() const { return static_cast<APGoal>(mGoal); }
		bool			IsGoal(APGoal theGoal) const { return mGoal == static_cast<int>(theGoal); }
		int				GetStartingTank() const { return mStartingTank; }
	};

	typedef SlotData slotData;
}

#endif // __SLOTDATA_H__

