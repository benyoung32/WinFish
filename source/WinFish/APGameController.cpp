#include "APGameController.h"
#include "APDefinitions.h"
#include "WinFishApp.h"
#include "Board.h"
#include "Alien.h"
#include "Fish.h"
#include "Breeder.h"
#include "ProfileMgr.h"
#include "SlotData.h"
#include "MenuButtonWidget.h"
#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Debug.h>

using namespace Sexy;

extern int gFoodType;
extern int gFoodLimit;

APGameController::APGameController(WinFishApp* theApp, APBridge* theBridge) :
	mApp(theApp),
	mBridge(theBridge),
	mFoodQuality(0),
	mFoodQuantity(0),
	mPetSlots(0),
	mWeaponLevel(0),
	mStartingMoneyBonus(0),
	mUpgradeAvailability(0),
	mNotificationCooldown(0)
{
	for (int i = 0; i < 5; i++)
		mUnlockedTanks[i] = false;
	for (int i = 0; i < 24; i++)
		mUnlockedPets[i] = false;
}

void APGameController::Update()
{
	// Process non-board specific timer updates if needed
	if (mNotificationCooldown > 0)
		mNotificationCooldown--;

	if (mTrapCooldown > 0)
		mTrapCooldown--;

	if (mBridge != NULL)
	{
		std::vector<APSentItem> aSentItems;
		mBridge->PopSentItems(aSentItems);
		for (const APSentItem& aSent : aSentItems)
		{
			QueueNotification("Sent " + aSent.mItemName + " to " + aSent.mRecipientSlot + "!");
		}
	}
}

void APGameController::OnConnected(const std::string& theSlot, const std::string& theSeed)
{
	if (mBridge != NULL)
		mUpgradeAvailability = mBridge->GetSlotData().mUpgradeAvailability;

	if (mApp != NULL && mApp->mCurrentProfile != NULL)
	{
		mApp->mCurrentProfile->m0xb4 = GetPetSlots();
		mApp->mCurrentProfile->mAPUpgradeAvailability = mUpgradeAvailability;
		mApp->SaveCurrentUserData();
	}

	if (mApp != NULL && mApp->mBoard != NULL)
	{
		ApplyUpgradeAvailabilities(mApp->mBoard);
	}

	QueueNotification("Connected to Archipelago!");
}

void APGameController::OnDisconnected()
{
	OutputDebugStringA("APGameController: Disconnected\n");
}

void APGameController::ResetSession()
{
	mFoodQuality = 0;
	mFoodQuantity = 0;
	mPetSlots = 0;
	mWeaponLevel = 0;
	mStartingMoneyBonus = 0;
	mUpgradeAvailability = 0;
	for (int i = 0; i < 5; i++)
		mUnlockedTanks[i] = false;
	for (int i = 0; i < 24; i++)
		mUnlockedPets[i] = false;

	mPendingTraps.clear();
	mNotificationQueue.clear();
	mReceivedHistory.clear();
	mNotificationCooldown = 0;
}

bool APGameController::IsTankUnlocked(const LevelData& theLevel) const
{
	int aTank = theLevel.mTank;
	if (aTank >= 1 && aTank <= 4)
		return mUnlockedTanks[aTank];
	if (aTank == 5)
		return mUnlockedTanks[1] && mUnlockedTanks[2] && mUnlockedTanks[3] && mUnlockedTanks[4];
	return false;
}


int APGameController::GetPetSlots() const
{
	int aStart = 0;
	if (mBridge != NULL)
		aStart = mBridge->GetSlotData().mStartPetSlots;
	int aTotal = aStart + mPetSlots;
	return aTotal > 24 ? 24 : aTotal;
}

