#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include "SlotData.h"

Sexy::SlotData::SlotData() :
	mStartingTank(1),
	mAliensanity(false),
	mUpgradeAvailability(0),
	mStartingMoneyBonus(0),
	mStartPetSlots(0)
{
}

Sexy::SlotData::SlotData(const std::string& theJsonOrPath) :
	mStartingTank(1),
	mAliensanity(false),
	mUpgradeAvailability(0),
	mStartingMoneyBonus(0),
	mStartPetSlots(0)
{
	if (!theJsonOrPath.empty())
	{
		size_t aFirst = theJsonOrPath.find_first_not_of(" \t\r\n");
		if (aFirst != std::string::npos && theJsonOrPath[aFirst] == '{')
			ParseJson(theJsonOrPath);
		else
			LoadFromFile(theJsonOrPath);
	}
}

bool Sexy::SlotData::LoadFromFile(const std::string& theFilePath)
{
	std::ifstream aFile(theFilePath);
	if (!aFile.is_open())
		return false;

	std::stringstream aBuffer;
	aBuffer << aFile.rdbuf();
	return ParseJson(aBuffer.str());
}

bool Sexy::SlotData::ParseJson(const std::string& theJsonStr)
{
	if (theJsonStr.empty())
		return false;

	mRawJson = theJsonStr;
	try
	{
		nlohmann::json aJson = nlohmann::json::parse(theJsonStr);

		mCyraxRequired = aJson["cyrax_required"].get<bool>();
		mTimeTrialRequired = aJson["time_trial_required"].get<bool>();
		mChallengeModeRequired = aJson["challenge_mode_required"].get<bool>();

		// Starting Tank Unlock
		std::string aTankStr = aJson["starting_tank_unlock"].get<std::string>();
		if (aTankStr.find("Tank 2") != std::string::npos)
			mStartingTank = 2;
		else if (aTankStr.find("Tank 3") != std::string::npos)
			mStartingTank = 3;
		else if (aTankStr.find("Tank 4") != std::string::npos)
			mStartingTank = 4;
		else
			mStartingTank = 1;

		mAliensanity = aJson["aliensanity"].get<bool>();

		mUpgradeAvailability = aJson["upgrade_availability"].get<int>();

		// Starting Money Bonus
		if (aJson.contains("starting_money"))
		{
			if (aJson["starting_money"].is_number())
				mStartingMoneyBonus = aJson["starting_money"].get<int>();
		}

		// Starting Pet Slots (default 0)
		mStartPetSlots = 0;
		if (aJson.contains("start_pet_slots"))
		{
			if (aJson["start_pet_slots"].is_number())
				mStartPetSlots = aJson["start_pet_slots"].get<int>();
		}
		else if (aJson.contains("starting_pet_slots"))
		{
			if (aJson["starting_pet_slots"].is_number())
				mStartPetSlots = aJson["starting_pet_slots"].get<int>();
		}

		return true;
	}
	catch (...)
	{
		return false;
	}
}

