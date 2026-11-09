#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/DialogButton.h>

#include "TankScreen.h"
#include "WinFishApp.h"
#include "WinFishCommon.h"
#include "ProfileMgr.h"
#include "Res.h"
#include "APGameController.h"

Sexy::TankScreen::TankScreen(WinFishApp* theApp)
{
	for (int i = 0; i < 8; i+=2)
	{
		m0xa8[i] = 0;
		m0xa8[i + 1] = 0;
	}
	mApp = theApp;
	mX = 0;
	mY = 0;
	mWidth = theApp->mWidth;
	mHeight = theApp->mHeight;
	mMenuButton = MakeDialogButton2(99, this, "Menu", IMAGE_MAINBUTTON);
	mMenuButton->Resize(525, 4, 80, mMenuButton->mHeight);
	mStoriesButton = MakeDialogButton(4, this, "Stories", FONT_JUNGLEFEVER12OUTLINE);
	mStoriesButton->Resize(245, 247, 150, mStoriesButton->mHeight);
	mStoriesButton->SetColor(0, Color(255, 240, 0));
	if (mApp->mCurrentProfile == NULL || mApp->mCurrentProfile->m0x7c == 0 || mApp->mGameMode != GAMEMODE_CHALLENGE)
		mStoriesButton->mVisible = false;
	m0xa8[0] = 70;
	m0xa8[1] = 71;
	m0xa8[2] = 350;
	m0xa8[3] = 71;
	m0xa8[4] = m0xa8[0];
	m0xa8[6] = m0xa8[2];
	m0xa8[5] = mStoriesButton->mVisible ? 284 : 264;
	m0xa8[7] = mStoriesButton->mVisible ? 284 : 264;

	for (int i = 0; i < 4; i++)
	{
		mTankButtons[i] = MakeDialogButton2(i, this, StrFormat("Tank %d", i + 1), IMAGE_MAINBUTTON);
		mTankButtons[i]->SetFont(FONT_JUNGLEFEVER12OUTLINE);
		mTankButtons[i]->Resize(m0xa8[i * 2] + 14, m0xa8[i * 2 + 1] + 134, 193, mTankButtons[i]->mHeight);
	}

	mCyraxButton = MakeDialogButton2(BUTTON_CYRAX, this, "Cyrax", IMAGE_MAINBUTTON);
	mCyraxButton->SetFont(FONT_JUNGLEFEVER12OUTLINE);
	mCyraxButton->Resize((mWidth - 193) / 2, 442, 193, mCyraxButton->mHeight);
	mCyraxButton->SetColor(0, Color(255, 240, 0));
	mCyraxButton->mVisible = false;
}

Sexy::TankScreen::~TankScreen()
{
	for (int i = 0; i < 4; i++)
		if (mTankButtons[i])
			delete mTankButtons[i];
	if (mMenuButton)
		delete mMenuButton;
	if (mStoriesButton)
		delete mStoriesButton;
	if (mCyraxButton)
		delete mCyraxButton;
}

