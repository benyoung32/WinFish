#ifndef __SLOTDATA_H__
#define __SLOTDATA_H__

#include <string>
#include "APDefinitions.h"

namespace Sexy
{
	struct SlotData
	{
		bool			mCyraxRequired;
		bool 			mTimeTrialRequired;
		bool			mChallengeModeRequired;
		int				mStartingTank;             // 1, 2, 3, or 4
		bool			mAliensanity;              // defeat each alien for the first time is a check
		int				mUpgradeAvailability;      // 0 = instant, 1 = free in shop, 2 = shop buy
		int				mStartingMoneyBonus;       // starting bonus money if set in slot_data
		int				mStartPetSlots;            // starting pet slots from slot_data (default 0)
		std::string		mRawJson;                  // raw JSON content from slot_data.json

		SlotData();
		explicit SlotData(const std::string& theJsonOrPath);

		bool			LoadFromFile(const std::string& theFilePath);
		bool			ParseJson(const std::string& theJsonStr);
		int				GetStartingTank() const { return mStartingTank; }
	};

	typedef SlotData slotData;

	enum UpgradeAvailability
	{
		UPGRADE_AVAILABILITY_INSTANT = 0,
		UPGRADE_AVAILABILITY_FREE_IN_SHOP = 1,
		UPGRADE_AVAILABILITY_SHOP_BUY = 2
	};
}

#endif // __SLOTDATA_H__

