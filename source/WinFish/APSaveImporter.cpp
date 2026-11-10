#include "APSaveImporter.h"

#include <cstdio>
#include <cstring>
#include <cctype>
#include <fstream>
#include <nlohmann/json.hpp>

#include "apclientpp/zlib/include/zlib.h"

namespace Sexy
{
#pragma pack(push, 1)
	struct ZipLocalHeader
	{
		uint32_t mSignature;
		uint16_t mVersionNeeded, mFlags, mCompressionMethod, mModTime, mModDate;
		uint32_t mCrc32, mCompressedSize, mUncompressedSize;
		uint16_t mFilenameLength, mExtraFieldLength;
	};

	struct ZipCentralDirHeader
	{
		uint32_t mSignature;
		uint16_t mVersionMadeBy, mVersionNeeded, mFlags, mCompressionMethod, mModTime, mModDate;
		uint32_t mCrc32, mCompressedSize, mUncompressedSize;
		uint16_t mFilenameLength, mExtraFieldLength, mCommentLength, mDiskNumberStart, mInternalAttrs;
		uint32_t mExternalAttrs, mLocalHeaderOffset;
	};

	struct ZipEOCD
	{
		uint32_t mSignature;
		uint16_t mDiskNumber, mStartDiskNumber, mEntriesOnDisk, mTotalEntries;
		uint32_t mCentralDirSize, mCentralDirOffset;
		uint16_t mCommentLength;
	};
#pragma pack(pop)

