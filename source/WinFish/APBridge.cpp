// apclient.hpp must come before anything that includes windows.h (see APBridge.h).
#include <chrono>
#include <apclient.hpp>
#include <apuuid.hpp>

#include "APBridge.h"

using namespace Sexy;

// Must match `game` in Archipelago/worlds/insaniquarium.
static const char* const AP_GAME_NAME = "Insaniquarium Deluxe";

// Remote items from other worlds, our own world, and starting inventory.
static const int AP_ITEMS_HANDLING = 0b111;


static std::string MakeServerUri(const std::string& theServer)
{
	if (theServer.find("://") != std::string::npos)
		return theServer;

	std::string aHost = theServer.substr(0, theServer.find(':'));
	for (char& c : aHost)
		c = (char)tolower((unsigned char)c);
	if (aHost == "localhost" || aHost == "127.0.0.1")
		return "ws://" + theServer;
	return theServer;
}

APBridge::APBridge(const std::string& theDataFolder, const std::string& theCertFile)
{
	mClient = NULL;
	mState = AP_DISCONNECTED;
	mSocketErrors = 0;
	mSocketErrorsToReport = 1;
	mJustConnected = false;
	mDataFolder = theDataFolder;
	mCertFile = theCertFile;
	mGoalSent = false;
	mLastSentDeathLinkTime = 0.0;
}

APBridge::~APBridge()
{
	Disconnect();
}

void APBridge::Connect(const std::string& theServer, const std::string& theSlot, const std::string& thePassword)
{
	Disconnect();

	mServer = theServer;
	mSlot = theSlot;
	mPassword = thePassword;
	mLastError.clear();
	mSocketErrors = 0;
	mJustConnected = false;
	mReceivedItems.clear();
	mConfirmedChecks.clear();
	mSentItems.clear();
	mCheckedLocations.reset();

	std::string aUuid = ap_get_uuid(mDataFolder + "ap_uuid.txt", theServer);
	std::string aUri = MakeServerUri(theServer);
	mSocketErrorsToReport = aUri.find("://") != std::string::npos ? 1 : 2;
	mClient = new APClient(aUuid, AP_GAME_NAME, aUri, mCertFile);
	mClient->set_receive_own_locations(true);
	mState = AP_SOCKET_CONNECTING;

	mClient->set_socket_error_handler([this](const std::string& theError)
	{
		mLastError = theError;
		mSocketErrors++;
	});

	mClient->set_socket_disconnected_handler([this]()
	{
		// apclientpp reconnects on its own; we just have to log in again after the next RoomInfo.
		mState = AP_SOCKET_CONNECTING;
	});

	mClient->set_room_info_handler([this]()
	{
		mState = AP_SLOT_CONNECTING;
		mSocketErrors = 0;
		std::list<std::string> aTags;
		if (mSlotData.mDeathLink)
			aTags.push_back("DeathLink");
		mClient->ConnectSlot(mSlot, mPassword, AP_ITEMS_HANDLING, aTags);
	});

	mClient->set_slot_connected_handler([this](const nlohmann::json& theSlotData)
	{
		mState = AP_SLOT_CONNECTED;
		mLastError.clear();
		mJustConnected = true;
		mSlotData.ParseJson(theSlotData.dump());
		if (mSlotData.mDeathLink)
			mClient->ConnectUpdate(false, 0, true, {"DeathLink"});
	});

	mClient->set_bounced_handler([this](const nlohmann::json& thePacket)
	{
		if (!mSlotData.mDeathLink)
			return;

		if (!thePacket.contains("tags") || !thePacket["tags"].is_array())
			return;

		bool isDeathLink = false;
		for (const auto& aTag : thePacket["tags"])
		{
			if (aTag == "DeathLink")
			{
				isDeathLink = true;
				break;
			}
		}
		if (!isDeathLink)
			return;

		if (!thePacket.contains("data") || !thePacket["data"].is_object())
			return;

		const nlohmann::json& aData = thePacket["data"];
		std::string aSource = aData.value("source", "");
		std::string aCause = aData.value("cause", "");
		double aTime = aData.value("time", 0.0);

		if (aSource == mSlot)
			return;
		if (aTime > 0.0 && aTime == mLastSentDeathLinkTime)
			return;

		mReceivedDeathLinks.push_back({ aTime, aSource, aCause });
	});

	mClient->set_items_received_handler([this](const std::list<APClient::NetworkItem>& theItems)
	{
		for (const APClient::NetworkItem& anItem : theItems)
			mReceivedItems.push_back({ static_cast<APItem>(anItem.item), anItem.location, anItem.player, anItem.index });
	});

	// Fires with the server's checked locations right after login, and again whenever it confirms new checks.
	mClient->set_location_checked_handler([this](const std::list<int64_t>& theLocations)
	{
		for (int64_t aLocationId : theLocations)
		{
			APLocation aLoc = static_cast<APLocation>(aLocationId);
			mConfirmedChecks.push_back(aLoc);
			SetLocationChecked(aLoc);
		
		}
	});

	mClient->set_print_json_handler([this](const APClient::PrintJSONArgs& theArgs)
	{
		if (theArgs.type != "ItemSend" || theArgs.item == NULL || theArgs.receiving == NULL)
			return;

		if (theArgs.item->player != mClient->get_player_number())
			return;

		int aReceiverSlot = *theArgs.receiving;
		std::string aRecipientSlot = mClient->get_player_alias(aReceiverSlot);
		if (aRecipientSlot.empty() || aRecipientSlot == "Unknown")
		{
			if (aReceiverSlot == mClient->get_player_number())
				aRecipientSlot = mSlot;
			else
				aRecipientSlot = "Player " + std::to_string(aReceiverSlot);
		}

		int64_t aItemId = theArgs.item->item;
		std::string aReceiverGame = mClient->get_player_game(aReceiverSlot);
		std::string anItemName = mClient->get_item_name(aItemId, aReceiverGame);
		if (anItemName.empty() || anItemName == "Unknown")
		{
			const char* aLocalName = APItemName(aItemId);
			if (strcmp(aLocalName, "Unknown Item") != 0)
				anItemName = aLocalName;
			else
				anItemName = "Item " + std::to_string(aItemId);
		}

		mSentItems.push_back({ anItemName, aRecipientSlot });
	});

	mClient->set_slot_refused_handler([this](const std::list<std::string>& theErrors)
	{
		mState = AP_SLOT_REFUSED;
		mLastError.clear();
		for (const std::string& anError : theErrors)
		{
			if (!mLastError.empty())
				mLastError += ", ";
			mLastError += anError;
		}
	});
}

