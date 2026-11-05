#ifndef __LEVELSETTINGS_H__
#define __LEVELSETTINGS_H__

#include <map>
#include "Board.h"

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

		bool						HasOverride(int theTank, int theLevel) const;
		const LevelConfig*			GetOverride(int theTank, int theLevel) const;
		void						SetOverride(int theTank, int theLevel, const LevelConfig& theConfig);
		void						ClearOverrides();

		// Populates theConfig with standard vanilla level data
		static void					GetDefaultConfig(int theTank, int theLevel, bool theRelaxMode, LevelConfig& theConfig);

	private:
		LevelSettingsManager();
		std::map<std::pair<int, int>, LevelConfig> mOverrides;
	};
}

#endif // __LEVELSETTINGS_H__