	bool APSaveImporter::ExtractArchive(const std::string& theZipFilePath, APArchiveContents& theOutContents, std::string& theOutError)
	{
		theOutContents = APArchiveContents();
		theOutError.clear();

		std::ifstream aFile(theZipFilePath, std::ios::binary | std::ios::ate);
		if (!aFile.is_open())
		{
			theOutError = "Could not open file: " + theZipFilePath;
			return false;
		}

		size_t aFileSize = static_cast<size_t>(aFile.tellg());
		if (aFileSize < sizeof(ZipEOCD))
		{
			theOutError = "File is too small to be a valid ZIP archive.";
			return false;
		}

		aFile.seekg(0, std::ios::beg);
		std::vector<uint8_t> aBuffer(aFileSize);
		if (!aFile.read(reinterpret_cast<char*>(aBuffer.data()), aFileSize))
		{
			theOutError = "Failed to read file data.";
			return false;
		}

		// Locate End of Central Directory (EOCD) signature backwards from end
		size_t aMaxSearch = (aBuffer.size() > 65535 + sizeof(ZipEOCD)) ? 65535 + sizeof(ZipEOCD) : aBuffer.size();
		size_t anEocdOffset = static_cast<size_t>(-1);
		for (size_t i = aBuffer.size() - sizeof(ZipEOCD); i >= aBuffer.size() - aMaxSearch; --i)
		{
			if (aBuffer[i] == 0x50 && aBuffer[i + 1] == 0x4b && aBuffer[i + 2] == 0x05 && aBuffer[i + 3] == 0x06)
			{
				anEocdOffset = i;
				break;
			}
			if (i == 0) break;
		}

		const ZipEOCD* anEocd = reinterpret_cast<const ZipEOCD*>(&aBuffer[anEocdOffset]);

		size_t aCurr = anEocd->mCentralDirOffset;
		for (uint16_t i = 0; i < anEocd->mTotalEntries && aCurr + sizeof(ZipCentralDirHeader) <= aBuffer.size(); ++i)
		{
			const ZipCentralDirHeader* aCd = reinterpret_cast<const ZipCentralDirHeader*>(&aBuffer[aCurr]);
			if (aCd->mSignature != 0x02014b50)
				break;

			size_t aNextEntry = aCurr + sizeof(ZipCentralDirHeader) + aCd->mFilenameLength + aCd->mExtraFieldLength + aCd->mCommentLength;
			if (aNextEntry > aBuffer.size())
				break;

			std::string aName(reinterpret_cast<const char*>(&aBuffer[aCurr + sizeof(ZipCentralDirHeader)]), aCd->mFilenameLength);
			for (char& c : aName)
				c = (c == '\\') ? '/' : static_cast<char>(tolower(static_cast<unsigned char>(c)));

			size_t aSlash = aName.find_last_of('/');
			std::string aBase = (aSlash != std::string::npos) ? aName.substr(aSlash + 1) : aName;

			bool isUsersDat = (aBase == "users.dat");
			bool isUser1Dat = (aBase == "user1.dat");
			bool isAnyUserDat = !isUsersDat && (aBase.find("user") != std::string::npos && aBase.size() >= 4 && aBase.rfind(".dat") == aBase.size() - 4);
			bool isArchipelagoJson = (aBase == "archipelago.json");
			bool isSlotDataJson = (aBase == "slot_data.json");

			if (isUsersDat || isUser1Dat || isAnyUserDat || isArchipelagoJson || isSlotDataJson)
			{
				size_t aLocOffset = aCd->mLocalHeaderOffset;
				if (aLocOffset + sizeof(ZipLocalHeader) <= aBuffer.size())
				{
					const ZipLocalHeader* aLoc = reinterpret_cast<const ZipLocalHeader*>(&aBuffer[aLocOffset]);
					size_t aDataStart = aLocOffset + sizeof(ZipLocalHeader) + aLoc->mFilenameLength + aLoc->mExtraFieldLength;
					if (aLoc->mSignature == 0x04034b50 && aDataStart + aCd->mCompressedSize <= aBuffer.size())
					{
						std::vector<uint8_t> anUncomp(aCd->mUncompressedSize);
						bool anOk = false;

						if (aCd->mCompressionMethod == 0 && aCd->mCompressedSize == aCd->mUncompressedSize)
						{
							if (!anUncomp.empty())
								memcpy(anUncomp.data(), &aBuffer[aDataStart], anUncomp.size());
							anOk = true;
						}
						else if (aCd->mCompressionMethod == 8)
						{
							z_stream aStrm = {};
							aStrm.next_in = const_cast<Bytef*>(&aBuffer[aDataStart]);
							aStrm.avail_in = aCd->mCompressedSize;
							aStrm.next_out = anUncomp.data();
							aStrm.avail_out = aCd->mUncompressedSize;
							if (inflateInit2(&aStrm, -MAX_WBITS) == Z_OK)
							{
								int aRet = inflate(&aStrm, Z_FINISH);
								inflateEnd(&aStrm);
								anOk = (aRet == Z_STREAM_END || aRet == Z_OK || (aRet == Z_BUF_ERROR && aStrm.total_out == aCd->mUncompressedSize));
							}
						}

						if (anOk)
						{
							if (isUsersDat) { theOutContents.mUsersDat = anUncomp; }
							if (isUser1Dat || (theOutContents.mUser1Dat.empty() && isAnyUserDat)) { theOutContents.mUser1Dat = anUncomp; }
							if (isArchipelagoJson) { theOutContents.mArchipelagoJson.assign(reinterpret_cast<char*>(anUncomp.data()), anUncomp.size()); }
							if (isSlotDataJson) { theOutContents.mSlotDataJson.assign(reinterpret_cast<char*>(anUncomp.data()), anUncomp.size()); }
						}
					}
				}
			}

			aCurr = aNextEntry;
		}

		if (theOutContents.mUsersDat.empty())
		{
			theOutError = "The archive is missing users.dat.";
			return false;
		}
		if (theOutContents.mUser1Dat.empty())
		{
			theOutError = "The archive is missing user save data (user1.dat).";
			return false;
		}
		if (theOutContents.mArchipelagoJson.empty())
		{
			theOutError = "The archive is missing archipelago.json.";
			return false;
		}
		if (theOutContents.mSlotDataJson.empty())
		{
			theOutError = "The archive is missing slot_data.json.";
			return false;
		}

		// Parse archipelago.json
		auto j = nlohmann::json::parse(theOutContents.mArchipelagoJson, nullptr, false);
		if (j.is_object())
		{
			for (const char* k : { "player_name", "slot_name", "slot", "name" })
				if (j.contains(k) && j[k].is_string() && !(theOutContents.mSlotName = j[k]).empty()) break;

			for (const char* k : { "player", "player_id", "id" })
				if (j.contains(k) && j[k].is_number()) { theOutContents.mPlayerId = j[k]; break; }

			for (const char* k : { "server", "host", "url", "address" })
				if (j.contains(k) && j[k].is_string() && !(theOutContents.mServer = j[k]).empty()) break;

			if (j.contains("port") && !theOutContents.mServer.empty() && theOutContents.mServer.find(':') == std::string::npos)
				theOutContents.mServer += ":" + j["port"].dump();

			theOutContents.mPassword = j.value("password", "");

			for (const char* k : { "seed_name", "seed" })
				if (j.contains(k) && j[k].is_string() && !(theOutContents.mSeed = j[k]).empty()) break;
		}

		// Parse slot_data.json
		auto aSlotJson = nlohmann::json::parse(theOutContents.mSlotDataJson, nullptr, false);
		std::string aTank = aSlotJson.value("starting_tank_unlock", "");
		for (int t = 2; t <= 4; ++t)
			if (aTank.find("Tank " + std::to_string(t)) != std::string::npos || aTank == std::to_string(t))
				theOutContents.mStartingTank = t;

		theOutContents.mServer = "archipelago.gg:";

		return true;
	}
}
