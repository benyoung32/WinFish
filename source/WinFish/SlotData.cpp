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

		mCyraxRequired = aJson.value("cyrax_required", false);
		mTimeTrialRequired = aJson.value("time_trial_required", false);
		mChallengeModeRequired = aJson.value("challenge_mode_required", false);

		// Starting Tank Unlock
		mStartingTank = 1;
		if (aJson.contains("starting_tank_unlock"))
		{
			if (aJson["starting_tank_unlock"].is_string())
			{
				std::string aTankStr = aJson["starting_tank_unlock"].get<std::string>();
				if (aTankStr.find("Tank 2") != std::string::npos || aTankStr == "2")
					mStartingTank = 2;
				else if (aTankStr.find("Tank 3") != std::string::npos || aTankStr == "3")
					mStartingTank = 3;
				else if (aTankStr.find("Tank 4") != std::string::npos || aTankStr == "4")
					mStartingTank = 4;
			}
			else if (aJson["starting_tank_unlock"].is_number())
			{
				mStartingTank = aJson["starting_tank_unlock"].get<int>();
			}
		}

		mAliensanity = aJson.value("aliensanity", false);

		// Upgrade Availability: 0 = instant, 1 = free in shop, 2 = shop buy
		mUpgradeAvailability = UPGRADE_AVAILABILITY_INSTANT;
		if (aJson.contains("upgrade_availability"))
		{
			if (aJson["upgrade_availability"].is_number())
			{
				mUpgradeAvailability = aJson["upgrade_availability"].get<int>();
			}
			else if (aJson["upgrade_availability"].is_string())
			{
				std::string aStr = aJson["upgrade_availability"].get<std::string>();
				if (aStr == "free_in_shop" || aStr == "free")
					mUpgradeAvailability = UPGRADE_AVAILABILITY_FREE_IN_SHOP;
				else if (aStr == "shop_buy" || aStr == "shop")
					mUpgradeAvailability = UPGRADE_AVAILABILITY_SHOP_BUY;
				else
					mUpgradeAvailability = UPGRADE_AVAILABILITY_INSTANT;
			}
		}

		// Starting Money Bonus
		mStartingMoneyBonus = 0;
		if (aJson.contains("starting_money") && aJson["starting_money"].is_number())
			mStartingMoneyBonus = aJson["starting_money"].get<int>();

		// Starting Pet Slots (default 0)
		mStartPetSlots = 0;
		if (aJson.contains("start_pet_slots"))
		{
			if (aJson["start_pet_slots"].is_number())
				mStartPetSlots = aJson["start_pet_slots"].get<int>();
			else if (aJson["start_pet_slots"].is_string())
				mStartPetSlots = atoi(aJson["start_pet_slots"].get<std::string>().c_str());
		}
		else if (aJson.contains("starting_pet_slots"))
		{
			if (aJson["starting_pet_slots"].is_number())
				mStartPetSlots = aJson["starting_pet_slots"].get<int>();
			else if (aJson["starting_pet_slots"].is_string())
				mStartPetSlots = atoi(aJson["starting_pet_slots"].get<std::string>().c_str());
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}

