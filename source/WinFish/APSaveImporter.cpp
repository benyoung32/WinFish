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

	static bool ExtractFile(const std::vector<uint8_t>& theBuffer, const ZipCentralDirHeader* theCd, std::vector<uint8_t>& theOut)
	{
		size_t aLocOffset = theCd->mLocalHeaderOffset;
		if (aLocOffset + sizeof(ZipLocalHeader) > theBuffer.size())
			return false;

		const ZipLocalHeader* aLoc = reinterpret_cast<const ZipLocalHeader*>(&theBuffer[aLocOffset]);
		size_t aDataStart = aLocOffset + sizeof(ZipLocalHeader) + aLoc->mFilenameLength + aLoc->mExtraFieldLength;
		if (aLoc->mSignature != 0x04034b50 || aDataStart + theCd->mCompressedSize > theBuffer.size())
			return false;

		theOut.resize(theCd->mUncompressedSize);

		if (theCd->mCompressionMethod == 0 && theCd->mCompressedSize == theCd->mUncompressedSize)
		{
			if (!theOut.empty())
				memcpy(theOut.data(), &theBuffer[aDataStart], theOut.size());
			return true;
		}
		else if (theCd->mCompressionMethod == 8)
		{
			z_stream aStrm = {};
			aStrm.next_in = const_cast<Bytef*>(&theBuffer[aDataStart]);
			aStrm.avail_in = theCd->mCompressedSize;
			aStrm.next_out = theOut.data();
			aStrm.avail_out = theCd->mUncompressedSize;
			if (inflateInit2(&aStrm, -MAX_WBITS) == Z_OK)
			{
				int aRet = inflate(&aStrm, Z_FINISH);
				inflateEnd(&aStrm);
				return (aRet == Z_STREAM_END || aRet == Z_OK || (aRet == Z_BUF_ERROR && aStrm.total_out == theCd->mUncompressedSize));
			}
		}

		return false;
	}

	static void ExtractFiles(const std::vector<uint8_t>& theBuffer, size_t theEocdOffset, APArchiveContents& output)
	{
		const ZipEOCD* anEocd = reinterpret_cast<const ZipEOCD*>(&theBuffer[theEocdOffset]);

		size_t aCurr = anEocd->mCentralDirOffset;
		for (uint16_t i = 0; i < anEocd->mTotalEntries && aCurr + sizeof(ZipCentralDirHeader) <= theBuffer.size(); ++i)
		{
			const ZipCentralDirHeader* aCd = reinterpret_cast<const ZipCentralDirHeader*>(&theBuffer[aCurr]);
			if (aCd->mSignature != 0x02014b50)
				break;

			size_t aNextEntry = aCurr + sizeof(ZipCentralDirHeader) + aCd->mFilenameLength + aCd->mExtraFieldLength + aCd->mCommentLength;
			if (aNextEntry > theBuffer.size())
				break;

			std::string aName(reinterpret_cast<const char*>(&theBuffer[aCurr + sizeof(ZipCentralDirHeader)]), aCd->mFilenameLength);
			for (char& c : aName)
				c = (c == '\\') ? '/' : static_cast<char>(tolower(static_cast<unsigned char>(c)));

			size_t aSlash = aName.find_last_of('/');
			std::string aBase = (aSlash != std::string::npos) ? aName.substr(aSlash + 1) : aName;

			bool isArchipelagoJson = (aBase == "archipelago.json");
			bool isSlotDataJson = (aBase == "slot_data.json");

			if (isArchipelagoJson || isSlotDataJson)
			{
				std::vector<uint8_t> anUncomp;
				if (ExtractFile(theBuffer, aCd, anUncomp))
				{
					if (isArchipelagoJson) { output.mArchipelagoJson.assign(reinterpret_cast<char*>(anUncomp.data()), anUncomp.size()); }
					if (isSlotDataJson) { output.mSlotDataJson.assign(reinterpret_cast<char*>(anUncomp.data()), anUncomp.size()); }
				}
			}

			aCurr = aNextEntry;
		}
	}

	bool APSaveImporter::ExtractArchive(const std::string& theZipFilePath, APArchiveContents& output, std::string& error)
	{
		output = APArchiveContents();
		error.clear();

		std::ifstream aFile(theZipFilePath, std::ios::binary | std::ios::ate);
		if (!aFile.is_open())
		{
			error = "Could not open file: " + theZipFilePath;
			return false;
		}

		size_t aFileSize = static_cast<size_t>(aFile.tellg());
		if (aFileSize < sizeof(ZipEOCD))
		{
			error = "File is too small to be a valid ZIP archive.";
			return false;
		}

		aFile.seekg(0, std::ios::beg);
		std::vector<uint8_t> aBuffer(aFileSize);
		if (!aFile.read(reinterpret_cast<char*>(aBuffer.data()), aFileSize))
		{
			error = "Failed to read file data.";
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

		if (anEocdOffset == static_cast<size_t>(-1))
		{
			error = "Missing ZIP End of Central Directory record.";
			return false;
		}

		ExtractFiles(aBuffer, anEocdOffset, output);

		if (output.mArchipelagoJson.empty())
		{
			error = "The archive is missing archipelago.json.";
			return false;
		}
		if (output.mSlotDataJson.empty())
		{
			error = "The archive is missing slot_data.json.";
			return false;
		}

		// Parse archipelago.json
		auto j = nlohmann::json::parse(output.mArchipelagoJson, nullptr, false);
		for (const char* k : { "player_name", "slot_name", "slot", "name" })
			if (j.contains(k) && j[k].is_string() && !(output.mSlotName = j[k]).empty()) break;

		for (const char* k : { "player", "player_id", "id" })
			if (j.contains(k) && j[k].is_number()) { output.mPlayerId = j[k]; break; }

		// Parse slot_data.json
		auto aSlotJson = nlohmann::json::parse(output.mSlotDataJson, nullptr, false);
		std::string aTank = aSlotJson.value("starting_tank_unlock", "");
		for (int t = 1; t <= 4; ++t)
			if (aTank.find("Tank " + std::to_string(t)) != std::string::npos || aTank == std::to_string(t))
				output.mStartingTank = t;

		output.mServer = "archipelago.gg:";

		return true;
	}
}
