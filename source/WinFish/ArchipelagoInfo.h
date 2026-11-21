#ifndef __ARCHIPELAGOINFO_H__
#define __ARCHIPELAGOINFO_H__

#include <SexyAppFramework/Widget.h>
#include <SexyAppFramework/ButtonListener.h>

namespace Sexy
{
	class WinFishApp;
	class DialogButton;
	class BubbleMgr;

	extern int gArchipelagoInfoTab;

	class ArchipelagoInfo : public Widget, public ButtonListener
	{
	public:
		WinFishApp*		mApp;
		BubbleMgr*		mBubbleMgr;
		DialogButton*	mMenuButton;
		DialogButton*	mNextButton;
		DialogButton*	mPreviousButton;
		int				mPageIdx;

	public:
		ArchipelagoInfo(WinFishApp* theApp);
		virtual ~ArchipelagoInfo();

		virtual void	AddedToManager(WidgetManager* theWidgetManager);
		virtual void	RemovedFromManager(WidgetManager* theWidgetManager);
		virtual void	Update();
		virtual void	Draw(Graphics* g);
		virtual void	KeyDown(KeyCode theKey);

		virtual void	ButtonPress(int theId);
		virtual void	ButtonDepress(int theId);

	private:
		void			DrawReceivedItemsPage(Graphics* g);
		void			DrawTankPage(Graphics* g, int theTank);
		void			DrawItemRow(Graphics* g, const Rect& theRect, int theRowIndex, const SexyString& theName, const SexyString& theValueStr, const SexyString& theDesc);
		void			DrawTankRow(Graphics* g, const Rect& theRect, int theRowIndex, const SexyString& theName, const SexyString& theValueStr, bool isComplete, bool isCheckmarkRow, const SexyString& theDesc);
	};
}

#endif // __ARCHIPELAGOINFO_H__
