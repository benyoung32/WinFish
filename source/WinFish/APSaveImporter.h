#ifndef __APSAVEIMPORTER_H__
#define __APSAVEIMPORTER_H__

#include <cstdint>
#include <string>
#include <vector>

namespace Sexy
{
	struct APArchiveContents
	{
		std::string mArchipelagoJson;
		std::string mSlotDataJson;
		std::string mSlotName;
		
		int mPlayerId;
		int mStartingTank;

		APArchiveContents() :
			mPlayerId(0),
			mStartingTank(1)
		{
		}
	};

	class APSaveImporter
	{
	public:
		// Reads and extracts archipelago.json and slot_data.json
		// from a .apinsani / .zip archive into memory, and parses slot metadata.
		static bool ExtractArchive(const std::string& theZipFilePath, APArchiveContents& theOutContents, std::string& theOutError);
	};
}

#endif // __APSAVEIMPORTER_H__

