#include "APSaveImporter.h"

#include <cstdio>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <fstream>

#include "apclientpp/zlib/include/zlib.h"

namespace Sexy
{
	static bool ExtractJsonString(const std::string& theJson, const std::string& theKey, std::string& theOutValue)
	{
		std::string aTarget = "\"" + theKey + "\"";
		size_t aKeyPos = theJson.find(aTarget);
		if (aKeyPos == std::string::npos)
			return false;

		size_t aColon = theJson.find(':', aKeyPos + aTarget.size());
		if (aColon == std::string::npos)
			return false;

		size_t aStart = theJson.find_first_not_of(" \t\r\n", aColon + 1);
		if (aStart == std::string::npos)
			return false;

		if (theJson[aStart] == '\"')
		{
			size_t aEnd = theJson.find('\"', aStart + 1);
			if (aEnd != std::string::npos)
			{
				theOutValue = theJson.substr(aStart + 1, aEnd - aStart - 1);
				return true;
			}
		}
		else if (theJson.compare(aStart, 4, "null") == 0)
		{
			theOutValue.clear();
			return true;
		}
		return false;
	}

	static bool ExtractJsonInt(const std::string& theJson, const std::string& theKey, int& theOutValue)
	{
		std::string aTarget = "\"" + theKey + "\"";
		size_t aKeyPos = theJson.find(aTarget);
		if (aKeyPos == std::string::npos)
			return false;

		size_t aColon = theJson.find(':', aKeyPos + aTarget.size());
		if (aColon == std::string::npos)
			return false;

		size_t aStart = theJson.find_first_not_of(" \t\r\n", aColon + 1);
		if (aStart == std::string::npos)
			return false;

		if (isdigit(static_cast<unsigned char>(theJson[aStart])) || theJson[aStart] == '-')
		{
			theOutValue = atoi(&theJson[aStart]);
			return true;
		}
		return false;
	}
#pragma pack(push, 1)
	struct ZipLocalHeader
	{
		uint32_t mSignature;			// 0x04034b50
		uint16_t mVersionNeeded;
		uint16_t mFlags;
		uint16_t mCompressionMethod;	// 0 = store, 8 = deflate
		uint16_t mModTime;
		uint16_t mModDate;
		uint32_t mCrc32;
		uint32_t mCompressedSize;
		uint32_t mUncompressedSize;
		uint16_t mFilenameLength;
		uint16_t mExtraFieldLength;
	};

	struct ZipCentralDirHeader
	{
		uint32_t mSignature;			// 0x02014b50
		uint16_t mVersionMadeBy;
		uint16_t mVersionNeeded;
		uint16_t mFlags;
		uint16_t mCompressionMethod;
		uint16_t mModTime;
		uint16_t mModDate;
		uint32_t mCrc32;
		uint32_t mCompressedSize;
		uint32_t mUncompressedSize;
		uint16_t mFilenameLength;
		uint16_t mExtraFieldLength;
		uint16_t mCommentLength;
		uint16_t mDiskNumberStart;
		uint16_t mInternalAttrs;
		uint32_t mExternalAttrs;
		uint32_t mLocalHeaderOffset;
	};

	struct ZipEOCD
	{
		uint32_t mSignature;			// 0x06054b50
		uint16_t mDiskNumber;
		uint16_t mStartDiskNumber;
		uint16_t mEntriesOnDisk;
		uint16_t mTotalEntries;
		uint32_t mCentralDirSize;
		uint32_t mCentralDirOffset;
		uint16_t mCommentLength;
	};
#pragma pack(pop)