void Sexy::TankScreen::UpdateButtonStates()
{
	for (int i = 0; i < 4; i++)
	{
		bool anAPActive = (mApp->mAPController != NULL && mApp->mCurrentProfile != NULL && mApp->mCurrentProfile->HasAPConnection());
		if (anAPActive)
		{
			if (mApp->mAPController->IsTankUnlocked(LevelData(i + 1)))
			{
				mTankButtons[i]->SetDisabled(false);
				mTankButtons[i]->mComponentImage = IMAGE_MAINBUTTON;
				mTankButtons[i]->mFrameNoDraw = false;
				mTankButtons[i]->SetColor(0, Color(255, 240, 0));
			}
			else
			{
				mTankButtons[i]->SetDisabled(true);
				mTankButtons[i]->mComponentImage = nullptr;
				mTankButtons[i]->mFrameNoDraw = true;
				mTankButtons[i]->SetColor(0, Color::White);
			}
		}
		else if (mApp->mGameMode == GAMEMODE_TIME_TRIAL)
		{
			if (mApp->mCurrentProfile != NULL && (mApp->mCurrentProfile->mTank >= i + 2 || mApp->mCurrentProfile->mFinishedGame))
			{
				mTankButtons[i]->SetDisabled(false);
				mTankButtons[i]->mComponentImage = IMAGE_MAINBUTTON;
				mTankButtons[i]->mFrameNoDraw = false;
				mTankButtons[i]->SetColor(0, Color(255, 240, 0));
			}
			else
			{
				mTankButtons[i]->SetDisabled(true);
				mTankButtons[i]->mComponentImage = nullptr;
				mTankButtons[i]->mFrameNoDraw = true;
				mTankButtons[i]->SetColor(0, Color::White);
			}
		}
		else if (mApp->mGameMode == GAMEMODE_CHALLENGE)
		{
			if (i > 0 && (mApp->mCurrentProfile == NULL || !mApp->mCurrentProfile->m0x78[i-1]))
			{
				mTankButtons[i]->SetDisabled(true);
				mTankButtons[i]->mComponentImage = nullptr;
				mTankButtons[i]->mFrameNoDraw = true;
				mTankButtons[i]->SetColor(0, Color::White);
			}
			else
			{
				mTankButtons[i]->SetDisabled(false);
				mTankButtons[i]->mComponentImage = IMAGE_MAINBUTTON;
				mTankButtons[i]->mFrameNoDraw = false;
				mTankButtons[i]->SetColor(0, Color(255, 240, 0));
			}
		}
		else // GAMEMODE_ADVENTURE (non-AP fallback)
		{
			if (mApp->mCurrentProfile != NULL && (mApp->mCurrentProfile->mTank >= i + 1 || mApp->mCurrentProfile->mFinishedGame))
			{
				mTankButtons[i]->SetDisabled(false);
				mTankButtons[i]->mComponentImage = IMAGE_MAINBUTTON;
				mTankButtons[i]->mFrameNoDraw = false;
				mTankButtons[i]->SetColor(0, Color(255, 240, 0));
			}
			else
			{
				mTankButtons[i]->SetDisabled(true);
				mTankButtons[i]->mComponentImage = nullptr;
				mTankButtons[i]->mFrameNoDraw = true;
				mTankButtons[i]->SetColor(0, Color::White);
			}
		}
	}

	if (mApp->mGameMode == GAMEMODE_ADVENTURE)
	{
		mCyraxButton->mVisible = true;
		if (AreAllTanksCompleted())
		{
			mCyraxButton->SetDisabled(false);
			mCyraxButton->mComponentImage = IMAGE_MAINBUTTON;
			mCyraxButton->mFrameNoDraw = false;
			mCyraxButton->SetColor(0, Color(255, 240, 0));
		}
		else
		{
			mCyraxButton->SetDisabled(true);
			mCyraxButton->mComponentImage = IMAGE_MAINBUTTON;
			mCyraxButton->mFrameNoDraw = false;
			mCyraxButton->SetColor(0, Color(160, 160, 160));
		}
	}
	else
	{
		mCyraxButton->mVisible = false;
		mCyraxButton->SetDisabled(true);
	}
}

void Sexy::TankScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMenuButton);
	UpdateButtonStates();
	for (int i = 0; i < 4; i++)
	{
		theWidgetManager->AddWidget(mTankButtons[i]);
	}

	theWidgetManager->AddWidget(mStoriesButton);
	theWidgetManager->AddWidget(mCyraxButton);
}

void Sexy::TankScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMenuButton);
	for (int i = 0; i < 4; i++)
		theWidgetManager->RemoveWidget(mTankButtons[i]);
	theWidgetManager->RemoveWidget(mStoriesButton);
	theWidgetManager->RemoveWidget(mCyraxButton);
}

void Sexy::TankScreen::Update()
{
	UpdateButtonStates();
	MarkDirty();
}

