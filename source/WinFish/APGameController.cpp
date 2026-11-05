#include "APGameController.h"
#include "WinFishApp.h"
#include "Board.h"
#include "ProfileMgr.h"
#include "Res.h"
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
	mNotificationCooldown(0)
{
	for (int i = 0; i < 5; i++)
		mUnlockedTanks[i] = false;
	for (int i = 0; i < 24; i++)
		mUnlockedPets[i] = false;
}

APGameController::~APGameController()
{
}

void APGameController::Update()
{
	// Process non-board specific timer updates if needed
	if (mNotificationCooldown > 0)
		mNotificationCooldown--;

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
	OutputDebugStringA(StrFormat("APGameController: Connected as '%s' on seed '%s'\n", theSlot.c_str(), theSeed.c_str()).c_str());
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
	for (int i = 0; i < 5; i++)
		mUnlockedTanks[i] = false;
	for (int i = 0; i < 24; i++)
		mUnlockedPets[i] = false;

	mPendingTraps.clear();
	mNotificationQueue.clear();
	mReceivedHistory.clear();
	mNotificationCooldown = 0;
}

bool APGameController::IsTankUnlocked(int theTank) const
{
	if (theTank >= 1 && theTank <= 4)
		return mUnlockedTanks[theTank];
	if (theTank == 5)
		return mUnlockedTanks[1] && mUnlockedTanks[2] && mUnlockedTanks[3] && mUnlockedTanks[4];
	return false;
}

bool APGameController::IsPetUnlocked(int thePetIndex) const
{
	if (thePetIndex >= 0 && thePetIndex < 24)
		return mUnlockedPets[thePetIndex];
	return false;
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
		break;

	case APItem::FOOD_QUANTITY:
		mFoodQuantity++;
		if (mFoodQuantity > 8)
			mFoodQuantity = 8;
		break;

	case APItem::PET_SLOT:
		mPetSlots++;
		break;

	case APItem::WEAPON_UPGRADE:
		mWeaponLevel++;
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
		SaveToProfile(mApp->mCurrentProfile);
}

void APGameController::ReportLocation(int64_t theLocationId, const std::string& theLocationName)
{
	(void)theLocationName;
	if (mApp != NULL)
	{
		mApp->CheckArchipelagoLocation(static_cast<int>(theLocationId));
	}
}

void APGameController::ReportEggPiece(int theTank, int theLevel, int thePiece)
{
	if (theTank < 1 || theTank > 4 || theLevel < 1 || theLevel > 5 || thePiece < 1 || thePiece > 3)
		return;

	// Formula matching APDefinitions.h:
	// Tank 1 starts at 2 (Tank 1 Complete is 1, then Tank 1-1 Piece 1 is 2)
	// Each tank block is 17 locations (1 Tank Complete + 15 Egg Pieces + 1 Time Trial Complete)
	int64_t aLocId = static_cast<int64_t>((theTank - 1) * 17 + 2 + (theLevel - 1) * 3 + (thePiece - 1));
	std::string aLocName = StrFormat("Tank %d-%d Egg Piece %d", theTank, theLevel, thePiece);
	ReportLocation(aLocId, aLocName);
}

void APGameController::ReportTankComplete(int theTank)
{
	if (theTank < 1 || theTank > 4)
		return;

	// Tank 1 Complete = 1, Tank 2 = 18, Tank 3 = 35, Tank 4 = 52
	int64_t aLocId = static_cast<int64_t>(1 + (theTank - 1) * 17);
	std::string aLocName = StrFormat("Tank %d Complete", theTank);
	ReportLocation(aLocId, aLocName);
}

void APGameController::ReportTimeTrialComplete(int theTank)
{
	if (theTank < 1 || theTank > 4)
		return;

	// Time Trial 1 = 17, Time Trial 2 = 34, Time Trial 3 = 51, Time Trial 4 = 68
	int64_t aLocId = static_cast<int64_t>(theTank * 17);
	std::string aLocName = StrFormat("Time Trial %d Complete", theTank);
	ReportLocation(aLocId, aLocName);
}

void APGameController::ReportCyrax()
{
	ReportLocation(static_cast<int64_t>(APLocation::CYRAX), "Cyrax");
}

void APGameController::ReportBonusPet(int theBonusPetIndex)
{
	if (theBonusPetIndex < 0 || theBonusPetIndex > 3)
		return;

	int64_t aLocId = static_cast<int64_t>(APLocation::BONUS_PET_1) + theBonusPetIndex;
	ReportLocation(aLocId, StrFormat("Bonus Pet %d", theBonusPetIndex + 1));
}

void APGameController::ApplyLevelStart(Board* theBoard)
{
	if (theBoard == NULL)
		return;

	// Apply starting money bonus
	if (mStartingMoneyBonus > 0)
	{
		theBoard->mMoney += mStartingMoneyBonus;
		theBoard->UpdateMoneyLabelText();
	}

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
}

void APGameController::UpdateBoard(Board* theBoard)
{
	if (theBoard == NULL)
		return;

	// Service notification queue via non-blocking ShowText
	if (mNotificationCooldown <= 0 && !mNotificationQueue.empty())
	{
		std::string aMsg = mNotificationQueue.front();
		mNotificationQueue.pop_front();

		// Show non-blocking green notification on board
		theBoard->ShowText(aMsg, false, -1);
		mNotificationCooldown = 180; // ~3 seconds at 60fps
	}

	// Service traps during active gameplay
	if (!mPendingTraps.empty())
	{
		APItem aTrap = mPendingTraps.front();
		mPendingTraps.pop_front();

		if (aTrap == APItem::TRAP_ALIEN_ATTACK)
		{
			// Force alien timer to zero or trigger alien arrival
			theBoard->mAlienTimer = 1;
			theBoard->ShowText("ALIEN ATTACK TRAP ACTIVATED!", true, -1);
		}
		else if (aTrap == APItem::TRAP_FAMINE)
		{
			// Set guppies hunger high
			theBoard->ShowText("FAMINE TRAP ACTIVATED!", true, -1);
		}
	}
}

void APGameController::QueueNotification(const std::string& theMessage)
{
	if (theMessage.empty())
		return;

	// Prevent excessive queue growth
	if (mNotificationQueue.size() >= 20)
		mNotificationQueue.pop_front();

	mNotificationQueue.push_back(theMessage);
}

void APGameController::LoadFromProfile(UserProfile* theProfile)
{
	if (theProfile == NULL)
		return;

	mFoodQuality = theProfile->mAPFoodQuality;
	mFoodQuantity = theProfile->mAPFoodQuantity;
	mPetSlots = theProfile->mAPPetSlots;
	mWeaponLevel = theProfile->mAPWeaponLevel;
	mStartingMoneyBonus = theProfile->mAPStartingMoneyBonus;

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
	theProfile->mAPWeaponLevel = mWeaponLevel;
	theProfile->mAPStartingMoneyBonus = mStartingMoneyBonus;

	for (int i = 0; i < 4; i++)
		theProfile->mAPUnlockedTanks[i] = mUnlockedTanks[i + 1];
}
