#include <SexyAppFramework/DialogButton.h>
#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Font.h>

#include "HatchScreen.h"
#include "WinFishApp.h"
#include "WinFishCommon.h"
#include "ProfileMgr.h"
#include "BubbleMgr.h"
#include "APGameController.h"
#include "Res.h"

Sexy::HatchScreen::HatchScreen(WinFishApp* theApp, int thePetId)
{
	mApp = theApp;
	mBubbleMgr = new BubbleMgr();

	mBubbleMgr->mBubbleBounds = Rect(190, 220, 258, 200);
	mBubbleMgr->SetBubbleConfig(10, 5);
	mBubbleMgr->UpdateALot();
	m0x94 = 0;
	mPetId = thePetId;
	theApp->m0x884 = true;
	mX = 0;
	mY = 0;
	mWidth = mApp->mWidth;
	mHeight = mApp->mHeight;

	mContinueButton = MakeDialogButton2(99, this, "Please Wait...", IMAGE_MAINBUTTON);
	mContinueButton->SetFont(FONT_JUNGLEFEVER12OUTLINE);
	mContinueButton->mMouseVisible = false;
	mContinueButton->mColors[0] = Color(255, 240, 0, 255);
	mContinueButton->Resize(186, 445, 264, mContinueButton->mHeight);
	mApp->mWidgetManager->BringToFront(mContinueButton);

	mMenuButton = MakeDialogButton2(100, this, "Menu", IMAGE_MAINBUTTON);
	mMenuButton->Resize(525, 4, 80, mMenuButton->mHeight);
	if (mApp->mCurrentProfile->mTank == 5 && mApp->mCurrentProfile->mLevel == 2)
		mMenuButton->mVisible = false;

	mOverlay = new HatchScreenOverlay();
	mOverlay->mScreen = this;
	mOverlay->mMouseVisible = false;
	mOverlay->mHasAlpha = true;
	mOverlay->mWidth = 640;
	mOverlay->mHeight = 480;

	m0x9c = 0;
	m0xa0 = 0;
	m0xa4 = 480;
	mApp->StopMusic();
	mApp->PlayMusic(2, 54);
}

Sexy::HatchScreen::~HatchScreen()
{
	if (mBubbleMgr)
		delete mBubbleMgr;
	if (mContinueButton)
		delete mContinueButton;
	if (mMenuButton)
		delete mMenuButton;
	if (mOverlay)
		delete mOverlay;
}

void Sexy::HatchScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mContinueButton);
	theWidgetManager->AddWidget(mMenuButton);
	theWidgetManager->AddWidget(mOverlay);
}

void Sexy::HatchScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mContinueButton);
	theWidgetManager->RemoveWidget(mMenuButton);
	theWidgetManager->RemoveWidget(mOverlay);
}

void Sexy::HatchScreen::Update()
{
	mBubbleMgr->Update();
	MarkDirty();
	m0x9c = mApp->mSeed->Next() % 4;
	m0xa0 = mApp->mSeed->Next() % 2;

	if (!mIsDown || m0x94 > 139)
	{
		if (m0x94 == 141)
		{
			mApp->PlaySample(SOUND_HATCH);
		}
	}
	else
	{
		m0x94 = 140;
	}

	if (mApp->mGameMode == GAMEMODE_TIME_TRIAL)
	{
		if (m0x94 == 28 || m0x94 == 48 || m0x94 == 68)
			mApp->PlaySample(SOUND_BUY);
		if (m0x94 == 88)
			mApp->PlaySample(SOUND_GROW);
	}

	if (m0x94 == 170)
	{
		mContinueButton->mLabel = "Click Here to Continue";
		mContinueButton->mMouseVisible = true;
	}

	m0x94++;
}