void Sexy::TankScreen::Draw(Graphics* g)
{
	g->DrawImageBox(Rect(-5, -5, 650, 490), IMAGE_SCREENBACK);
	DrawHorzCrease(g, IMAGE_HORZCREASE, 13, 130, 614);
	DrawHorzCrease(g, IMAGE_HORZCREASE, 13, mStoriesButton->mVisible ? 350 : 320, 614);
	DrawVertCrease(g, IMAGE_VERTCREASE, 180, 100, 369);
	DrawVertCrease(g, IMAGE_VERTCREASE, 460, 100, 369);
	DrawBolts(g, 50, 110, true, true);
	DrawBolts(g, 30, 150, true, true);
	DrawBolts(g, 320, 100, false, true);
	DrawBolts(g, 320, 158, false, true);
	g->DrawImageBox(Rect(20, 0, 600, IMAGE_SCREENTITLE->mHeight), IMAGE_SCREENTITLE);
	g->DrawImageBox(Rect(mMenuButton->mX - 1, mMenuButton->mY - 1, mMenuButton->mWidth + 2, IMAGE_SCREENTITLEHOLE->mHeight), IMAGE_SCREENTITLEHOLE);

	g->SetFont(FONT_JUNGLEFEVER17OUTLINE);
	g->SetColor(Color(255, 200, 0));
	SexyString aStr;
	if(mApp->mGameMode == GAMEMODE_TIME_TRIAL)
		aStr = "Time Trial";
	else if(mApp->mGameMode == GAMEMODE_CHALLENGE)
		aStr = "Challenge";
	else
		aStr = "Choose a Tank";
	WriteCenteredLine(g, 25, aStr);
	
	g->SetColor(Color::White);
	g->SetFont(FONT_JUNGLEFEVER10OUTLINE);
	if (mApp->mGameMode == GAMEMODE_TIME_TRIAL)
		aStr = "How much money can you earn before time runs out?";
	else if(mApp->mGameMode == GAMEMODE_CHALLENGE)
		aStr = "Can you fend off the increasingly hungry aliens?";
	else
		aStr = "";

	if(!aStr.empty())
		WriteCenteredLine(g, 60, aStr);

	for (int i = 0; i < 4; i++)
	{
		g->DrawImage(IMAGE_TANKCHOOSER, m0xa8[i * 2], m0xa8[i * 2 + 1]);
		if (!mTankButtons[i]->mDisabled)
		{
			int anId = i;
			if (i > 1)
				anId = i + 1;

			g->DrawImage(GetImageById(anId + IMAGE_TANKBACKDROP1_ID), m0xa8[i * 2] + 16, m0xa8[i * 2 + 1] + 10);
		}
		else
		{
			Rect aDrawStrRect = Rect(m0xa8[i * 2] + 25, m0xa8[i * 2 + 1] + 43, 170, 200);
			g->SetFont(FONT_JUNGLEFEVER10OUTLINE);
			g->SetColor(Color::White);
			bool anAPActive = (mApp->mAPController != NULL && mApp->mCurrentProfile != NULL && mApp->mCurrentProfile->HasAPConnection());
			if (anAPActive)
				WriteWordWrapped(g, aDrawStrRect, StrFormat("Unlock Tank %d\nin Archipelago\nto Play", i + 1), -1, 0);
			else if (mApp->mGameMode == GAMEMODE_TIME_TRIAL)
				WriteWordWrapped(g, aDrawStrRect, StrFormat("Complete Tank %d\nin Adventure Mode\nto Unlock this Tank", i + 1), -1, 0);
			else if(mApp->mGameMode == GAMEMODE_CHALLENGE)
				WriteWordWrapped(g, aDrawStrRect, StrFormat("Complete Tank %d\nin Challenge Mode\nto Unlock this Tank", i), -1, 0);
			else if (mApp->mGameMode == GAMEMODE_ADVENTURE)
				WriteWordWrapped(g, aDrawStrRect, StrFormat("Complete Tank %d\nin Adventure Mode\nto Unlock this Tank", i), -1, 0);
		}
	}
}

