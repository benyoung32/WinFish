#ifndef __APGAMECONTROLLER_H__
#define __APGAMECONTROLLER_H__

#include <cstdint>
#include <deque>
#include <string>
#include <vector>
#include <set>

#include "APDefinitions.h"
#include "APBridge.h"
#include "LevelData.h"

namespace Sexy
{
	class WinFishApp;
	class Board;
	class UserProfile;

	// ---------------------------------------------------------------------------
	// Location Mapping Tables
	// ---------------------------------------------------------------------------
	static const APLocation gEggPieceLocations[4][5][3] = {
		// Tank 1
		{
			{ APLocation::TANK_1_1_EGG_PIECE_1, APLocation::TANK_1_1_EGG_PIECE_2, APLocation::TANK_1_1_EGG_PIECE_3 },
			{ APLocation::TANK_1_2_EGG_PIECE_1, APLocation::TANK_1_2_EGG_PIECE_2, APLocation::TANK_1_2_EGG_PIECE_3 },
			{ APLocation::TANK_1_3_EGG_PIECE_1, APLocation::TANK_1_3_EGG_PIECE_2, APLocation::TANK_1_3_EGG_PIECE_3 },
			{ APLocation::TANK_1_4_EGG_PIECE_1, APLocation::TANK_1_4_EGG_PIECE_2, APLocation::TANK_1_4_EGG_PIECE_3 },
			{ APLocation::TANK_1_5_EGG_PIECE_1, APLocation::TANK_1_5_EGG_PIECE_2, APLocation::TANK_1_5_EGG_PIECE_3 },
		},
		// Tank 2
		{
			{ APLocation::TANK_2_1_EGG_PIECE_1, APLocation::TANK_2_1_EGG_PIECE_2, APLocation::TANK_2_1_EGG_PIECE_3 },
			{ APLocation::TANK_2_2_EGG_PIECE_1, APLocation::TANK_2_2_EGG_PIECE_2, APLocation::TANK_2_2_EGG_PIECE_3 },
			{ APLocation::TANK_2_3_EGG_PIECE_1, APLocation::TANK_2_3_EGG_PIECE_2, APLocation::TANK_2_3_EGG_PIECE_3 },
			{ APLocation::TANK_2_4_EGG_PIECE_1, APLocation::TANK_2_4_EGG_PIECE_2, APLocation::TANK_2_4_EGG_PIECE_3 },
			{ APLocation::TANK_2_5_EGG_PIECE_1, APLocation::TANK_2_5_EGG_PIECE_2, APLocation::TANK_2_5_EGG_PIECE_3 },
		},
		// Tank 3
		{
			{ APLocation::TANK_3_1_EGG_PIECE_1, APLocation::TANK_3_1_EGG_PIECE_2, APLocation::TANK_3_1_EGG_PIECE_3 },
			{ APLocation::TANK_3_2_EGG_PIECE_1, APLocation::TANK_3_2_EGG_PIECE_2, APLocation::TANK_3_2_EGG_PIECE_3 },
			{ APLocation::TANK_3_3_EGG_PIECE_1, APLocation::TANK_3_3_EGG_PIECE_2, APLocation::TANK_3_3_EGG_PIECE_3 },
			{ APLocation::TANK_3_4_EGG_PIECE_1, APLocation::TANK_3_4_EGG_PIECE_2, APLocation::TANK_3_4_EGG_PIECE_3 },
			{ APLocation::TANK_3_5_EGG_PIECE_1, APLocation::TANK_3_5_EGG_PIECE_2, APLocation::TANK_3_5_EGG_PIECE_3 },
		},
		// Tank 4
		{
			{ APLocation::TANK_4_1_EGG_PIECE_1, APLocation::TANK_4_1_EGG_PIECE_2, APLocation::TANK_4_1_EGG_PIECE_3 },
			{ APLocation::TANK_4_2_EGG_PIECE_1, APLocation::TANK_4_2_EGG_PIECE_2, APLocation::TANK_4_2_EGG_PIECE_3 },
			{ APLocation::TANK_4_3_EGG_PIECE_1, APLocation::TANK_4_3_EGG_PIECE_2, APLocation::TANK_4_3_EGG_PIECE_3 },
			{ APLocation::TANK_4_4_EGG_PIECE_1, APLocation::TANK_4_4_EGG_PIECE_2, APLocation::TANK_4_4_EGG_PIECE_3 },
			{ APLocation::TANK_4_5_EGG_PIECE_1, APLocation::TANK_4_5_EGG_PIECE_2, APLocation::TANK_4_5_EGG_PIECE_3 },
		}
	};