void APGameController::OnItemReceived(const APReceivedItem& theItem)
{
	mReceivedHistory.push_back(theItem);
	APItem anAPItem = static_cast<APItem>(theItem.mItem);

	switch (anAPItem)
	{
	case APItem::TANK_1_UNLOCK:
	case APItem::TANK_2_UNLOCK:
	case APItem::TANK_3_UNLOCK:
	case APItem::TANK_4_UNLOCK:
	{
		int aTank = static_cast<int>(anAPItem);
		if (aTank >= 1 && aTank <= 4)
			mUnlockedTanks[aTank] = true;
		break;
	}

	case APItem::FOOD_QUALITY:
		mFoodQuality++;
		if (mFoodQuality > 2)
			mFoodQuality = 2;
		if (mApp != NULL && mApp->mBoard != NULL)
		{
			mApp->mBoard->mMaxFoodQuality = mFoodQuality;
			int aUpgradeAvail = mUpgradeAvailability;
			if (aUpgradeAvail == UPGRADE_AVAILABILITY_INSTANT)
			{
				if (gFoodType < mFoodQuality)
					gFoodType = mFoodQuality;
			}
			else
			{
				mApp->mBoard->MenuButtonSetupNoVT(SLOT_FOODLVL, true);
			}
		}
		break;

	case APItem::FOOD_QUANTITY:
		mFoodQuantity++;
		if (mFoodQuantity > 8)
			mFoodQuantity = 8;
		if (mApp != NULL && mApp->mBoard != NULL)
		{
			mApp->mBoard->mMaxFoodLimit = 1 + mFoodQuantity;
			int aUpgradeAvail = mUpgradeAvailability;
			if (aUpgradeAvail == UPGRADE_AVAILABILITY_INSTANT)
			{
				if (gFoodLimit < (1 + mFoodQuantity))
					gFoodLimit = 1 + mFoodQuantity;
			}
			else
			{
				mApp->mBoard->MenuButtonSetupNoVT(SLOT_FOODLIMIT, true);
			}
		}
		break;

	case APItem::PET_SLOT:
		mPetSlots++;
		if (mApp != NULL && mApp->mCurrentProfile != NULL)
			mApp->mCurrentProfile->m0xb4 = GetPetSlots();
		break;

	// TODO: allow this check to be affected by settings (should weapon upgrades give 2 lvls each or just 1)
	case APItem::WEAPON_UPGRADE:
		mWeaponLevel++;
		if (mApp != NULL && mApp->mBoard != NULL)
		{
			int aLaser = 2 + mWeaponLevel;
			if (aLaser > 12)
				aLaser = 12;
			mApp->mBoard->mMaxWeaponUpgrade = aLaser;
			int aUpgradeAvail = mUpgradeAvailability;
			if (aUpgradeAvail == UPGRADE_AVAILABILITY_INSTANT || mApp->mBoard->mTank == 5)
			{
				mApp->mBoard->m0x3e4 = aLaser;
			}
			else
			{
				mApp->mBoard->MenuButtonSetupNoVT(SLOT_WEAPON, true);
			}
		}
		break;

	case APItem::STARTING_MONEY:
		mStartingMoneyBonus += 100;
		if (mApp != NULL && mApp->mBoard != NULL)
		{
			// If already in a tank, grant the money directly
			mApp->mBoard->mMoney += 100;
			mApp->mBoard->UpdateMoneyLabelText();
		}
		break;

	case APItem::TRAP_ALIEN_ATTACK:
	case APItem::TRAP_FAMINE:
		mPendingTraps.push_back(anAPItem);
		break;

	default:
		// Check for pet items
		if (theItem.mItem >= static_cast<int64_t>(APItem::STINKY) &&
			theItem.mItem <= static_cast<int64_t>(APItem::WALTER))
		{
			int aPetIdx = static_cast<int>(theItem.mItem - static_cast<int64_t>(APItem::STINKY));
			if (aPetIdx >= 0 && aPetIdx < 24)
			{
				mUnlockedPets[aPetIdx] = true;
				if (mApp != NULL && mApp->mCurrentProfile != NULL)
					mApp->mCurrentProfile->UnlockPet(aPetIdx, true);
			}
		}
		break;
	}

	std::string anItemName = APItemToString(anAPItem);
	QueueNotification(std::string("Received: ") + anItemName + "!");

	// Persist updated controller state to the profile
	if (mApp != NULL && mApp->mCurrentProfile != NULL)
	{
		SaveToProfile(mApp->mCurrentProfile);
		mApp->SaveCurrentUserData();
	}
}

void APGameController::ReportLocation(int64_t theLocationId)
{
	mApp->CheckArchipelagoLocation(static_cast<int>(theLocationId));
}

void APGameController::ReportEggPiece(const LevelData& theLevel, int thePiece)
{
	if (thePiece < 1 || thePiece > 3)
	{
		return;
	}

	APLocation aLoc = gEggPieceLocations[theLevel.mTank - 1][theLevel.mLevel - 1][thePiece - 1];
	ReportLocation(static_cast<int64_t>(aLoc));
}

