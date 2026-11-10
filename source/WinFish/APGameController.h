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
		void						ReportCyrax();
		void						ReportBonusPet(int theBonusPetIndex);

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