void APBridge::Disconnect()
{
	delete mClient;
	mClient = NULL;
	mState = AP_DISCONNECTED;
	mSentItems.clear();
	mReceivedDeathLinks.clear();
	mLastSentDeathLinkTime = 0.0;
	mGoalSent = false;
	mSlotData = SlotData();
	mCheckedLocations.reset();
}

void APBridge::StopForWrongSeed()
{
	Disconnect();
	mState = AP_WRONG_SEED;
}

void APBridge::Update()
{
	if (mClient != NULL)
		mClient->poll();
}

bool APBridge::ConsumeJustConnected()
{
	bool aJustConnected = mJustConnected;
	mJustConnected = false;
	return aJustConnected;
}

template<typename T>
static void PopQueue(std::vector<T>& theSrc, std::vector<T>& theDest)
{
	theDest.swap(theSrc);
	theSrc.clear();
}

void APBridge::PopReceivedItems(std::vector<APReceivedItem>& theItems)
{
	PopQueue(mReceivedItems, theItems);
}

void APBridge::PopConfirmedChecks(std::vector<APLocation>& theLocations)
{
	PopQueue(mConfirmedChecks, theLocations);
}

void APBridge::PopSentItems(std::vector<APSentItem>& theSentItems)
{
	PopQueue(mSentItems, theSentItems);
}

void APBridge::PopReceivedDeathLinks(std::vector<APDeathLink>& theDeathLinks)
{
	PopQueue(mReceivedDeathLinks, theDeathLinks);
}

void APBridge::SendDeathLink(const std::string& theCause)
{
	if (mClient == NULL || mState != AP_SLOT_CONNECTED || !mSlotData.mDeathLink)
		return;

	double aTimestamp = (double)std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
	mLastSentDeathLinkTime = aTimestamp;

	nlohmann::json aData = {
		{"time", aTimestamp},
		{"source", mSlot}
	};
	if (!theCause.empty())
		aData["cause"] = theCause;

	mClient->Bounce(aData, {}, {}, {"DeathLink"});
}

bool APBridge::IsLocationChecked(int64_t theLocationId) const
{
	if (static_cast<size_t>(theLocationId) < mCheckedLocations.size())
		return mCheckedLocations.test(static_cast<size_t>(theLocationId));

	if (mClient == NULL)
		return false;
	std::set<int64_t> aChecked = mClient->get_checked_locations();
	return aChecked.find(theLocationId) != aChecked.end();
}

std::string APBridge::GetSeed() const
{
	return mClient != NULL ? mClient->get_seed() : "";
}

void APBridge::SendChecks(const std::set<APLocation>& theLocations)
{
	if (mClient == NULL || theLocations.empty())
		return;
	std::list<int64_t> aLocations;
	for (APLocation aLoc : theLocations)
		aLocations.push_back(static_cast<int64_t>(aLoc));
	mClient->LocationChecks(aLocations);
}

void APBridge::SendCheck(APLocation theLocation)
{
	if (mClient == NULL)
		return;
	mClient->LocationChecks({ static_cast<int64_t>(theLocation) });
}

void APBridge::SendGoal()
{
	if (mClient != NULL)
	{
		mClient->StatusUpdate(APClient::ClientStatus::GOAL);
		mGoalSent = true;
	}
}

void APBridge::RequestSync()
{
	if (mClient != NULL)
		mClient->Sync();
}

std::string APBridge::GetStatusText() const
{
	switch (mState)
	{
	case AP_SOCKET_CONNECTING:
		if (mSocketErrors >= mSocketErrorsToReport)
			return "Archipelago: can't reach " + mServer + ", retrying...";
		return "Archipelago: connecting to " + mServer + "...";
	case AP_SLOT_CONNECTING:
		return "Archipelago: logging in as " + mSlot + "...";
	case AP_SLOT_CONNECTED:
		return "Archipelago: connected as " + mSlot;
	case AP_SLOT_REFUSED:
		return "Archipelago: refused (" + mLastError + ")";
	case AP_WRONG_SEED:
		return "Archipelago: this server is running a different multiworld";
	default:
		return "";
	}
}