void APGameController::ReportTankComplete(const LevelData& theLevel)
{
	int aTank = theLevel.mTank;
	if (aTank < 1 || aTank > 4)
		return;

	ReportLocation(static_cast<int64_t>(gTankCompleteLocations[aTank - 1]));
}

void APGameController::ReportTimeTrialComplete(const LevelData& theLevel)
{
	int aTank = theLevel.mTank;
	if (aTank < 1 || aTank > 4)
		return;

	ReportLocation(static_cast<int64_t>(gTimeTrialLocations[aTank - 1]));
	CheckVictoryCondition();
}

void APGameController::ReportChallengeComplete(const LevelData& theLevel)
{
	int aTank = theLevel.mTank;
	if (aTank < 1 || aTank > 4)
		return;

	ReportLocation(static_cast<int64_t>(gChallengeLocations[aTank - 1]));
	CheckVictoryCondition();
}

void APGameController::ReportCyrax()
{
	ReportLocation(static_cast<int64_t>(APLocation::CYRAX));
	CheckVictoryCondition();
}

void APGameController::ReportBonusPet(int theBonusPetIndex)
{
	if (theBonusPetIndex < 0 || theBonusPetIndex > 3)
		return;

	ReportLocation(static_cast<int64_t>(gBonusPetLocations[theBonusPetIndex]));
}

void APGameController::ReportAlienDefeated(int theAlienType)
{
	if (mBridge == NULL || !mBridge->GetSlotData().mAliensanity)
		return;

	APLocation aLoc = APLocation::NONE;
	switch (theAlienType)
	{
	case ALIEN_WEAK_SYLV:
	case ALIEN_STRONG_SYLV:
		aLoc = APLocation::DEFEAT_SYLVESTER;
		break;
	case ALIEN_BALROG:
		aLoc = APLocation::DEFEAT_BALROG;
		break;
	case ALIEN_GUS:
		aLoc = APLocation::DEFEAT_GUS;
		break;
	case ALIEN_DESTRUCTOR:
		aLoc = APLocation::DEFEAT_DESTRUCTOR;
		break;
	case ALIEN_PSYCHOSQUID:
		aLoc = APLocation::DEFEAT_PSYCHOSQUID;
		break;
	case ALIEN_ULYSEES:
		aLoc = APLocation::DEFEAT_ULYSSES;
		break;
	case ALIEN_BILATERUS:
		aLoc = APLocation::DEFEAT_BILATERUS;
		break;
	default:
		return;
	}

	mApp->CheckArchipelagoLocation(static_cast<int>(aLoc));
}

bool APGameController::CheckVictoryCondition()
{
	if (mBridge == NULL || mBridge->IsGoalSent())
		return false;

	bool aVictory = false;
	if (mBridge->GetSlotData().mCyraxRequired)
	{
		aVictory = 	mBridge->IsLocationChecked(AP_LOCATION_CYRAX);
	}

	if (mBridge->GetSlotData().mChallengeModeRequired) 
	{ 
		aVictory =  mBridge->IsLocationChecked(AP_LOCATION_CHALLENGE_1_COMPLETE) &&
					mBridge->IsLocationChecked(AP_LOCATION_CHALLENGE_2_COMPLETE) &&
					mBridge->IsLocationChecked(AP_LOCATION_CHALLENGE_3_COMPLETE) &&
					mBridge->IsLocationChecked(AP_LOCATION_CHALLENGE_4_COMPLETE);
	}
	
	if (mBridge->GetSlotData().mTimeTrialRequired)
	{
		aVictory =  mBridge->IsLocationChecked(AP_LOCATION_TIME_TRIAL_1_COMPLETE) &&
					mBridge->IsLocationChecked(AP_LOCATION_TIME_TRIAL_2_COMPLETE) &&
					mBridge->IsLocationChecked(AP_LOCATION_TIME_TRIAL_3_COMPLETE) &&
					mBridge->IsLocationChecked(AP_LOCATION_TIME_TRIAL_4_COMPLETE);
	}
	
	if (aVictory)
	{
		mBridge->SendGoal();
		QueueNotification("Archipelago: Goal Achieved!");
		return true;
	}

	return false;
}

