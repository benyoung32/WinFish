#ifndef __TANKSCREEN_H__
#define __TANKSCREEN_H__

#include "SexyAppFramework/Widget.h"
#include "SexyAppFramework/ButtonListener.h"

namespace Sexy
{
	class WinFishApp;
	class DialogButton;

	class TankScreen : public Widget, public ButtonListener
	{
	public:
		enum
		{
			BUTTON_CYRAX = 5,
			BUTTON_BONUS = 6
		};

		WinFishApp* mApp;
		DialogButton* mMenuButton;
		DialogButton* mStoriesButton;
		DialogButton* mTankButtons[4];
		DialogButton* mCyraxButton;
		DialogButton* mBonusButton;
		int m0xa8[8];

	public:
		TankScreen(WinFishApp* theApp);
		virtual ~TankScreen();

		virtual void			AddedToManager(WidgetManager* theWidgetManager);
		virtual void			RemovedFromManager(WidgetManager* theWidgetManager);
		virtual void			Update();
		virtual void			Draw(Graphics* g);
		virtual void			KeyDown(KeyCode theKey);

		virtual void			ButtonPress(int theId);
		virtual void			ButtonDepress(int theId);

		void					DrawBolts(Graphics* g, int xOffset, int yOffset, bool drawRight, bool drawBottom);
		void					UpdateButtonStates();
		bool					AreAllTanksCompleted() const;
	};
}

#endif