	static const APLocation gTankCompleteLocations[4] = {
		APLocation::TANK_1_COMPLETE,
		APLocation::TANK_2_COMPLETE,
		APLocation::TANK_3_COMPLETE,
		APLocation::TANK_4_COMPLETE
	};

	static const APLocation gTimeTrialLocations[4] = {
		APLocation::TIME_TRIAL_1_COMPLETE,
		APLocation::TIME_TRIAL_2_COMPLETE,
		APLocation::TIME_TRIAL_3_COMPLETE,
		APLocation::TIME_TRIAL_4_COMPLETE
	};

	static const APLocation gChallengeLocations[4] = {
		APLocation::CHALLENGE_1_COMPLETE,
		APLocation::CHALLENGE_2_COMPLETE,
		APLocation::CHALLENGE_3_COMPLETE,
		APLocation::CHALLENGE_4_COMPLETE
	};

	static const APLocation gBonusPetLocations[4] = {
		APLocation::BONUS_PET_1,
		APLocation::BONUS_PET_2,
		APLocation::BONUS_PET_3,
		APLocation::BONUS_PET_4
	};

	class APGameController
	{
	public:
		APGameController(WinFishApp* theApp, APBridge* theBridge);
		void						Update();

		// Connection & Session Lifecycle
		void						OnConnected(const std::string& theSlot, const std::string& theSeed);
		void						OnDisconnected();
		void						ResetSession();

		// Item Ingestion & Queries
		void						OnItemReceived(const APReceivedItem& theItem);
		int							GetFoodQuality() const { return mFoodQuality; }
		int							GetFoodQuantity() const { return mFoodQuantity; }
		int							GetPetSlots() const;
		int							GetReceivedPetSlots() const { return mPetSlots; }
		int							GetWeaponLevel() const { return mWeaponLevel; }
		int							GetStartingMoneyBonus() const { return mStartingMoneyBonus; }
		bool						IsTankUnlocked(const LevelData& theLevel) const;

		// Location Check Dispatch
		void						ReportLocation(int64_t theLocationId);
		void						ReportEggPiece(const LevelData& theLevel, int thePiece);
		void						ReportTankComplete(const LevelData& theLevel);
		void						ReportTimeTrialComplete(const LevelData& theLevel);
		void						ReportChallengeComplete(const LevelData& theLevel);
		void						ReportCyrax();
		void						ReportBonusPet(int theBonusPetIndex);
		void						ReportAlienDefeated(int theAlienType);

		// Goal & Victory Conditions
		bool						CheckVictoryCondition();

		// Board / Gameplay Session Hooks
		void						ApplyUpgradeAvailabilities(Board* theBoard);
		void						ApplyLevelStart(Board* theBoard);
		void						UpdateBoard(Board* theBoard);

		// Notification Queue (displayed via Board::ShowText)
		void						QueueNotification(const std::string& theMessage);

		// Persistence / Profile Synchronization (Option C)
		void						LoadFromProfile(UserProfile* theProfile);
		void						SaveToProfile(UserProfile* theProfile);

	public:
		WinFishApp*					mApp;
		APBridge*					mBridge;

		// Cumulative Unlocks
		int							mFoodQuality;
		int							mFoodQuantity;
		int							mPetSlots;
		int							mWeaponLevel;
		int							mStartingMoneyBonus;
		bool						mUnlockedTanks[5];
		bool						mUnlockedPets[24];

		// Traps & Queues
		std::deque<APItem>			mPendingTraps;
		std::deque<std::string>		mNotificationQueue;
		int							mNotificationCooldown; // ticks between ShowText popups

		// History of received items
		std::vector<APReceivedItem>	mReceivedHistory;
	};
}

#endif // __APGAMECONTROLLER_H__
