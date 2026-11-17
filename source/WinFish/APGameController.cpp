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
#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Debug.h>
#include <algorithm>

using namespace Sexy;

extern int gFoodType;
extern int gFoodLimit;

APGameController::APGameController(WinFishApp* theApp, APBridge* theBridge) :
	mApp(theApp),
	mBridge(theBridge),
	mNotificationCooldown(0),
	mTrapCooldown(0),
	mFishDeathsCounter(0),
	mSuppressDeathLink(false)
{
}

UserProfile* APGameController::GetProfile() const
{
	return mApp->mCurrentProfile;
}

int APGameController::GetPetSlots() const
{
	UserProfile* aProfile = GetProfile();
	if (aProfile == NULL)
		return 0;

	int aStart = 0;
	if (mBridge != NULL)
		aStart = mBridge->GetSlotData().mStartPetSlots;
	int aTotal = aStart + aProfile->mAPPetSlots;
	return aTotal > 24 ? 24 : aTotal;
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
	UserProfile* aProf = GetProfile();
	if (aProf != NULL && mBridge != NULL)
	{
		const SlotData& aSlotData = mBridge->GetSlotData();
		aProf->mAPSlotData = aSlotData.mRawJson;
		aProf->mAPUpgradeAvailability = aSlotData.mUpgradeAvailability;

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

			aProf->mAPUnlockedTanks[aStartTank - 1] = true;
			aProf->mTank = aStartTank;
			aProf->mAPTankLevels[aStartTank - 1] = 1;
		}

		aProf->m0xb4 = GetPetSlots();
		mApp->SaveCurrentUserData();
	}

	if (mApp->mBoard != NULL)
	{
		ApplyUpgradeAvailabilities(mApp->mBoard);
	}

	QueueNotification("Connected to Archipelago!");
}

void APGameController::ResetSession()
{
	mPendingTraps.clear();
	mNotificationQueue.clear();
	mNotificationCooldown = 0;
	mTrapCooldown = 0;
	mFishDeathsCounter = 0;
	mSuppressDeathLink = false;
	ClearLastCheck();
}

void APGameController::OnItemReceived(const APReceivedItem& theItem)
{
	UserProfile* aProfile = GetProfile();
	if (aProfile == NULL)
		return;

	Board* aBoard = mApp->mBoard;
	APItem anAPItem = theItem.mItem;

	if (anAPItem >= APItem::TANK_1_UNLOCK && anAPItem <= APItem::TANK_4_UNLOCK)
	{
		int aTank = static_cast<int>(anAPItem);
		if (aTank >= 1 && aTank <= 4)
			aProfile->mAPUnlockedTanks[aTank - 1] = true;
	}

	switch (anAPItem)
	{
	case APItem::FOOD_QUALITY:
		aProfile->mAPFoodQuality = (std::min)(aProfile->mAPFoodQuality + 1, 2);
		ApplyUpgradeAvailabilities(aBoard);
		break;

	case APItem::FOOD_QUANTITY:
		aProfile->mAPFoodQuantity = (std::min)(aProfile->mAPFoodQuantity + 1, 8);
		ApplyUpgradeAvailabilities(aBoard);
		break;

	case APItem::PET_SLOT:
		aProfile->mAPPetSlots++;
		aProfile->m0xb4 = GetPetSlots();
		break;

	// TODO: allow this check to be affected by settings (should weapon upgrades give 2 lvls each or just 1)
	case APItem::WEAPON_UPGRADE:
		aProfile->mAPWeaponLevel++;
		ApplyUpgradeAvailabilities(aBoard);
		break;

	case APItem::STARTING_MONEY:
		aProfile->mAPStartingMoneyBonus += 100;
		if (aBoard != NULL)
		{
			// If already in a tank, grant the money directly
			aBoard->mMoney += 100;
			aBoard->UpdateMoneyLabelText();
		}
		break;

	case APItem::BONUS_SHELLS:
		aProfile->mBonusLevelTickets++;
		break;

	case APItem::TRAP_ALIEN_ATTACK:
	case APItem::TRAP_FAMINE:
		mPendingTraps.push_back(anAPItem);
		break;

	default:
		// Check for pet items
		if (anAPItem >= APItem::STINKY && anAPItem <= APItem::WALTER)
		{
			int aPetIdx = static_cast<int>(anAPItem) - static_cast<int>(APItem::STINKY);
			if (aPetIdx >= 0 && aPetIdx < 24)
			{
				aProfile->UnlockPet(aPetIdx, true);
			}
		}
		break;
	}

	std::string anItemName = APItemToString(anAPItem);
	QueueNotification(std::string("Received: ") + anItemName + "!");
	SetLastCheck(anItemName, "Received for yourself!");

	mApp->SaveCurrentUserData();
}

void APGameController::ReportLocation(APLocation theLocation)
{
	mApp->CheckArchipelagoLocation(theLocation);
	CheckVictoryCondition();
}

