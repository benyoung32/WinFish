#ifndef __APSAVEIMPORTER_H__
#define __APSAVEIMPORTER_H__

#include <cstdint>
#include <string>
#include <vector>

namespace Sexy
{
	struct APArchiveContents
	{
		std::vector<uint8_t> mUsersDat;
		std::vector<uint8_t> mUser1Dat;
		std::string mArchipelagoJson;
		std::string mSlotDataJson;

		bool mHasUsersDat;
		bool mHasUser1Dat;
		bool mHasArchipelagoJson;
		bool mHasSlotDataJson;

		std::string mSlotName;
		std::string mServer;
		std::string mPassword;
		std::string mSeed;
		int mPlayerId;
		int mStartingTank;

		APArchiveContents() :
			mHasUsersDat(false),
			mHasUser1Dat(false),
			mHasArchipelagoJson(false),
			mHasSlotDataJson(false),
			mPlayerId(0),
			mStartingTank(1)
		{
		}
	};

	class APSaveImporter
	{
	public:
		// Reads and extracts users.dat, user1.dat, archipelago.json, and slot_data.json
		// from a .apinsani / .zip archive into memory, and parses slot metadata.
		static bool ExtractArchive(const std::string& theZipFilePath, APArchiveContents& theOutContents, std::string& theOutError);
	};
}

#endif // __APSAVEIMPORTER_H__

