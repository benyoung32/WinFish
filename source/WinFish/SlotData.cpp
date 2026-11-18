#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>
#include "SlotData.h"
#include "Alien.h"

static int ParseTankNumber(const std::string& theStr)
{
	int aTank = 0;
	size_t aPos = theStr.find_first_of("0123456789");
	if (aPos != std::string::npos)
		aTank = atoi(theStr.c_str() + aPos);
	return aTank;
}

static int AlienStringToType(const std::string& theAlienStr)
{
	std::string aLower = theAlienStr;
	std::transform(aLower.begin(), aLower.end(), aLower.begin(), [](unsigned char c) { return (char)std::tolower(c); });

	if (aLower.empty() || aLower == "none" || aLower == "alien_none")
		return Sexy::ALIEN_NONE;
	if (aLower.find("weak") != std::string::npos)
		return Sexy::ALIEN_WEAK_SYLV;
	if (aLower.find("mini") != std::string::npos)
		return Sexy::ALIEN_MINI_SYLV;
	if (aLower.find("sylv") != std::string::npos)
		return Sexy::ALIEN_STRONG_SYLV;
	if (aLower.find("balrog") != std::string::npos)
		return Sexy::ALIEN_BALROG;
	if (aLower.find("gus") != std::string::npos)
		return Sexy::ALIEN_GUS;
	if (aLower.find("destructor") != std::string::npos)
		return Sexy::ALIEN_DESTRUCTOR;
	if (aLower.find("uly") != std::string::npos)
		return Sexy::ALIEN_ULYSEES;
	if (aLower.find("psycho") != std::string::npos || aLower.find("squid") != std::string::npos)
		return Sexy::ALIEN_PSYCHOSQUID;
	if (aLower.find("bilat") != std::string::npos)
		return Sexy::ALIEN_BILATERUS;
	if (aLower.find("double") != std::string::npos)
		return Sexy::ALIEN_DOUBLE_SPAWN;
	if (aLower.find("cyrax") != std::string::npos)
		return Sexy::ALIEN_CYRAX;

	return atoi(theAlienStr.c_str());
}

Sexy::SlotData::SlotData() :
	mCyraxRequired(false),
	mTimeTrialRequired(false),
	mChallengeModeRequired(false),
	mStartingTank(1),
	mAliensanity(false),
	mUpgradeAvailability(0),
	mStartingMoneyBonus(0),
	mStartPetSlots(0),
	mDeathLink(false),
	mDeathLinkAmnesty(0),
	mDeathLinkEffect(0),
	mAlienRandomized(false)
{
}

Sexy::SlotData::SlotData(const std::string& theJsonOrPath) :
	mCyraxRequired(false),
	mTimeTrialRequired(false),
	mChallengeModeRequired(false),
	mStartingTank(1),
	mAliensanity(false),
	mUpgradeAvailability(0),
	mStartingMoneyBonus(0),
	mStartPetSlots(0),
	mDeathLink(false),
	mDeathLinkAmnesty(0),
	mDeathLinkEffect(0),
	mAlienRandomized(false)
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

		// Death Link
		mDeathLink = false;
		if (aJson.contains("death_link"))
		{
			if (aJson["death_link"].is_boolean())
				mDeathLink = aJson["death_link"].get<bool>();
			else if (aJson["death_link"].is_number())
				mDeathLink = (aJson["death_link"].get<int>() != 0);
			else if (aJson["death_link"].is_string())
			{
				std::string aStr = aJson["death_link"].get<std::string>();
				mDeathLink = (aStr == "true" || aStr == "1");
			}
		}

		// Death Link Amnesty
		mDeathLinkAmnesty = 0;
		if (aJson.contains("death_link_amnesty"))
		{
			if (aJson["death_link_amnesty"].is_number())
				mDeathLinkAmnesty = aJson["death_link_amnesty"].get<int>();
			else if (aJson["death_link_amnesty"].is_string())
				mDeathLinkAmnesty = atoi(aJson["death_link_amnesty"].get<std::string>().c_str());
		}

		// Death Link Effect (0 = Instant Kill, 1 = Random Trap)
		mDeathLinkEffect = DEATH_LINK_EFFECT_INSTANT_KILL;
		if (aJson.contains("death_link_effect"))
		{
			if (aJson["death_link_effect"].is_number())
				mDeathLinkEffect = aJson["death_link_effect"].get<int>();
			else if (aJson["death_link_effect"].is_string())
				mDeathLinkEffect = atoi(aJson["death_link_effect"].get<std::string>().c_str());
		}

		// Alien Randomized
		mAlienRandomized = false;
		if (aJson.contains("alien_randomized"))
		{
			if (aJson["alien_randomized"].is_boolean())
				mAlienRandomized = aJson["alien_randomized"].get<bool>();
			else if (aJson["alien_randomized"].is_number())
				mAlienRandomized = (aJson["alien_randomized"].get<int>() != 0);
			else if (aJson["alien_randomized"].is_string())
			{
				std::string aStr = aJson["alien_randomized"].get<std::string>();
				mAlienRandomized = (aStr == "true" || aStr == "1");
			}
		}

		// Alien Randomization: dict mapping from "tank" to a list of aliens
		mAlienRandomization.clear();
		if (aJson.contains("alien_randomization") && aJson["alien_randomization"].is_object())
		{
			for (auto& anItem : aJson["alien_randomization"].items())
			{
				int aTank = ParseTankNumber(anItem.key());
				if (aTank <= 0)
					continue;

				std::vector<int> aAliensList;
				if (anItem.value().is_array())
				{
					for (auto& aAlienVal : anItem.value())
					{
						if (aAlienVal.is_number())
						{
							aAliensList.push_back(aAlienVal.get<int>());
						}
						else if (aAlienVal.is_string())
						{
							aAliensList.push_back(AlienStringToType(aAlienVal.get<std::string>()));
						}
					}
				}
				mAlienRandomization[aTank] = aAliensList;
			}

			if (!aJson.contains("alien_randomized") && !mAlienRandomization.empty())
				mAlienRandomized = true;
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}

