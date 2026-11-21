#include <SexyAppFramework/DialogButton.h>
#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/KeyCodes.h>

#include "ArchipelagoInfo.h"
#include "WinFishCommon.h"
#include "WinFishApp.h"
#include "BubbleMgr.h"
#include "Res.h"
#include "APGameController.h"
#include "ProfileMgr.h"

using namespace Sexy;

int Sexy::gArchipelagoInfoTab = 1;

Sexy::ArchipelagoInfo::ArchipelagoInfo(WinFishApp* theApp)
{
	mApp = theApp;
	mBubbleMgr = new BubbleMgr();
	mBubbleMgr->mBubbleBounds = Rect(0, 80, 640, 400);
	mBubbleMgr->mBubbleFishBounds = Rect(0, 80, 640, 350);
	mBubbleMgr->SetBubbleConfig(20, 3);
	mBubbleMgr->UpdateALot();

	mMenuButton = MakeDialogButton2(1, this, "Menu", IMAGE_MAINBUTTON);
	mMenuButton->Resize(525, 4, 80, mMenuButton->mHeight);

	mPreviousButton = MakeDialogButton2(3, this, "Previous", IMAGE_MAINBUTTON);
	mPreviousButton->Resize(190, 430, 120, mPreviousButton->mHeight);

	mNextButton = MakeDialogButton2(2, this, "Next", IMAGE_MAINBUTTON);
	mNextButton->Layout(LAY_SameBottom | LAY_Right | LAY_SameHeight | LAY_SameWidth, mPreviousButton, 20);

	if (mApp->mBoard != NULL)
	{
		mMenuButton->Resize(480, 4, 125, mMenuButton->mHeight);
		mMenuButton->mLabel = "Back to Game";
	}

	mPageIdx = gArchipelagoInfoTab;
	if (mPageIdx < 1 || mPageIdx > 5)
		mPageIdx = 1;
}

Sexy::ArchipelagoInfo::~ArchipelagoInfo()
{
	if (mPageIdx >= 1 && mPageIdx <= 5)
		gArchipelagoInfoTab = mPageIdx;

	if (mBubbleMgr)
		delete mBubbleMgr;
	if (mMenuButton)
		delete mMenuButton;
	if (mNextButton)
		delete mNextButton;
	if (mPreviousButton)
		delete mPreviousButton;
}

void Sexy::ArchipelagoInfo::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMenuButton);
	theWidgetManager->AddWidget(mNextButton);
	theWidgetManager->AddWidget(mPreviousButton);
}

void Sexy::ArchipelagoInfo::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMenuButton);
	theWidgetManager->RemoveWidget(mNextButton);
	theWidgetManager->RemoveWidget(mPreviousButton);
}

void Sexy::ArchipelagoInfo::Update()
{
	Widget::Update();
	if (mBubbleMgr)
		mBubbleMgr->Update();
	MarkDirty();
}

void Sexy::ArchipelagoInfo::Draw(Graphics* g)
{
	// 1. Underwater gradient background
	int aLumin = 107;
	for (int i = 0; i < 480; i += 4)
	{
		g->SetColor(Color(mApp->HSLToRGB(151, 138, aLumin)));
		g->FillRect(0, i, mWidth, 4);
		if (--aLumin < 50)
			aLumin = 50;
	}

	if (mBubbleMgr)
		mBubbleMgr->Draw(g);
	DrawWaves(g, 80, mUpdateCnt);

	// 2. Page Subtitle & Number
	SexyString aPageTitle = "";
	switch (mPageIdx)
	{
	case 1:
		aPageTitle = "RECEIVED ITEMS";
		break;
	case 2:
		aPageTitle = "TANK 1";
		break;
	case 3:
		aPageTitle = "TANK 2";
		break;
	case 4:
		aPageTitle = "TANK 3";
		break;
	case 5:
		aPageTitle = "TANK 4";
		break;
	default:
		aPageTitle = "ARCHIPELAGO PROGRESS";
		break;
	}

	g->SetFont(FONT_CONTINUUMBOLD14);
	g->SetColor(Color::White);
	DrawStringWithOutline(g, aPageTitle, 320 - FONT_CONTINUUMBOLD14->StringWidth(aPageTitle) / 2, 58, FONT_CONTINUUMBOLD14OUTLINE, 0);

	SexyString aTabPage = StrFormat("Page %d of 5", mPageIdx);
	DrawStringWithOutline(g, aTabPage, 320 - FONT_CONTINUUMBOLD14->StringWidth(aTabPage) / 2, 78, FONT_CONTINUUMBOLD14OUTLINE, 0);

	// 3. Page Body Container
	Rect aMainBox(18, 98, 604, 322);
	g->SetColor(Color(0, 0, 0, 45));
	g->FillRect(aMainBox);
	g->SetColor(Color(0x316584));
	g->DrawRect(aMainBox);

	// 4. Draw page-specific content
	if (mPageIdx == 1)
	{
		DrawReceivedItemsPage(g);
	}
	else
	{
		DrawTankPage(g, mPageIdx - 1);
	}

	// 5. Top banner (drawn on top)
	g->DrawImageBox(Rect(20, 0, 600, IMAGE_SCREENTITLE->mHeight), IMAGE_SCREENTITLE);
	g->DrawImageBox(Rect(mMenuButton->mX - 1, mMenuButton->mY - 1, mMenuButton->mWidth + 2, IMAGE_SCREENTITLEHOLE->mHeight), IMAGE_SCREENTITLEHOLE);

	g->SetFont(FONT_JUNGLEFEVER17OUTLINE);
	g->SetColor(Color(0xff, 200, 0, 0xff));
	WriteCenteredLine(g, 25, "Archipelago Progress");
}

