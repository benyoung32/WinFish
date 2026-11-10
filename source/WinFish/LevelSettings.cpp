#include "LevelSettings.h"
#include "Alien.h"

using namespace Sexy;

static LevelSettingsManager* gLevelSettingsManager = NULL;

LevelConfig::LevelConfig()
{
	for (int i = 0; i < SLOT_END; i++)
	{
		mSlotNumber[i] = -1;
		mSlotPrices[i] = 0;
	}
	mAlienExpect = ALIEN_STRONG_SYLV;
	mAlienTimer = 3000;
}

LevelSettingsManager::LevelSettingsManager()
{
}

LevelSettingsManager* LevelSettingsManager::Get()
{
	if (gLevelSettingsManager == NULL)
		gLevelSettingsManager = new LevelSettingsManager();
	return gLevelSettingsManager;
}

bool LevelSettingsManager::HasOverride(const LevelData& theLevel) const
{
	return mOverrides.find(theLevel) != mOverrides.end();
}

const LevelConfig* LevelSettingsManager::GetOverride(const LevelData& theLevel) const
{
	auto aIt = mOverrides.find(theLevel);
	if (aIt != mOverrides.end())
		return &aIt->second;
	return NULL;
}

void LevelSettingsManager::SetOverride(const LevelData& theLevel, const LevelConfig& theConfig)
{
	mOverrides[theLevel] = theConfig;
}

void LevelSettingsManager::ClearOverrides()
{
	mOverrides.clear();
}