void APGameController::ApplyUpgradeAvailabilities(Board* theBoard)
{
	if (theBoard == NULL)
		return;

	// Set max upgrade limits based on items received from Archipelago
	theBoard->mMaxFoodLimit = 1 + mFoodQuantity;
	theBoard->mMaxFoodQuality = mFoodQuality;
	theBoard->mMaxWeaponUpgrade = (2 + mWeaponLevel > 12) ? 12 : (2 + mWeaponLevel);

	// int aUpgradeAvailability = UPGRADE_AVAILABILITY_SHOP_BUY;
	int aUpgradeAvailability = mBridge->GetSlotData().mUpgradeAvailability;

	switch (aUpgradeAvailability)
	{
	case UPGRADE_AVAILABILITY_INSTANT:
		// Apply weapon upgrade level (base is 2, laser level = 2 + mWeaponLevel, max 12)
		if (mWeaponLevel > 0)
		{
			int aLaser = 2 + mWeaponLevel;
			if (aLaser > 12)
				aLaser = 12;
			theBoard->m0x3e4 = aLaser;
		}
		
		// Apply food quality & quantity upgrades if received
		if (mFoodQuality > 0 && gFoodType < mFoodQuality)
			gFoodType = mFoodQuality;

		if (mFoodQuantity > 0 && gFoodLimit < (1 + mFoodQuantity))
			gFoodLimit = 1 + mFoodQuantity;

		// Disable food and weapon slots from the shop
		for (int i = 0; i < SLOT_END; i++)
		{
			if (i == SLOT_WEAPON || i == SLOT_FOODLIMIT || i == SLOT_FOODLVL)
			{
				theBoard->mSlotNumber[i] = -1;
				theBoard->mSlotPrices[i] = 0;
				theBoard->m0x374[i] = -1;
				theBoard->mSlotUnlocked[i] = false;
			}
		}
		break;
		
	case UPGRADE_AVAILABILITY_FREE_IN_SHOP:
		// Boss tank (Tank 5) has no shop, grant weapon directly
		if (theBoard->mTank == 5 && mWeaponLevel > 0)
		{
			int aLaser = 2 + mWeaponLevel;
			if (aLaser > 12)
				aLaser = 12;
			theBoard->m0x3e4 = aLaser;
		}

		if (theBoard->mSlotNumber[SLOT_WEAPON] > -1)
			theBoard->mSlotPrices[SLOT_WEAPON] = 0;
		if (theBoard->mSlotNumber[SLOT_FOODLIMIT] > -1)
			theBoard->mSlotPrices[SLOT_FOODLIMIT] = 0;
		if (theBoard->mSlotNumber[SLOT_FOODLVL] > -1)
			theBoard->mSlotPrices[SLOT_FOODLVL] = 0;

		if (theBoard->mSlotUnlocked[SLOT_WEAPON])
			theBoard->MenuButtonSetupNoVT(SLOT_WEAPON, false);
		if (theBoard->mSlotUnlocked[SLOT_FOODLIMIT])
			theBoard->MenuButtonSetupNoVT(SLOT_FOODLIMIT, false);
		if (theBoard->mSlotUnlocked[SLOT_FOODLVL])
			theBoard->MenuButtonSetupNoVT(SLOT_FOODLVL, false);
		break;
		
	case UPGRADE_AVAILABILITY_SHOP_BUY:
		// Boss tank (Tank 5) has no shop, grant weapon directly
		if (theBoard->mTank == 5 && mWeaponLevel > 0)
		{
			int aLaser = 2 + mWeaponLevel;
			if (aLaser > 12)
				aLaser = 12;
			theBoard->m0x3e4 = aLaser;
		}
		// Normal prices from LevelConfig are kept
		if (theBoard->mSlotUnlocked[SLOT_WEAPON])
			theBoard->MenuButtonSetupNoVT(SLOT_WEAPON, false);
		if (theBoard->mSlotUnlocked[SLOT_FOODLIMIT])
			theBoard->MenuButtonSetupNoVT(SLOT_FOODLIMIT, false);
		if (theBoard->mSlotUnlocked[SLOT_FOODLVL])
			theBoard->MenuButtonSetupNoVT(SLOT_FOODLVL, false);
		break;
	}

	if (theBoard->mWidgetManager != nullptr)
		theBoard->mWidgetManager->MarkAllDirty();
}