void Sexy::ArchipelagoInfo::DrawItemRow(Graphics* g, const Rect& theRect, int theRowIndex,
	const SexyString& theName, const SexyString& theValueStr, const SexyString& theDesc)
{
	// Row background
	g->SetColor(Color(0, 0, 0, 35));
	g->FillRect(theRect);
	g->SetColor(Color(0x316584));
	g->DrawRect(theRect);

	// Icon box on the left
	Rect aIconRect(theRect.mX + 6, theRect.mY + 4, 54, 44);
	g->SetColor(Color(0x60000000));
	g->FillRect(aIconRect);
	g->SetColor(Color(0x316584));
	g->DrawRect(aIconRect);

	// Icon
	switch (theRowIndex)
	{
	case 0: // Food quality
	{
		int aQualityCel = (mApp->mCurrentProfile != NULL) ? mApp->mCurrentProfile->mAPFoodQuality : 0;
		if (aQualityCel > 2) aQualityCel = 2;
		if (aQualityCel < 0) aQualityCel = 0;
		g->DrawImageCel(IMAGE_FOOD,
			aIconRect.mX + (aIconRect.mWidth - IMAGE_FOOD->GetCelWidth()) / 2,
			aIconRect.mY + (aIconRect.mHeight - IMAGE_FOOD->GetHeight()) / 2,
			(mUpdateCnt / 4) % 10, aQualityCel);
		break;
	}
	case 1: // Food quantity
	{
		int aPelletCel = (mUpdateCnt / 3) % 10;
		g->DrawImageCel(IMAGE_FOOD, aIconRect.mX + 8, aIconRect.mY + 12, aPelletCel, 0);
		g->DrawImageCel(IMAGE_FOOD, aIconRect.mX + 24, aIconRect.mY + 12, (aPelletCel + 3) % 10, 0);
		break;
	}
	case 2: // Weapon upgrade
	{
		g->DrawImageCel(IMAGE_LASERUPGRADES,
			aIconRect.mX + (aIconRect.mWidth - IMAGE_LASERUPGRADES->GetCelWidth()) / 2,
			aIconRect.mY + (aIconRect.mHeight - IMAGE_LASERUPGRADES->GetHeight()) / 2,
			(mUpdateCnt / 36) % 3);
		break;
	}
	case 3: // Pets
	{
		g->DrawImageCel(IMAGE_SCL_ITCHY,
			aIconRect.mX + (aIconRect.mWidth - IMAGE_SCL_ITCHY->GetCelWidth()) / 2,
			aIconRect.mY + (aIconRect.mHeight - IMAGE_SCL_ITCHY->GetHeight()) / 2,
			(mUpdateCnt / 4) % 10);
		break;
	}
	case 4: // Pet slots
	{
		g->DrawImage(IMAGE_HELPPETCHOOSE,
			aIconRect.mX + (aIconRect.mWidth - IMAGE_HELPPETCHOOSE->mWidth) / 2,
			aIconRect.mY + (aIconRect.mHeight - IMAGE_HELPPETCHOOSE->mHeight) / 2);
		break;
	}
	default:
		break;
	}

	// Title
	g->SetFont(FONT_CONTINUUMBOLD14);
	g->SetColor(Color(0xffffaa));
	DrawStringWithOutline(g, theName, theRect.mX + 70, theRect.mY + 22, FONT_CONTINUUMBOLD14OUTLINE, 0);

	// Description
	g->SetFont(FONT_JUNGLEFEVER10OUTLINE);
	SetVaryingBlueColorGraphics(g, mUpdateCnt, theRect.mY + 39);
	g->DrawString(theDesc, theRect.mX + 70, theRect.mY + 39);

	// Value (received/total)
	g->SetFont(FONT_CONTINUUMBOLD14);
	g->SetColor(Color::White);
	int aValWidth = FONT_CONTINUUMBOLD14->StringWidth(theValueStr);
	DrawStringWithOutline(g, theValueStr, theRect.mX + theRect.mWidth - 25 - aValWidth, theRect.mY + 32, FONT_CONTINUUMBOLD14OUTLINE, 0);
}