	bool APSaveImporter::ExtractArchive(const std::string& theZipFilePath, APArchiveContents& theOutContents, std::string& theOutError)
	{
		theOutContents.mHasUsersDat = false;
		theOutContents.mHasUser1Dat = false;
		theOutContents.mHasArchipelagoJson = false;
		theOutContents.mHasSlotDataJson = false;
		theOutContents.mUsersDat.clear();
		theOutContents.mUser1Dat.clear();
		theOutContents.mArchipelagoJson.clear();
		theOutContents.mSlotDataJson.clear();
		theOutContents.mSlotName.clear();
		theOutContents.mServer.clear();
		theOutContents.mPlayerId = 0;
		theOutError.clear();

		std::ifstream aFile(theZipFilePath, std::ios::binary | std::ios::ate);
		if (!aFile.is_open())
		{
			theOutError = "Could not open file: " + theZipFilePath;
			return false;
		}

		std::streamsize aFileSize = aFile.tellg();
		if (aFileSize < (std::streamsize)sizeof(ZipEOCD))
		{
			theOutError = "File is too small to be a valid ZIP archive.";
			return false;
		}

		aFile.seekg(0, std::ios::beg);
		std::vector<uint8_t> aBuffer(static_cast<size_t>(aFileSize));
		if (!aFile.read(reinterpret_cast<char*>(aBuffer.data()), aFileSize))
		{
			theOutError = "Failed to read file data.";
			return false;
		}

		// Locate End of Central Directory (EOCD) signature backwards from end
		size_t aMaxSearch = (aBuffer.size() > 65535 + sizeof(ZipEOCD)) ? 65535 + sizeof(ZipEOCD) : aBuffer.size();
		size_t aSearchStart = aBuffer.size() - sizeof(ZipEOCD);
		size_t aSearchEnd = aBuffer.size() - aMaxSearch;
		size_t anEocdOffset = static_cast<size_t>(-1);

		for (size_t i = aSearchStart;; --i)
		{
			if (aBuffer[i] == 0x50 && aBuffer[i + 1] == 0x4b && aBuffer[i + 2] == 0x05 && aBuffer[i + 3] == 0x06)
			{
				anEocdOffset = i;
				break;
			}
			if (i == aSearchEnd || i == 0)
				break;
		}

		if (anEocdOffset == static_cast<size_t>(-1) || anEocdOffset + sizeof(ZipEOCD) > aBuffer.size())
		{
			theOutError = "Missing ZIP End of Central Directory record.";
			return false;
		}

		const ZipEOCD* anEocd = reinterpret_cast<const ZipEOCD*>(&aBuffer[anEocdOffset]);
		uint32_t aCdOffset = anEocd->mCentralDirOffset;
		uint16_t anEntryCount = anEocd->mTotalEntries;

		if (aCdOffset >= aBuffer.size())
		{
			theOutError = "Corrupted ZIP Central Directory offset.";
			return false;
		}

		size_t aCurr = aCdOffset;
		for (uint16_t i = 0; i < anEntryCount && aCurr + sizeof(ZipCentralDirHeader) <= aBuffer.size(); ++i)
		{
			const ZipCentralDirHeader* aCd = reinterpret_cast<const ZipCentralDirHeader*>(&aBuffer[aCurr]);
			if (aCd->mSignature != 0x02014b50)
				break;

			size_t aNameLen = aCd->mFilenameLength;
			size_t anExtraLen = aCd->mExtraFieldLength;
			size_t aCommentLen = aCd->mCommentLength;
			size_t aNextEntry = aCurr + sizeof(ZipCentralDirHeader) + aNameLen + anExtraLen + aCommentLen;
			if (aNextEntry > aBuffer.size())
				break;

			std::string aFilename(reinterpret_cast<const char*>(&aBuffer[aCurr + sizeof(ZipCentralDirHeader)]), aNameLen);

			// Normalize filename to lowercase and forward slashes
			std::string aNormName = aFilename;
			for (size_t c = 0; c < aNormName.size(); ++c)
			{
				if (aNormName[c] == '\\')
					aNormName[c] = '/';
				aNormName[c] = static_cast<char>(tolower(static_cast<unsigned char>(aNormName[c])));
			}

			bool isUsersDat = (aNormName == "users.dat" || (aNormName.size() >= 10 && aNormName.compare(aNormName.size() - 10, 10, "/users.dat") == 0));
			bool isUser1Dat = (aNormName == "user1.dat" || (aNormName.size() >= 10 && aNormName.compare(aNormName.size() - 10, 10, "/user1.dat") == 0));
			bool isAnyUserDat = !isUsersDat && (aNormName.find("user") != std::string::npos && aNormName.size() >= 4 && aNormName.compare(aNormName.size() - 4, 4, ".dat") == 0);
			bool isArchipelagoJson = (aNormName == "archipelago.json" || (aNormName.size() >= 17 && aNormName.compare(aNormName.size() - 17, 17, "/archipelago.json") == 0));
			bool isSlotDataJson = (aNormName == "slot_data.json" || (aNormName.size() >= 15 && aNormName.compare(aNormName.size() - 15, 15, "/slot_data.json") == 0));

			if (isUsersDat || isUser1Dat || isAnyUserDat || isArchipelagoJson || isSlotDataJson)
			{
				size_t aLocOffset = aCd->mLocalHeaderOffset;
				if (aLocOffset + sizeof(ZipLocalHeader) <= aBuffer.size())
				{
					const ZipLocalHeader* aLoc = reinterpret_cast<const ZipLocalHeader*>(&aBuffer[aLocOffset]);
					if (aLoc->mSignature == 0x04034b50)
					{
						size_t aDataStart = aLocOffset + sizeof(ZipLocalHeader) + aLoc->mFilenameLength + aLoc->mExtraFieldLength;
						size_t aCompSize = aCd->mCompressedSize;
						size_t anUncompSize = aCd->mUncompressedSize;
						uint16_t aMethod = aCd->mCompressionMethod;

						if (aDataStart + aCompSize <= aBuffer.size())
						{
							std::vector<uint8_t> anUncompressed(anUncompSize);
							bool anOk = false;

							if (aMethod == 0) // Stored (no compression)
							{
								if (aCompSize == anUncompSize)
								{
									if (anUncompSize > 0)
										memcpy(anUncompressed.data(), &aBuffer[aDataStart], anUncompSize);
									anOk = true;
								}
							}
							else if (aMethod == 8) // Deflate
							{
								z_stream aStrm;
								memset(&aStrm, 0, sizeof(aStrm));
								aStrm.next_in = const_cast<Bytef*>(&aBuffer[aDataStart]);
								aStrm.avail_in = static_cast<uInt>(aCompSize);
								aStrm.next_out = reinterpret_cast<Bytef*>(anUncompressed.data());
								aStrm.avail_out = static_cast<uInt>(anUncompSize);

								// -MAX_WBITS tells zlib to decode raw deflate without zlib/gzip headers
								if (inflateInit2(&aStrm, -MAX_WBITS) == Z_OK)
								{
									int aRet = inflate(&aStrm, Z_FINISH);
									inflateEnd(&aStrm);
									anOk = (aRet == Z_STREAM_END || aRet == Z_OK);
								}
							}

							if (anOk)
							{
								if (isUsersDat)
								{
									theOutContents.mUsersDat = anUncompressed;
									theOutContents.mHasUsersDat = true;
								}
								if (isUser1Dat || (!theOutContents.mHasUser1Dat && isAnyUserDat))
								{
									theOutContents.mUser1Dat = anUncompressed;
									theOutContents.mHasUser1Dat = true;
								}
								if (isArchipelagoJson)
								{
									theOutContents.mArchipelagoJson.assign(reinterpret_cast<const char*>(anUncompressed.data()), anUncompressed.size());
									theOutContents.mHasArchipelagoJson = true;
								}
								if (isSlotDataJson)
								{
									theOutContents.mSlotDataJson.assign(reinterpret_cast<const char*>(anUncompressed.data()), anUncompressed.size());
									theOutContents.mHasSlotDataJson = true;
								}
							}
						}
					}
				}
			}

			aCurr = aNextEntry;
		}

		if (!theOutContents.mHasUser1Dat)
		{
			theOutError = "The file does not contain user save data (user1.dat).";
			return false;
		}

		// Parse archipelago.json if present
		if (theOutContents.mHasArchipelagoJson && !theOutContents.mArchipelagoJson.empty())
		{
			ExtractJsonString(theOutContents.mArchipelagoJson, "player_name", theOutContents.mSlotName);
			ExtractJsonInt(theOutContents.mArchipelagoJson, "player", theOutContents.mPlayerId);
			ExtractJsonString(theOutContents.mArchipelagoJson, "server", theOutContents.mServer);
		}

		// Parse slot_data.json if present (e.g. starting_tank_unlock)
		theOutContents.mStartingTank = 1;
		if (theOutContents.mHasSlotDataJson && !theOutContents.mSlotDataJson.empty())
		{
			std::string aStartingTankStr;
			if (ExtractJsonString(theOutContents.mSlotDataJson, "starting_tank_unlock", aStartingTankStr))
			{
				if (aStartingTankStr.find("Tank 2") != std::string::npos)
					theOutContents.mStartingTank = 2;
				else if (aStartingTankStr.find("Tank 3") != std::string::npos)
					theOutContents.mStartingTank = 3;
				else if (aStartingTankStr.find("Tank 4") != std::string::npos)
					theOutContents.mStartingTank = 4;
			}
		}

		// Fallback for slot name from users.dat if not found in archipelago.json
		if (theOutContents.mSlotName.empty() && theOutContents.mHasUsersDat && theOutContents.mUsersDat.size() >= 8)
		{
			const uint8_t* p = theOutContents.mUsersDat.data();
			uint16_t aNameLen = *reinterpret_cast<const uint16_t*>(p + 6);
			if (aNameLen > 0 && 8 + aNameLen <= theOutContents.mUsersDat.size())
			{
				theOutContents.mSlotName.assign(reinterpret_cast<const char*>(p + 8), aNameLen);
			}
		}

		// Fallback for slot name from archive filename (e.g. "P1_Player1_5BNvm3JeQtW6CrcD9ATMbA.apinsani")
		if (theOutContents.mSlotName.empty())
		{
			std::string aBase = theZipFilePath;
			size_t aSlash = aBase.find_last_of("/\\");
			if (aSlash != std::string::npos)
				aBase = aBase.substr(aSlash + 1);
			size_t aDot = aBase.rfind('.');
			if (aDot != std::string::npos)
				aBase = aBase.substr(0, aDot);

			if (aBase.size() > 3 && aBase[0] == 'P' && isdigit(static_cast<unsigned char>(aBase[1])))
			{
				size_t aFirstUnder = aBase.find('_');
				size_t aSecondUnder = (aFirstUnder != std::string::npos) ? aBase.find('_', aFirstUnder + 1) : std::string::npos;
				if (aFirstUnder != std::string::npos && aSecondUnder != std::string::npos && aSecondUnder > aFirstUnder + 1)
				{
					theOutContents.mSlotName = aBase.substr(aFirstUnder + 1, aSecondUnder - aFirstUnder - 1);
				}
			}
			if (theOutContents.mSlotName.empty())
				theOutContents.mSlotName = aBase;
		}

		// Prefill default server if none was in archipelago.json
		if (theOutContents.mServer.empty())
		{
			theOutContents.mServer = "archipelago.gg:";
		}

		return true;
	}
}

