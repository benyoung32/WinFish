#ifndef __LEVELSETTINGS_H__
#define __LEVELSETTINGS_H__

#include <map>
#include "LevelData.h"
enum SlotTypes
	{
		SLOT_GUPPY,
		SLOT_BREEDER,
		SLOT_FOODLVL,
		SLOT_FOODLIMIT,
		SLOT_OSCAR,
		SLOT_POTION,
		SLOT_STARCATCHER,
		SLOT_GRUBBER,
		SLOT_GEKKO,
		SLOT_ULTRA,
		SLOT_WEAPON,
		SLOT_EGG,
		SLOT_END
	};

namespace Sexy
{	

	struct LevelConfig
	{
		int mSlotNumber[12]; // SLOT_END
		int mSlotPrices[12];
		int mAlienExpect;
		int mAlienTimer;

		LevelConfig();
	};

	class LevelSettingsManager
	{
	public:
		static LevelSettingsManager* Get();

		bool						HasOverride(const LevelData& theLevel) const;
		const LevelConfig*			GetOverride(const LevelData& theLevel) const;
		void						SetOverride(const LevelData& theLevel, const LevelConfig& theConfig);
		void						ClearOverrides();

		// Populates theConfig with standard vanilla level data
		static void					GetDefaultConfig(const LevelData& theLevel, bool theRelaxMode, LevelConfig& theConfig);

	private:
		LevelSettingsManager();
		std::map<LevelData, LevelConfig> mOverrides; // TODO: remove this map, it is deprecated 
	};
}

#endif // __LEVELSETTINGS_H__
