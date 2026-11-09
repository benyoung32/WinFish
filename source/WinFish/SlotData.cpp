#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include "SlotData.h"

Sexy::SlotData::SlotData() :
	mGoal(static_cast<int>(APGoal::CYRAX)),
	mStartingTank(1),
	mAliensanity(false),
	mUpgradeAvailability(0),
	mStartingMoneyBonus(0)
{
}

Sexy::SlotData::SlotData(const std::string& theJsonOrPath) :
	mGoal(static_cast<int>(APGoal::CYRAX)),
	mStartingTank(1),
	mAliensanity(false),
	mUpgradeAvailability(0),
	mStartingMoneyBonus(0)
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

		// Goal
		if (aJson.contains("goal"))
		{
			if (aJson["goal"].is_number())
			{
				mGoal = aJson["goal"].get<int>();
			}
			else if (aJson["goal"].is_string())
			{
				std::string aGoalStr = aJson["goal"].get<std::string>();
				if (aGoalStr == "all_tanks" || aGoalStr == "tanks")
					mGoal = static_cast<int>(APGoal::ALL_TANKS);
				else if (aGoalStr == "all_pets" || aGoalStr == "pets")
					mGoal = static_cast<int>(APGoal::ALL_PETS);
				else
					mGoal = static_cast<int>(APGoal::CYRAX);
			}
		}

		// Starting Tank Unlock
		if (aJson.contains("starting_tank_unlock"))
		{
			if (aJson["starting_tank_unlock"].is_number())
			{
				mStartingTank = aJson["starting_tank_unlock"].get<int>();
			}
			else if (aJson["starting_tank_unlock"].is_string())
			{
				std::string aTankStr = aJson["starting_tank_unlock"].get<std::string>();
				if (aTankStr.find("Tank 2") != std::string::npos || aTankStr == "2")
					mStartingTank = 2;
				else if (aTankStr.find("Tank 3") != std::string::npos || aTankStr == "3")
					mStartingTank = 3;
				else if (aTankStr.find("Tank 4") != std::string::npos || aTankStr == "4")
					mStartingTank = 4;
				else
					mStartingTank = 1;
			}
		}
		else if (aJson.contains("starting_tank"))
		{
			if (aJson["starting_tank"].is_number())
				mStartingTank = aJson["starting_tank"].get<int>();
		}

		// Aliensanity
		if (aJson.contains("aliensanity"))
		{
			if (aJson["aliensanity"].is_boolean())
				mAliensanity = aJson["aliensanity"].get<bool>();
			else if (aJson["aliensanity"].is_number())
				mAliensanity = (aJson["aliensanity"].get<int>() != 0);
		}

		// Upgrade Availability
		if (aJson.contains("upgrade_availability"))
		{
			if (aJson["upgrade_availability"].is_number())
				mUpgradeAvailability = aJson["upgrade_availability"].get<int>();
		}

		// Starting Money Bonus
		if (aJson.contains("starting_money"))
		{
			if (aJson["starting_money"].is_number())
				mStartingMoneyBonus = aJson["starting_money"].get<int>();
		}

		return true;
	}
	catch (...)
	{
		return false;
	}
}