void Sexy::HatchScreen::Draw(Graphics* g)
{
	g->DrawImage(IMAGE_HATCHSCREEN, 0, 0);
	g->DrawImageBox(Rect(20, 0, 600, IMAGE_SCREENTITLE->mHeight), IMAGE_SCREENTITLE);

	if (mMenuButton->mVisible)
		g->DrawImageBox(Rect(mMenuButton->mX - 1, mMenuButton->mY - 1, mMenuButton->mWidth + 2, IMAGE_SCREENTITLEHOLE->mHeight), IMAGE_SCREENTITLEHOLE);
	
	g->SetFont(FONT_JUNGLEFEVER17OUTLINE);
	g->SetColor(Color(0xff, 200, 0, 0xff));

	WriteCenteredLine(g, 25, "Found an Archipelago Item!");

	int aShakeTimer = m0x94 % 32;
	int anYOffset = 0;
	if (aShakeTimer < 4)
		anYOffset = 0;
	else if (aShakeTimer < 8)
		anYOffset = -1;
	else if (aShakeTimer < 12)
		anYOffset = -2;
	else if (aShakeTimer < 16)
		anYOffset = -1;
	else if (aShakeTimer < 20)
		anYOffset = 0;
	else if (aShakeTimer < 24)
		anYOffset = 1;
	else if (aShakeTimer < 28)
		anYOffset = 2;
	else if (aShakeTimer < 32)
		anYOffset = 1;

	Graphics g2(*g);
	if (m0x94 < 81)
	{
		int aCel = 0;
		if (m0x94 > 55)
			aCel = (m0x94 - 56) / 2;
		g->DrawImageCel(IMAGE_EGGCRACK1, 258, anYOffset + 60, aCel);
	}
	else if (m0x94 < 141)
	{
		int aCel = 0;
		if (m0x94 > 119)
			aCel = (m0x94 - 120) / 2;
		g->DrawImageCel(IMAGE_EGGCRACK2, m0x9c + 258, m0xa0 + 60 + anYOffset, aCel);
	}
	else
	{
		int aX = 278;
		int aY = 90;
		g2.ClipRect(aX, aY, 80, 80);
		if (IMAGE_ARCHIBALD != NULL)
		{
			int aCol = ((m0x94 - 140) / 2) % 10;
			if (aCol < 0)
				aCol = 0;
			g2.DrawImageCel(IMAGE_ARCHIBALD, aX, aY, aCol, 1);
		}
		g2.SetColorizeImages(false);

		SexyString aItemTitle = "Archipelago Item!";
		SexyString aItemDesc1 = "Location check sent to multiworld!";
		SexyString aItemDesc2 = "";

		if (mApp != NULL && mApp->mAPController != NULL)
		{
			const std::string& aLastTitle = mApp->mAPController->GetLastCheckTitle();
			const std::string& aLastDetail = mApp->mAPController->GetLastCheckDetail();
			if (!aLastTitle.empty())
			{
				aItemTitle = aLastTitle;
				aItemDesc1 = aLastDetail;
			}
		}

		g->SetColor(Color(255, 200, 0));
		g->SetFont(FONT_JUNGLEFEVER15OUTLINE);
		if (g->GetFont()->StringWidth(aItemTitle) > 400)
			g->SetFont(FONT_JUNGLEFEVER12OUTLINE);
		WriteCenteredLine(g, 260, aItemTitle);

		if (m0x94 > 170)
		{
			g->SetColor(Color::White);
			g->SetFont(FONT_JUNGLEFEVER10OUTLINE);
			WriteCenteredLine(g, 305, aItemDesc1);
			if (!aItemDesc2.empty())
				WriteCenteredLine(g, 325, aItemDesc2);

			g->SetColor(Color(255, 255, 100));
			if (mPetId == PET_PRESTO)
			{
				WriteWordWrapped(g, Rect(180, 373, 284, 0), "You have been awarded 5000 shells\nfor defeating the Final Boss!", -1, 0);
			}
			else
			{
				WriteCenteredLine(g, 390, "Your game has been saved.");
			}
		}
	}
}

void Sexy::HatchScreen::DrawOverlay(Graphics* g)
{
	g->DrawImage(IMAGE_HATCHREFLECTION, 240, 60);
	if (m0x94 > 139 && m0x94 < 200)
	{
		int aShardValues[16] = { -10, -15, 3, -13, 10, 5, 14, 12, 1, -11, -14, 12, -15, 11, 13, -1 };

		for (int i = 0; i < 8; i++)
		{
			int anOffset = m0x94 - 140;
			g->DrawImageCel(IMAGE_EGGSHARDS, aShardValues[i + 8] * anOffset + 268, aShardValues[i] * anOffset + 70, i);
		}
	}
}

void Sexy::HatchScreen::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

void Sexy::HatchScreen::ButtonDepress(int theId)
{
	if (mApp != NULL && mApp->mAPController != NULL)
		mApp->mAPController->ClearLastCheck();

	if (theId == 99)
	{
		mApp->RemoveHatchScreen();
		if (mApp->mGameMode == GAMEMODE_TIME_TRIAL || mApp->mGameMode == GAMEMODE_CHALLENGE)
			mApp->LeaveGameBoard();
		else if (mApp->mCurrentProfile->mTank != 5 || mApp->mCurrentProfile->mLevel != 2)
			mApp->SwitchToBoard(false, false);
		else
			mApp->SwitchToInterludeScreen();
	}
	else if (theId == 100)
	{
		mApp->RemoveHatchScreen();
		if (mApp->mCurrentProfile->mTank == 5 && mApp->mCurrentProfile->mLevel == 2)
			mApp->SwitchToBoard(false, false);
		else
			mApp->SwitchToGameSelector();
	}
}

void Sexy::HatchScreenOverlay::Draw(Graphics* g)
{
	mScreen->DrawOverlay(g);
}