void LevelSettingsManager::GetDefaultConfig(const LevelData& theLevelData, bool theRelaxMode, LevelConfig& theConfig)
{
	theConfig = LevelConfig();
	int theTank = theLevelData.mTank;
	int theLevel = theLevelData.mLevel;

	if (!theRelaxMode)
	{
		if (theTank == 1)
		{
			theConfig.mSlotNumber[SLOT_GUPPY] = 0;
			theConfig.mSlotNumber[SLOT_FOODLVL] = 1;
			theConfig.mSlotNumber[SLOT_FOODLIMIT] = 2;
			theConfig.mSlotNumber[SLOT_OSCAR] = 3;
			theConfig.mSlotNumber[SLOT_WEAPON] = 5;
			theConfig.mSlotNumber[SLOT_EGG] = 6;

			theConfig.mSlotPrices[SLOT_GUPPY] = 100;
			theConfig.mSlotPrices[SLOT_FOODLVL] = 200;
			theConfig.mSlotPrices[SLOT_FOODLIMIT] = 300;
			theConfig.mSlotPrices[SLOT_OSCAR] = 1000;
			theConfig.mSlotPrices[SLOT_WEAPON] = 1000;

			switch (theLevel)
			{
			case 1:
				theConfig.mAlienExpect = ALIEN_NONE;
				theConfig.mSlotPrices[SLOT_EGG] = 150;
				theConfig.mSlotNumber[SLOT_FOODLVL] = -1;
				theConfig.mSlotNumber[SLOT_FOODLIMIT] = -1;
				theConfig.mSlotNumber[SLOT_OSCAR] = -1;
				theConfig.mSlotNumber[SLOT_WEAPON] = -1;
				break;
			case 2:
				theConfig.mAlienExpect = ALIEN_WEAK_SYLV;
				theConfig.mSlotPrices[SLOT_EGG] = 500;
				theConfig.mAlienTimer = 1750;
				theConfig.mSlotNumber[SLOT_OSCAR] = -1;
				theConfig.mSlotNumber[SLOT_WEAPON] = -1;
				break;
			case 3:
				theConfig.mAlienExpect = ALIEN_STRONG_SYLV;
				theConfig.mSlotPrices[SLOT_EGG] = 2000;
				break;
			case 4:
				theConfig.mAlienExpect = ALIEN_BALROG;
				theConfig.mSlotPrices[SLOT_EGG] = 3000;
				break;
			case 5:
				theConfig.mAlienExpect = ALIEN_BALROG;
				theConfig.mSlotPrices[SLOT_EGG] = 5000;
				break;
			}
		}
		else if (theTank == 2)
		{
			theConfig.mSlotNumber[SLOT_GUPPY] = 0;
			theConfig.mSlotNumber[SLOT_FOODLVL] = 1;
			theConfig.mSlotNumber[SLOT_FOODLIMIT] = 2;
			theConfig.mSlotNumber[SLOT_POTION] = 3;
			theConfig.mSlotNumber[SLOT_STARCATCHER] = 4;
			theConfig.mSlotNumber[SLOT_WEAPON] = 5;
			theConfig.mSlotNumber[SLOT_EGG] = 6;

			theConfig.mSlotPrices[SLOT_GUPPY] = 100;
			theConfig.mSlotPrices[SLOT_FOODLVL] = 200;
			theConfig.mSlotPrices[SLOT_FOODLIMIT] = 300;
			theConfig.mSlotPrices[SLOT_POTION] = 250;
			theConfig.mSlotPrices[SLOT_STARCATCHER] = 750;
			theConfig.mSlotPrices[SLOT_WEAPON] = 1000;

			switch (theLevel)
			{
			case 1:
				theConfig.mAlienExpect = ALIEN_STRONG_SYLV;
				theConfig.mSlotPrices[SLOT_EGG] = 750;
				theConfig.mSlotNumber[SLOT_STARCATCHER] = -1;
				theConfig.mSlotNumber[SLOT_WEAPON] = -1;
				break;
			case 2:
				theConfig.mAlienExpect = ALIEN_BALROG;
				theConfig.mSlotPrices[SLOT_EGG] = 3000;
				break;
			case 3:
				theConfig.mAlienExpect = ALIEN_GUS;
				theConfig.mSlotPrices[SLOT_EGG] = 5000;
				break;
			case 4:
				theConfig.mAlienExpect = ALIEN_DESTRUCTOR;
				theConfig.mSlotPrices[SLOT_EGG] = 7500;
				break;
			case 5:
				theConfig.mAlienExpect = ALIEN_GUS; // random Gus or Destructor in vanilla
				theConfig.mSlotPrices[SLOT_EGG] = 10000;
				break;
			}
		}
		else if (theTank == 3)
		{
			theConfig.mSlotNumber[SLOT_GUPPY] = 0;
			theConfig.mSlotNumber[SLOT_FOODLVL] = 1;
			theConfig.mSlotNumber[SLOT_FOODLIMIT] = 2;
			theConfig.mSlotNumber[SLOT_GRUBBER] = 3;
			theConfig.mSlotNumber[SLOT_GEKKO] = 4;
			theConfig.mSlotNumber[SLOT_WEAPON] = 5;
			theConfig.mSlotNumber[SLOT_EGG] = 6;

			theConfig.mSlotPrices[SLOT_GUPPY] = 100;
			theConfig.mSlotPrices[SLOT_FOODLVL] = 200;
			theConfig.mSlotPrices[SLOT_FOODLIMIT] = 300;
			theConfig.mSlotPrices[SLOT_GRUBBER] = 750;
			theConfig.mSlotPrices[SLOT_GEKKO] = 2000;
			theConfig.mSlotPrices[SLOT_WEAPON] = 2000;

			switch (theLevel)
			{
			case 1:
				theConfig.mAlienExpect = ALIEN_BALROG;
				theConfig.mSlotPrices[SLOT_EGG] = 1000;
				theConfig.mSlotNumber[SLOT_GEKKO] = -1;
				theConfig.mSlotNumber[SLOT_WEAPON] = -1;
				break;
			case 2:
				theConfig.mAlienExpect = ALIEN_GUS;
				theConfig.mSlotPrices[SLOT_EGG] = 5000;
				break;
			case 3:
				theConfig.mAlienExpect = ALIEN_PSYCHOSQUID;
				theConfig.mSlotPrices[SLOT_EGG] = 7500;
				break;
			case 4:
				theConfig.mAlienExpect = ALIEN_ULYSEES;
				theConfig.mSlotPrices[SLOT_EGG] = 10000;
				break;
			case 5:
				theConfig.mAlienExpect = ALIEN_ULYSEES;
				theConfig.mSlotPrices[SLOT_EGG] = 15000;
				break;
			}
		}
		else if (theTank == 4)
		{
			theConfig.mSlotNumber[SLOT_BREEDER] = 0;
			theConfig.mSlotNumber[SLOT_FOODLVL] = 1;
			theConfig.mSlotNumber[SLOT_FOODLIMIT] = 2;
			theConfig.mSlotNumber[SLOT_OSCAR] = 3;
			theConfig.mSlotNumber[SLOT_ULTRA] = 4;
			theConfig.mSlotNumber[SLOT_WEAPON] = 5;
			theConfig.mSlotNumber[SLOT_EGG] = 6;

			theConfig.mSlotPrices[SLOT_BREEDER] = 200;
			theConfig.mSlotPrices[SLOT_FOODLVL] = 200;
			theConfig.mSlotPrices[SLOT_FOODLIMIT] = 300;
			theConfig.mSlotPrices[SLOT_OSCAR] = 1000;
			theConfig.mSlotPrices[SLOT_ULTRA] = 10000;
			theConfig.mSlotPrices[SLOT_WEAPON] = 5000;

			switch (theLevel)
			{
			case 1:
				theConfig.mAlienExpect = ALIEN_BALROG;
				theConfig.mSlotPrices[SLOT_EGG] = 3000;
				theConfig.mSlotNumber[SLOT_ULTRA] = -1;
				theConfig.mSlotNumber[SLOT_WEAPON] = -1;
				break;
			case 2:
				theConfig.mAlienExpect = ALIEN_BILATERUS;
				theConfig.mSlotPrices[SLOT_EGG] = 25000;
				break;
			case 3:
				theConfig.mAlienExpect = ALIEN_GUS;
				theConfig.mSlotPrices[SLOT_EGG] = 50000;
				break;
			case 4:
				theConfig.mAlienExpect = ALIEN_BILATERUS;
				theConfig.mSlotPrices[SLOT_EGG] = 75000;
				break;
			case 5:
				theConfig.mAlienExpect = ALIEN_BILATERUS;
				theConfig.mSlotPrices[SLOT_EGG] = 99999;
				break;
			}
		}
		else if (theTank == 5)
		{
			theConfig.mSlotNumber[SLOT_EGG] = 6;
			theConfig.mSlotPrices[SLOT_EGG] = 0;
			theConfig.mAlienExpect = ALIEN_CYRAX;
		}
	}
	else
	{
		// Relax Mode
		theConfig.mSlotNumber[SLOT_GUPPY] = 0;
		theConfig.mSlotNumber[SLOT_FOODLVL] = 1;
		theConfig.mSlotNumber[SLOT_FOODLIMIT] = 2;
		theConfig.mSlotNumber[SLOT_WEAPON] = 5;
		theConfig.mSlotNumber[SLOT_EGG] = 6;

		theConfig.mSlotPrices[SLOT_GUPPY] = 100;
		theConfig.mSlotPrices[SLOT_FOODLVL] = 200;
		theConfig.mSlotPrices[SLOT_FOODLIMIT] = 300;
		theConfig.mSlotPrices[SLOT_OSCAR] = 1000;
		theConfig.mSlotPrices[SLOT_ULTRA] = 10000;
		theConfig.mSlotPrices[SLOT_STARCATCHER] = 750;
		theConfig.mSlotPrices[SLOT_GRUBBER] = 750;
		theConfig.mSlotPrices[SLOT_GEKKO] = 2000;
		theConfig.mSlotPrices[SLOT_WEAPON] = 1000;

		switch (theTank)
		{
		case 1:
			theConfig.mSlotPrices[SLOT_EGG] = 5000;
			theConfig.mSlotNumber[SLOT_OSCAR] = 3;
			theConfig.mSlotNumber[SLOT_GUPPY] = 0;
			break;
		case 2:
			theConfig.mSlotPrices[SLOT_EGG] = 5000;
			theConfig.mSlotNumber[SLOT_STARCATCHER] = 3;
			theConfig.mSlotNumber[SLOT_GUPPY] = 0;
			break;
		case 3:
			theConfig.mSlotPrices[SLOT_EGG] = 10000;
			theConfig.mSlotNumber[SLOT_GRUBBER] = 3;
			theConfig.mSlotNumber[SLOT_GEKKO] = 5;
			theConfig.mSlotNumber[SLOT_GUPPY] = 0;
			break;
		case 4:
			theConfig.mSlotPrices[SLOT_EGG] = 25000;
			theConfig.mSlotNumber[SLOT_ULTRA] = 5;
			theConfig.mSlotNumber[SLOT_GUPPY] = 0;
			break;
		}
	}
}