void Sexy::TankScreen::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

void Sexy::TankScreen::ButtonDepress(int theId)
{
	if (theId == 99)
	{
		mApp->RemoveTankScreen();
		mApp->SwitchToGameSelector();
	}
	else if (theId < 4)
	{
		mApp->RemoveTankScreen();
		int aSelectedTank = theId + 1;
		mApp->mTankGameModeChoose = aSelectedTank;

		if (mApp->mGameMode == GAMEMODE_ADVENTURE && mApp->mCurrentProfile != NULL)
		{
			if (mApp->mCurrentProfile->mTank >= 1 && mApp->mCurrentProfile->mTank <= 4)
			{
				mApp->mCurrentProfile->mAPTankLevels[mApp->mCurrentProfile->mTank - 1] = mApp->mCurrentProfile->mLevel;
			}
			mApp->mCurrentProfile->mTank = aSelectedTank;
			int aLevel = mApp->mCurrentProfile->mAPTankLevels[aSelectedTank - 1];
			if (aLevel < 1 || aLevel > 5)
				aLevel = 1;
			mApp->mCurrentProfile->mLevel = aLevel;
			mApp->SaveCurrentUserData();
		}

		bool aLoadBoard = (mApp->mGameMode != GAMEMODE_ADVENTURE);
		mApp->SwitchToBoard(aLoadBoard, true);
	}
	else if (theId == 4)
	{
		mApp->RemoveTankScreen();
		mApp->SwitchToStoryScreen(-1);
	}
	else if (theId == BUTTON_CYRAX)
	{
		mApp->RemoveTankScreen();
		int aSelectedTank = 5;
		mApp->mTankGameModeChoose = aSelectedTank;

		if (mApp->mGameMode == GAMEMODE_ADVENTURE && mApp->mCurrentProfile != NULL)
		{
			if (mApp->mCurrentProfile->mTank >= 1 && mApp->mCurrentProfile->mTank <= 4)
			{
				mApp->mCurrentProfile->mAPTankLevels[mApp->mCurrentProfile->mTank - 1] = mApp->mCurrentProfile->mLevel;
			}
			mApp->mCurrentProfile->mTank = 5;
			mApp->mCurrentProfile->mLevel = 1;
			mApp->SaveCurrentUserData();
		}

		mApp->SwitchToBoard(false, true);
	}
}

bool Sexy::TankScreen::AreAllTanksCompleted() const
{
	if (mApp != NULL && mApp->mAPController != NULL)
		return mApp->mAPController->AreAllTanksCompleted();

	if (mApp == NULL || mApp->mAPBridge == NULL)
		return false;

	return mApp->mAPBridge->IsLocationChecked(static_cast<int>(AP_LOCATION_TANK_1_COMPLETE)) &&
	       mApp->mAPBridge->IsLocationChecked(static_cast<int>(AP_LOCATION_TANK_2_COMPLETE)) &&
	       mApp->mAPBridge->IsLocationChecked(static_cast<int>(AP_LOCATION_TANK_3_COMPLETE)) &&
	       mApp->mAPBridge->IsLocationChecked(static_cast<int>(AP_LOCATION_TANK_4_COMPLETE));
}

void Sexy::TankScreen::DrawBolts(Graphics* g, int xOffset, int yOffset, bool drawRight, bool drawBottom)
{
	g->DrawImage(IMAGE_BOLT, xOffset, yOffset);

	if (drawRight)
		g->DrawImage(IMAGE_BOLT, 640 - xOffset, yOffset);

	if (drawBottom)
		g->DrawImage(IMAGE_BOLT, xOffset, 480 - yOffset);

	if (drawRight && drawBottom)
		g->DrawImage(IMAGE_BOLT, 640 - xOffset, 480 - yOffset);
}