void Sexy::ArchipelagoInfo::DrawReceivedItemsPage(Graphics* g)
{
	UserProfile* aProf = mApp->mCurrentProfile;

	// Food quality (received/total)
	int aFoodQuality = aProf ? aProf->mAPFoodQuality : 0;
	int aTotalFoodQuality = 2;
	SexyString aFoodQualityStr = StrFormat("%d / %d", aFoodQuality, aTotalFoodQuality);

	// Food quantity (received/total)
	int aFoodQuantity = aProf ? aProf->mAPFoodQuantity : 0;
	int aTotalFoodQuantity = 8;
	SexyString aFoodQuantityStr = StrFormat("%d / %d", aFoodQuantity, aTotalFoodQuantity);

	// Weapon upgrade (received/total)
	int aWeaponLevel = aProf ? aProf->mAPWeaponLevel : 0;
	int aTotalWeaponLevel = 10;
	SexyString aWeaponStr = StrFormat("%d / %d", aWeaponLevel, aTotalWeaponLevel);

	// Pets (received/total)
	int aPetsUnlocked = 0;
	if (aProf != NULL)
	{
		for (int i = 0; i < 24; ++i)
		{
			if (aProf->IsPetUnlocked(i))
				aPetsUnlocked++;
		}
	}
	int aTotalPets = 24;
	SexyString aPetsStr = StrFormat("%d / %d", aPetsUnlocked, aTotalPets);

	// Pet slots (received/total)
	int aStartPetSlots = (mApp->mAPBridge != NULL) ? mApp->mAPBridge->GetSlotData().mStartPetSlots : 0;
	int aTotalPetSlotUpgrades = 24 - aStartPetSlots;
	if (aTotalPetSlotUpgrades <= 0)
		aTotalPetSlotUpgrades = 24;
	int aPetSlotsReceived = aProf ? aProf->mAPPetSlots : 0;
	SexyString aPetSlotsStr = StrFormat("%d / %d", aPetSlotsReceived, aTotalPetSlotUpgrades);

	// Draw 5 rows
	DrawItemRow(g, Rect(28, 106, 584, 52), 0, "Food Quality", aFoodQualityStr, "Nutrient quality upgrades for food pellets");
	DrawItemRow(g, Rect(28, 168, 584, 52), 1, "Food Quantity", aFoodQuantityStr, "Maximum food pellets dropped on screen at once");
	DrawItemRow(g, Rect(28, 230, 584, 52), 2, "Weapon Upgrade", aWeaponStr, "Progressive weapon laser firepower against aliens");
	DrawItemRow(g, Rect(28, 292, 584, 52), 3, "Pets", aPetsStr, "Helper companion pets unlocked across all tanks");
	DrawItemRow(g, Rect(28, 354, 584, 52), 4, "Pet Slots", aPetSlotsStr, "Companion pet slots unlocked for aquarium tanks");
}