void APGameController::ApplyLevelStart(Board* theBoard)
{
	// set trap cooldown to 60-180 seconds (1-3 minutes)
	mTrapCooldown = (rand() % 60 + 120) * 60;

	if (theBoard == NULL || (theBoard->mApp != NULL && theBoard->mApp->IsBonusLevel()))
		return;
	

	// Apply starting money bonus
	if (mStartingMoneyBonus > 0)
	{
		theBoard->mMoney += mStartingMoneyBonus;
		theBoard->UpdateMoneyLabelText();
	}
	
	ApplyUpgradeAvailabilities(theBoard);
}

void APGameController::UpdateBoard(Board* theBoard)
{
	if (theBoard == NULL)
		return;

	// Service notification queue: push notifications as long as the board's 3 notification slots have any room, waiting otherwise
	if (!mNotificationQueue.empty() && theBoard->HasNotificationRoom() && mNotificationCooldown == 0)
	{
		std::string aMsg = mNotificationQueue.front();
		mNotificationQueue.pop_front();

		// Show non-blocking green notification on board
		theBoard->ShowNotification(aMsg, false, -1);
		mNotificationCooldown += 30;
	}
	// TODO: traps should be dispensed according to some timer or cooldown
	// Service traps during active gameplay (not during bonus rounds)
	if (!mPendingTraps.empty() && !theBoard->mIsBonusRound && mTrapCooldown == 0)
	{
		APItem trap = mPendingTraps.front();
		mPendingTraps.pop_front();

		if (trap == APItem::TRAP_ALIEN_ATTACK)
		{
			// Force alien timer to zero or trigger alien arrival
			theBoard->mAlienTimer = 1;
			theBoard->ShowText("ALIEN ATTACK TRAP ACTIVATED!", true, -1);
		}
		else if (trap == APItem::TRAP_FAMINE)
		{
			// TODO: check that newHunger is the correct value. fish die at -500
			int newHunger = 50;
			theBoard->ShowText("FAMINE TRAP ACTIVATED!", true, -1);
			if (theBoard->mFishList != NULL)
			{
				for (size_t i = 0; i < theBoard->mFishList->size(); i++)
				{
					Fish* aFish = (*theBoard->mFishList)[i];
					if (aFish != NULL)
						aFish->mHunger = newHunger;
				}
			}
			if (theBoard->mBreederList != NULL)
			{
				for (size_t i = 0; i < theBoard->mBreederList->size(); i++)
				{
					Breeder* aBreeder = (*theBoard->mBreederList)[i];
					if (aBreeder != NULL)
						aBreeder->mHunger = newHunger;
				}
			}

		}
	}
}

void APGameController::QueueNotification(const std::string& theMessage)
{
	if (theMessage.empty())
		return;

	mNotificationQueue.push_back(theMessage);
}

void APGameController::LoadFromProfile(UserProfile* theProfile)
{
	if (theProfile == NULL)
		return;

	mFoodQuality = theProfile->mAPFoodQuality;
	mFoodQuantity = theProfile->mAPFoodQuantity;
	mPetSlots = theProfile->mAPPetSlots;
	theProfile->m0xb4 = GetPetSlots();
	mWeaponLevel = theProfile->mAPWeaponLevel;
	mStartingMoneyBonus = theProfile->mAPStartingMoneyBonus;
	mUpgradeAvailability = theProfile->mAPUpgradeAvailability;

	for (int i = 0; i < 4; i++)
		mUnlockedTanks[i + 1] = theProfile->mAPUnlockedTanks[i];

	for (int i = 0; i < 24; i++)
		mUnlockedPets[i] = theProfile->IsPetUnlocked(i);
}

void APGameController::SaveToProfile(UserProfile* theProfile)
{
	if (theProfile == NULL)
		return;

	theProfile->mAPFoodQuality = mFoodQuality;
	theProfile->mAPFoodQuantity = mFoodQuantity;
	theProfile->mAPPetSlots = mPetSlots;
	theProfile->m0xb4 = GetPetSlots();
	theProfile->mAPWeaponLevel = mWeaponLevel;
	theProfile->mAPStartingMoneyBonus = mStartingMoneyBonus;
	theProfile->mAPUpgradeAvailability = mUpgradeAvailability;

	for (int i = 0; i < 4; i++)
		theProfile->mAPUnlockedTanks[i] = mUnlockedTanks[i + 1];

	theProfile->mNumOfUnlockedPets = 0;
	for (int i = 0; i < 24; i++)
	{
		theProfile->mUnlockedPets[i] = mUnlockedPets[i];
		if (mUnlockedPets[i])
			theProfile->mNumOfUnlockedPets++;
	}
}
