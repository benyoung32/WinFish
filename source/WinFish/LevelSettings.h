#ifndef __LEVELSETTINGS_H__
#define __LEVELSETTINGS_H__

#include <map>
#include "Board.h"
#include "LevelData.h"

namespace Sexy
{
	struct LevelConfig
	{
		int mSlotNumber[SLOT_END];
		int mSlotPrices[SLOT_END];
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
		std::map<LevelData, LevelConfig> mOverrides;
	};
}

#endif // __LEVELSETTINGS_H__