void Sexy::ArchipelagoInfo::DrawTankRow(Graphics* g, const Rect& theRect, int theRowIndex,
	const SexyString& theName, const SexyString& theValueStr, bool isComplete, bool isCheckmarkRow, const SexyString& theDesc)
{
	// Row background
	g->SetColor(Color(0, 0, 0, 35));
	g->FillRect(theRect);
	g->SetColor(Color(0x316584));
	g->DrawRect(theRect);

	// Icon box on the left
	Rect aIconRect(theRect.mX + 10, theRect.mY + 9, 68, 60);
	g->SetColor(Color(0x60000000));
	g->FillRect(aIconRect);
	g->SetColor(Color(0x316584));
	g->DrawRect(aIconRect);

	// Icon
	switch (theRowIndex)
	{
	case 0: // Egg piece checks
	{
		g->DrawImageCel(IMAGE_EGGPIECES,
			aIconRect.mX + (aIconRect.mWidth - IMAGE_EGGPIECES->GetCelWidth()) / 2,
			aIconRect.mY + (aIconRect.mHeight - IMAGE_EGGPIECES->GetHeight()) / 2,
			(mUpdateCnt / 36) % 3);
		break;
	}
	case 1: // Challenge mode
	{
		g->DrawImageCel(IMAGE_TROPHYBUTTONS,
			aIconRect.mX + (aIconRect.mWidth - IMAGE_TROPHYBUTTONS->GetCelWidth()) / 2,
			aIconRect.mY + (aIconRect.mHeight - IMAGE_TROPHYBUTTONS->GetHeight()) / 2,
			4);
		break;
	}
	case 2: // Time Trial
	{
		g->DrawImageCel(IMAGE_TROPHYBUTTONS,
			aIconRect.mX + (aIconRect.mWidth - IMAGE_TROPHYBUTTONS->GetCelWidth()) / 2,
			aIconRect.mY + (aIconRect.mHeight - IMAGE_TROPHYBUTTONS->GetHeight()) / 2,
			2);
		break;
	}
	default:
		break;
	}

	// Title
	g->SetFont(FONT_CONTINUUMBOLD14);
	g->SetColor(Color(0xffffaa));
	DrawStringWithOutline(g, theName, theRect.mX + 90, theRect.mY + 28, FONT_CONTINUUMBOLD14OUTLINE, 0);

	// Description
	g->SetFont(FONT_JUNGLEFEVER10OUTLINE);
	SetVaryingBlueColorGraphics(g, mUpdateCnt, theRect.mY + 50);
	g->DrawString(theDesc, theRect.mX + 90, theRect.mY + 50);

	// Status on right
	if (isCheckmarkRow)
	{
		if (isComplete)
		{
			// Checkmark
			int aImgY = theRect.mY + (theRect.mHeight - IMAGE_CHECKED->mHeight) / 2;
			g->DrawImage(IMAGE_CHECKED, theRect.mX + theRect.mWidth - 145, aImgY);

			g->SetFont(FONT_CONTINUUMBOLD14);
			g->SetColor(Color(100, 255, 100));
			DrawStringWithOutline(g, "Complete", theRect.mX + theRect.mWidth - 110, theRect.mY + 46, FONT_CONTINUUMBOLD14OUTLINE, 0);
		}
		else
		{
			// X
			g->SetFont(FONT_CONTINUUMBOLD14);
			g->SetColor(Color(255, 60, 60));
			DrawStringWithOutline(g, "X", theRect.mX + theRect.mWidth - 138, theRect.mY + 46, FONT_CONTINUUMBOLD14OUTLINE, 0);

			g->SetColor(Color(255, 130, 130));
			DrawStringWithOutline(g, "Incomplete", theRect.mX + theRect.mWidth - 110, theRect.mY + 46, FONT_CONTINUUMBOLD14OUTLINE, 0);
		}
	}
	else
	{
		// Numeric (remaining/total)
		g->SetFont(FONT_CONTINUUMBOLD14);
		g->SetColor(Color::White);
		int aValWidth = FONT_CONTINUUMBOLD14->StringWidth(theValueStr);
		DrawStringWithOutline(g, theValueStr, theRect.mX + theRect.mWidth - 25 - aValWidth, theRect.mY + 36, FONT_CONTINUUMBOLD14OUTLINE, 0);

		g->SetFont(FONT_JUNGLEFEVER10OUTLINE);
		g->SetColor(Color(0xaa, 0xee, 0xff));
		SexyString aSubLabel = "Remaining";
		int aSubWidth = FONT_JUNGLEFEVER10OUTLINE->StringWidth(aSubLabel);
		g->DrawString(aSubLabel, theRect.mX + theRect.mWidth - 25 - aSubWidth, theRect.mY + 54);
	}
}

