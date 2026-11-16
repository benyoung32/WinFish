#include "APGameController.h"
#include "APDefinitions.h"
#include "WinFishApp.h"
#include "Board.h"
#include "Alien.h"
#include "Fish.h"
#include "Breeder.h"
#include "Oscar.h"
#include "Penta.h"
#include "Ultra.h"
#include "Grubber.h"
#include "Gekko.h"
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
	mNotificationCooldown(0),
	mTrapCooldown(0),
	mFishDeathsSinceLastDeathLink(0),
	mSuppressDeathLink(false)
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
			SetLastCheck(aSent.mItemName, "Sent to " + aSent.mRecipientSlot + "!");
		}

		std::vector<APDeathLink> aDeathLinks;
		mBridge->PopReceivedDeathLinks(aDeathLinks);
		for (const APDeathLink& aDL : aDeathLinks)
		{
			mPendingDeathLinks.push_back(aDL);
		}
	}
}

void APGameController::OnConnected(const std::string& theSlot, const std::string& theSeed)
{
	if (mBridge != NULL)
		mUpgradeAvailability = mBridge->GetSlotData().mUpgradeAvailability;

	if (mApp != NULL && mApp->mCurrentProfile != NULL)
	{
		UserProfile* aProf = mApp->mCurrentProfile;
		if (mBridge != NULL)
		{
			const SlotData& aSlotData = mBridge->GetSlotData();
			aProf->mAPSlotData = aSlotData.mRawJson;

			bool aHasAnyTank = false;
			for (int i = 0; i < 4; i++)
			{
				if (aProf->mAPUnlockedTanks[i])
				{
					aHasAnyTank = true;
					break;
				}
			}

			if (!aHasAnyTank)
			{
				int aStartTank = aSlotData.GetStartingTank();
				if (aStartTank < 1 || aStartTank > 4)
					aStartTank = 1;

				aProf->mAPUnlockedTanks[aStartTank - 1] = true;
				aProf->mTank = aStartTank;
				aProf->mAPTankLevels[aStartTank - 1] = 1;
				mUnlockedTanks[aStartTank] = true;
			}
			else if (aProf->mTank < 1 || aProf->mTank > 4 || !aProf->mAPUnlockedTanks[aProf->mTank - 1])
			{
				for (int i = 0; i < 4; i++)
				{
					if (aProf->mAPUnlockedTanks[i])
					{
						aProf->mTank = i + 1;
						break;
					}
				}
			}
		}

		aProf->m0xb4 = GetPetSlots();
		aProf->mAPUpgradeAvailability = mUpgradeAvailability;
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
	ClearLastCheck();
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

	case APItem::BONUS_SHELLS:
		if (mApp != NULL && mApp->mCurrentProfile != NULL)
		{
			mApp->mCurrentProfile->mBonusLevelTickets++;
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
	SetLastCheck(anItemName, "Received for yourself!");

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
	if (thePiece == 3)
	{
		SetLastCheck("Archipelago Item!", "Location check sent to multiworld!");
	}
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
	ResetDeathLinkAmnesty();

	// set trap cooldown to 60-180 seconds (1-3 minutes)
	mTrapCooldown = (rand() % 60 + 120) * 60;

	if (theBoard == NULL || (theBoard->mApp != NULL && theBoard->mApp->IsBonusLevel()))
		return;
	

	// Apply starting money bonus
	int aTotalStartingMoney = mStartingMoneyBonus;
	if (mBridge != NULL)
		aTotalStartingMoney += mBridge->GetSlotData().mStartingMoneyBonus;

	if (aTotalStartingMoney > 0)
	{
		theBoard->mMoney += aTotalStartingMoney;
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
	// TODO: check that its during adventure, time trial, or challenge mode
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

	// Service Death Links during active gameplay (not during bonus rounds)
	if (!mPendingDeathLinks.empty() && !theBoard->mIsBonusRound)
	{
		APDeathLink aDeathLink = mPendingDeathLinks.front();
		int anEffect = mBridge != NULL ? mBridge->GetSlotData().mDeathLinkEffect : 0;

		if (anEffect == DEATH_LINK_EFFECT_INSTANT_KILL)
		{
			// If fish are present in the tank, execute instant kill
			if (theBoard->HasAnyFish())
			{
				mPendingDeathLinks.pop_front();

				mSuppressDeathLink = true;

				if (theBoard->mFishList != NULL)
				{
					std::vector<Fish*> aFishCopy = *theBoard->mFishList;
					for (Fish* aFish : aFishCopy)
					{
						if (aFish != NULL)
							aFish->Die(true);
					}
				}
				if (theBoard->mBreederList != NULL)
				{
					std::vector<Breeder*> aBreederCopy = *theBoard->mBreederList;
					for (Breeder* aBreeder : aBreederCopy)
					{
						if (aBreeder != NULL)
							aBreeder->Die(true);
					}
				}
				if (theBoard->mOscarList != NULL)
				{
					std::vector<Oscar*> anOscarCopy = *theBoard->mOscarList;
					for (Oscar* anOscar : anOscarCopy)
					{
						if (anOscar != NULL)
							anOscar->Die(true);
					}
				}
				if (theBoard->mPentaList != NULL)
				{
					std::vector<Penta*> aPentaCopy = *theBoard->mPentaList;
					for (Penta* aPenta : aPentaCopy)
					{
						if (aPenta != NULL)
							aPenta->Die(true);
					}
				}
				if (theBoard->mUltraList != NULL)
				{
					std::vector<Ultra*> anUltraCopy = *theBoard->mUltraList;
					for (Ultra* anUltra : anUltraCopy)
					{
						if (anUltra != NULL)
							anUltra->Die(true);
					}
				}
				if (theBoard->mGrubberList != NULL)
				{
					std::vector<Grubber*> aGrubberCopy = *theBoard->mGrubberList;
					for (Grubber* aGrubber : aGrubberCopy)
					{
						if (aGrubber != NULL)
							aGrubber->Die(true);
					}
				}
				if (theBoard->mGekkoList != NULL)
				{
					std::vector<Gekko*> aGekkoCopy = *theBoard->mGekkoList;
					for (Gekko* aGekko : aGekkoCopy)
					{
						if (aGekko != NULL)
							aGekko->Die(true);
					}
				}

				mSuppressDeathLink = false;

				std::string aMsg = aDeathLink.mCause.empty() ?
					("DEATH LINK: " + aDeathLink.mSource + " died!") :
					("DEATH LINK: " + aDeathLink.mCause);
				theBoard->ShowText(aMsg, true, -1);
			}
		}
		else if (anEffect == DEATH_LINK_EFFECT_RANDOM_TRAP)
		{
			mPendingDeathLinks.pop_front();

			APItem aRandomTrap = (rand() % 2 == 0) ? APItem::TRAP_ALIEN_ATTACK : APItem::TRAP_FAMINE;
			mPendingTraps.push_back(aRandomTrap);

			std::string aMsg = "DEATH LINK from " + aDeathLink.mSource + ": Trap queued!";
			theBoard->ShowText(aMsg, true, -1);
		}
	}
}

void APGameController::OnFishDied(const std::string& theCause)
{
	if (mBridge == NULL || !mBridge->GetSlotData().mDeathLink)
		return;

	if (mSuppressDeathLink)
		return;

	if (mApp == NULL || mApp->mBoard == NULL || mApp->mBoard->mIsBonusRound || mApp->mGameMode == GAMEMODE_VIRTUAL_TANK)
		return;

	mFishDeathsSinceLastDeathLink++;
	int anAmnesty = mBridge->GetSlotData().mDeathLinkAmnesty;

	if (mFishDeathsSinceLastDeathLink > anAmnesty)
	{
		std::string aCauseStr = mBridge->GetSlotName() + "'s " + theCause;
		mBridge->SendDeathLink(aCauseStr);
		mFishDeathsSinceLastDeathLink = 0;

		if (mApp->mBoard != NULL)
		{
			mApp->mBoard->ShowText("DEATH LINK SENT!", true, -1);
		}
	}
}

void APGameController::ResetDeathLinkAmnesty()
{
	mFishDeathsSinceLastDeathLink = 0;
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

	if (mBridge != NULL && !theProfile->mAPSlotData.empty())
	{
		mBridge->GetSlotData().ParseJson(theProfile->mAPSlotData);
	}

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