void APGameController::ReportEggPiece(const LevelData& theLevel, int thePiece)
{
	APLocation aLoc = gEggPieceLocations[theLevel.mTank - 1][theLevel.mLevel - 1][thePiece - 1];
	ReportLocation(aLoc);
	SetLastCheck("Archipelago Item!", "Location check sent to multiworld!");
}

void APGameController::ReportTankComplete(const LevelData& theLevel)
{
	ReportLocation(gTankCompleteLocations[theLevel.mTank - 1]);
}

void APGameController::ReportTimeTrialComplete(const LevelData& theLevel)
{
	ReportLocation(gTimeTrialLocations[theLevel.mTank - 1]);
}

void APGameController::ReportChallengeComplete(const LevelData& theLevel)
{
	ReportLocation(gChallengeLocations[theLevel.mTank - 1]);
}

void APGameController::ReportCyrax()
{
	ReportLocation(APLocation::CYRAX);
}

void APGameController::ReportBonusPet(int theBonusPetIndex)
{
	if (theBonusPetIndex < 0 || theBonusPetIndex > 3)
		return;

	ReportLocation(gBonusPetLocations[theBonusPetIndex]);
}

void APGameController::ReportAlienDefeated(int theAlienType)
{
	if (mBridge == NULL || !mBridge->GetSlotData().mAliensanity)
		return;

	APLocation aLoc = APLocation::NONE;
	switch (theAlienType)
	{
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

	ReportLocation(aLoc);
}

bool APGameController::CheckVictoryCondition()
{
	if (mBridge == NULL || mBridge->IsGoalSent())
		return false;

	bool victory =  mBridge->IsLocationChecked(APLocation::TANK_1_COMPLETE) &&
					mBridge->IsLocationChecked(APLocation::TANK_2_COMPLETE) &&
					mBridge->IsLocationChecked(APLocation::TANK_3_COMPLETE) &&
					mBridge->IsLocationChecked(APLocation::TANK_4_COMPLETE);
	
	if (mBridge->GetSlotData().mCyraxRequired)
	{
		victory = 	victory && 
					mBridge->IsLocationChecked(APLocation::CYRAX);
	}

	if (mBridge->GetSlotData().mChallengeModeRequired) 
	{ 
		victory =   victory &&
					mBridge->IsLocationChecked(APLocation::CHALLENGE_1_COMPLETE) &&
					mBridge->IsLocationChecked(APLocation::CHALLENGE_2_COMPLETE) &&
					mBridge->IsLocationChecked(APLocation::CHALLENGE_3_COMPLETE) &&
					mBridge->IsLocationChecked(APLocation::CHALLENGE_4_COMPLETE);
	}
	
	if (mBridge->GetSlotData().mTimeTrialRequired)
	{
		victory = 	victory &&   
					mBridge->IsLocationChecked(APLocation::TIME_TRIAL_1_COMPLETE) &&
					mBridge->IsLocationChecked(APLocation::TIME_TRIAL_2_COMPLETE) &&
					mBridge->IsLocationChecked(APLocation::TIME_TRIAL_3_COMPLETE) &&
					mBridge->IsLocationChecked(APLocation::TIME_TRIAL_4_COMPLETE);
	}
	
	if (victory)
	{
		mBridge->SendGoal();
		if (!mBridge->IsGoalSent())
			QueueNotification("Archipelago: Goal Achieved!");
		return true;
	}

	return false;
}

void APGameController::ApplyUpgradeAvailabilities(Board* theBoard)
{
	UserProfile* aProfile = GetProfile();
	if (theBoard == NULL || aProfile == NULL)
		return;

	int aFoodQuantity = aProfile->mAPFoodQuantity;
	int aFoodQuality = aProfile->mAPFoodQuality;
	int aWeaponLevel = aProfile->mAPWeaponLevel;

	// Set max upgrade limits based on items received from Archipelago
	theBoard->mMaxFoodLimit = 1 + aFoodQuantity;
	theBoard->mMaxFoodQuality = aFoodQuality;
	theBoard->mMaxWeaponUpgrade = (std::min)(12, 2 + aWeaponLevel);

	int aUpgradeAvailability = (aProfile->mAPUpgradeAvailability != 0)
		? aProfile->mAPUpgradeAvailability
		: (mBridge != NULL ? mBridge->GetSlotData().mUpgradeAvailability : UPGRADE_AVAILABILITY_INSTANT);

	static const int kUpgradeSlots[] = { SLOT_WEAPON, SLOT_FOODLIMIT, SLOT_FOODLVL };

	if (aUpgradeAvailability == UPGRADE_AVAILABILITY_INSTANT)
	{
		// Apply weapon upgrade level (base is 2, laser level = 2 + aWeaponLevel, max 12)
		if (aWeaponLevel > 0)
			theBoard->m0x3e4 = (std::min)(12, 2 + aWeaponLevel);

		// Apply food quality & quantity upgrades if received
		if (aFoodQuality > 0 && gFoodType < aFoodQuality)
			gFoodType = aFoodQuality;

		if (aFoodQuantity > 0 && gFoodLimit < (1 + aFoodQuantity))
			gFoodLimit = 1 + aFoodQuantity;

		// Disable food and weapon slots from the shop
		for (int aSlot : kUpgradeSlots)
		{
			theBoard->mSlotNumber[aSlot] = -1;
			theBoard->mSlotPrices[aSlot] = 0;
			theBoard->m0x374[aSlot] = -1;
			theBoard->mSlotUnlocked[aSlot] = false;
		}
	}
	else
	{
		// Boss tank (Tank 5) has no shop, grant weapon directly
		if (theBoard->mTank == 5 && aWeaponLevel > 0)
			theBoard->m0x3e4 = (std::min)(12, 2 + aWeaponLevel);

		for (int aSlot : kUpgradeSlots)
		{
			if (aUpgradeAvailability == UPGRADE_AVAILABILITY_FREE_IN_SHOP && theBoard->mSlotNumber[aSlot] > -1)
				theBoard->mSlotPrices[aSlot] = 0;

			if (theBoard->mSlotUnlocked[aSlot])
				theBoard->MenuButtonSetupNoVT(aSlot, false);
		}
	}

	theBoard->mWidgetManager->MarkAllDirty();
}

void APGameController::ApplyLevelStart(Board* theBoard)
{
	UserProfile* aProfile = GetProfile();
	if (theBoard == NULL || aProfile == NULL || mApp->IsBonusLevel())
		return;

	mFishDeathsCounter = 0;

	// set trap cooldown to 60-180 seconds (1-3 minutes)
	mTrapCooldown = (rand() % 60 + 120) * 60;

	// Apply starting money bonus
	int aTotalStartingMoney = aProfile->mAPStartingMoneyBonus;
	if (mBridge != NULL)
		aTotalStartingMoney += mBridge->GetSlotData().mStartingMoneyBonus;

	if (aTotalStartingMoney > 0)
	{
		theBoard->mMoney += aTotalStartingMoney;
		theBoard->UpdateMoneyLabelText();
	}
	
	ApplyUpgradeAvailabilities(theBoard);
}

template<typename Func>
static void ForEachFishList(Board* theBoard, Func&& func)
{
	func(theBoard->mFishList);
	func(theBoard->mBreederList);
	func(theBoard->mOscarList);
	func(theBoard->mPentaList);
	func(theBoard->mUltraList);
	func(theBoard->mGrubberList);
	func(theBoard->mGekkoList);
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
			ForEachFishList(theBoard, [newHunger](auto* aList)
			{
				if (aList != NULL)
				{
					for (auto* aFish : *aList)
					{
						if (aFish != NULL)
							aFish->mHunger = newHunger;
					}
				}
			});
		}
	}

	// Service Death Links during active gameplay
	if (!mPendingDeathLinks.empty() && !theBoard->mIsBonusRound)
	{
		APDeathLink aDeathLink = mPendingDeathLinks.front();
		int anEffect = mBridge->GetSlotData().mDeathLinkEffect;

		if (anEffect == DEATH_LINK_EFFECT_INSTANT_KILL)
		{
			// If fish are present in the tank, execute instant kill
			if (theBoard->HasAnyFish())
			{
				mPendingDeathLinks.pop_front();

				mSuppressDeathLink = true;
				ForEachFishList(theBoard, [](auto* aList)
				{
					if (aList != NULL)
					{
						auto aCopy = *aList;
						for (auto* aFish : aCopy)
						{
							if (aFish != NULL)
								aFish->Die(true);
						}
					}
				});
				mSuppressDeathLink = false;

				std::string aMsg = aDeathLink.mCause.empty() ?
					("DEATH LINK: " + aDeathLink.mSource + " died!") :
					("DEATH LINK: " + aDeathLink.mCause);
				QueueNotification(aMsg);
			}
		}
		else if (anEffect == DEATH_LINK_EFFECT_RANDOM_TRAP)
		{
			mPendingDeathLinks.pop_front();

			APItem aRandomTrap = (rand() % 2 == 0) ? APItem::TRAP_ALIEN_ATTACK : APItem::TRAP_FAMINE;
			mPendingTraps.push_back(aRandomTrap);

			QueueNotification("DEATH LINK from " + aDeathLink.mSource + ": Trap queued!");
		}
	}
}

void APGameController::OnFishDied(const std::string& theCause)
{
	if (mBridge == NULL || !mBridge->GetSlotData().mDeathLink)
		return;

	if (mSuppressDeathLink)
		return;

	if (mApp->mBoard == NULL || mApp->mBoard->mIsBonusRound || mApp->mGameMode == GAMEMODE_VIRTUAL_TANK)
		return;

	mFishDeathsCounter++;
	int anAmnesty = mBridge->GetSlotData().mDeathLinkAmnesty;

	if (mFishDeathsCounter > anAmnesty)
	{
		std::string aCauseStr = mBridge->GetSlotName() + "'s " + theCause;
		mBridge->SendDeathLink(aCauseStr);
		mFishDeathsCounter = 0;

		QueueNotification("DEATH LINK SENT!");
	}
}

void APGameController::QueueNotification(const std::string& theMessage)
{
	if (theMessage.empty())
		return;

	mNotificationQueue.push_back(theMessage);
}