void Sexy::ArchipelagoInfo::DrawTankPage(Graphics* g, int theTank)
{
	if (theTank < 1 || theTank > 4)
		return;

	int t = theTank - 1;
	UserProfile* aProf = mApp->mCurrentProfile;

	// 1. Egg piece checks: (remaining)/(total)
	int aTotalEggPieces = 15;
	int aCheckedEggPieces = 0;
	for (int l = 0; l < 5; ++l)
	{
		for (int p = 0; p < 3; ++p)
		{
			APLocation aLoc = gEggPieceLocations[t][l][p];
			bool isChecked = false;
			if (mApp->mAPBridge != NULL && mApp->mAPBridge->IsLocationChecked(aLoc))
				isChecked = true;
			else if (aProf != NULL && aProf->mAPPendingChecks.count(aLoc) > 0)
				isChecked = true;

			if (isChecked)
				aCheckedEggPieces++;
		}
	}
	int aRemainingEggPieces = aTotalEggPieces - aCheckedEggPieces;
	SexyString aEggStr = StrFormat("%d / %d", aRemainingEggPieces, aTotalEggPieces);
	SexyString aEggDesc = StrFormat("%d of %d checks found across levels %d-1 through %d-5",
		aCheckedEggPieces, aTotalEggPieces, theTank, theTank);

	// 2. Challenge mode completion (checkmark or X)
	bool aChallengeDone = false;
	APLocation aChalLoc = gChallengeLocations[t];
	if (mApp->mAPBridge != NULL && mApp->mAPBridge->IsLocationChecked(aChalLoc))
		aChallengeDone = true;
	else if (aProf != NULL && aProf->mAPPendingChecks.count(aChalLoc) > 0)
		aChallengeDone = true;

	SexyString aChalDesc = "Survive 8 intense alien attack waves in Challenge Mode";

	// 3. Time Trial completion (checkmark or X)
	bool aTimeTrialDone = false;
	APLocation aTTLoc = gTimeTrialLocations[t];
	if (mApp->mAPBridge != NULL && mApp->mAPBridge->IsLocationChecked(aTTLoc))
		aTimeTrialDone = true;
	else if (aProf != NULL && aProf->mAPPendingChecks.count(aTTLoc) > 0)
		aTimeTrialDone = true;

	SexyString aTTDesc = StrFormat("Reach %d shells before the time limit expires", TIME_TRIAL_GOALS[t]);

	// Draw 3 rows
	DrawTankRow(g, Rect(28, 118, 584, 78), 0, "Egg Piece Checks", aEggStr, false, false, aEggDesc);
	DrawTankRow(g, Rect(28, 212, 584, 78), 1, "Challenge Mode Completion", "", aChallengeDone, true, aChalDesc);
	DrawTankRow(g, Rect(28, 306, 584, 78), 2, "Time Trial Completion", "", aTimeTrialDone, true, aTTDesc);
}

void Sexy::ArchipelagoInfo::KeyDown(KeyCode theKey)
{
	Widget::KeyDown(theKey);

	if (theKey == KEYCODE_ESCAPE)
	{
		ButtonDepress(1);
	}
	else if (theKey == KEYCODE_RIGHT || theKey == 'D' || theKey == 'd')
	{
		ButtonDepress(2);
	}
	else if (theKey == KEYCODE_LEFT || theKey == 'A' || theKey == 'a')
	{
		ButtonDepress(3);
	}
}

void Sexy::ArchipelagoInfo::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

void Sexy::ArchipelagoInfo::ButtonDepress(int theId)
{
	if (theId == 1) // Menu / Back to Game
	{
		mApp->RemoveArchipelagoInfo();
		if (!mApp->mBoard)
		{
			mApp->SwitchToGameSelector();
			return;
		}
	}
	else if (theId == 2) // Next
	{
		mPageIdx++;
		if (mPageIdx > 5)
			mPageIdx = 1;
		return;
	}
	else if (theId == 3) // Previous
	{
		mPageIdx--;
		if (mPageIdx < 1)
			mPageIdx = 5;
		return;
	}
}
