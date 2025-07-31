#include <Geode/modify/DailyLevelPage.hpp>
#include <Geode/modify/DailyLevelNode.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <Geode/ui/LazySprite.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

struct EventButtonData {
	int eventID;
	std::string eventName;
	std::string formattedURL;
};

static const std::unordered_map<TextureQuality, float> textureQualityToScale {
	{kTextureQualityLow, .4f},
	{kTextureQualityMedium, .8f},
	{kTextureQualityHigh, 1.f},
};

#define LOAD_LAZY_SPRITE(mcl, eventID, eventName)\
	MyCreatorLayer::Fields* fields = mcl->m_fields.self();\
	const std::string& formattedURL = fmt::format("https://raw.githubusercontent.com/RayDeeUx/DynamicEventButtonData/main/icons/{}.png", eventID);\
	EventButtonData currentEventData = { eventID, eventName, formattedURL };\
	fields->m_eventIcon = Ref(LazySprite::create({40.f, 40.f}, false));\
	fields->m_eventIcon->setLoadCallback([mcl, currentEventData](const Result<>& result) {\
		mcl->onEventLevelIconDownloadFinished(result, currentEventData);\
	});\
	fields->m_eventIcon->loadFromUrl(formattedURL)

#define FIND_CREATORLAYER_IF_NULL_RETURN\
	CreatorLayer* cl = CCScene::get()->getChildByType<CreatorLayer>(0);\
	if (!cl) return

#define FIND_EVENT_BUTTON_FROM(parent)\
	CCNode* eventButton = parent->querySelector("creator-buttons-menu > event-button");\
	if (!eventButton) return

#define FIND_CHILD_SPRITE_FROM(parent)\
	CCSprite* childSprite = parent->getChildByType<CCSprite>(0);\
	if (!childSprite) return

#define RESTORE_NODE_TRAITS(childSprite)\
	childSprite->setTag(-1);\
	childSprite->setOpacity(255);\
	childSprite->setCascadeColorEnabled(true);\
	childSprite->setCascadeOpacityEnabled(true);\
	for (CCNode* node : CCArrayExt<CCNode*>(childSprite->getChildren())) {\
		if (string::startsWith(node->getID(), "dynamic-event-"_spr)) node->removeMeAndCleanup();\
		else node->setSkewX(0.f);\
	}

#define SECURE_CONTAIN_PROTECT\
	FIND_CREATORLAYER_IF_NULL_RETURN;\
	FIND_EVENT_BUTTON_FROM(cl);\
	FIND_CHILD_SPRITE_FROM(eventButton);\
	RESTORE_NODE_TRAITS(childSprite)

class $modify(MyCreatorLayer, CreatorLayer) {
	struct Fields {
		Ref<LazySprite> m_eventIcon;
		Ref<LazySprite> m_eventIconShadow;
	};
	bool init() {
		if (!CreatorLayer::init()) return false;
		if (!Mod::get()->getSettingValue<bool>("enabled")) return true;
		GameLevelManager* glm = GameLevelManager::get();
		glm->getGJDailyLevelState(GJTimedLevelType::Event);

		const GJGameLevel* eventLevel = glm->getSavedDailyLevel(glm->m_activeEventID);
		if (!eventLevel) return true;

		const int currentEventID = glm->m_activeEventID - 200000;
		const std::string& currentEventLevelName = eventLevel->m_levelName;

		log::info("m_activeEventID (CURRENT EVENT ID): {}", currentEventID);
		log::info("currentEventLevelName: {}", currentEventLevelName);

		LOAD_LAZY_SPRITE(this, currentEventID, currentEventLevelName);

		return true;
	}
	void onEventLevelIconDownloadFinished(const Result<>& result, const EventButtonData& eventButtonData) {
		Fields* fields = this->m_fields.self();
		const int currentEventID = eventButtonData.eventID;
		const std::string& currentEventName = eventButtonData.eventName;
		if (!fields || !fields->m_eventIcon || result.isErr()) return log::info("could not load level icon for event level ID {} ({})", currentEventID, currentEventName);

		FIND_EVENT_BUTTON_FROM(this) log::info("could not find eventButton");
		FIND_CHILD_SPRITE_FROM(eventButton) log::info("could not find childSprite");

		if (childSprite->getTag() == 7252025) return log::info("childSprite already replaced");
		for (CCNode* node : CCArrayExt<CCNode*>(childSprite->getChildren())) {
			if (string::startsWith(node->getID(), "dynamic-event-"_spr)) continue;
			node->setSkewX(90.f); // quasi-invisibility; aint nobody got time to change this node trait and i sure as hell won't store node traits in a new data structure
		}

		CategoryButtonSprite* replacementSprite = CategoryButtonSprite::create(fields->m_eventIcon);
		fields->m_eventIcon->setPosition({50.f, 60.f});
		fields->m_eventIcon->setZOrder(1);
		if (currentEventID != 14) {
			// boomkitty + arclia special treatment
			fields->m_eventIcon->setScale(fields->m_eventIcon->getScale() * .75f);
			fields->m_eventIcon->setPositionX(52.5f);
		}
		fields->m_eventIcon->setScale(fields->m_eventIcon->getScale() * textureQualityToScale.at(CCDirector::get()->getLoadedTextureQuality()));

		fields->m_eventIconShadow = Ref(LazySprite::create({40.f, 40.f}, false));\
		fields->m_eventIconShadow->setLoadCallback([this, eventButtonData, replacementSprite](const Result<>& shadowResult) {\
			this->onEventLevelIconShadowDownloadFinished(shadowResult, eventButtonData, replacementSprite);\
		});\
		fields->m_eventIconShadow->loadFromUrl(eventButtonData.formattedURL);

		CCLabelBMFont* levelNameLabelShadow = CCLabelBMFont::create(currentEventName.c_str(), "bigFont.fnt");
		levelNameLabelShadow->setPosition({52.f + .85f, 20.5f - .85f});
		levelNameLabelShadow->limitLabelWidth(85.f, 1.f, .0001f);
		levelNameLabelShadow->setColor({0, 0, 0});
		levelNameLabelShadow->setOpacity(128);
		replacementSprite->addChild(levelNameLabelShadow);

		CCLabelBMFont* levelNameLabel = CCLabelBMFont::create(currentEventName.c_str(), "bigFont.fnt");
		levelNameLabel->limitLabelWidth(85.f, 1.f, .0001f);
		levelNameLabel->setPosition({52.f, 20.5f});
		replacementSprite->addChild(levelNameLabel);

		childSprite->setCascadeOpacityEnabled(false);
		childSprite->setCascadeColorEnabled(false);
		childSprite->setTag(7252025);
		childSprite->setOpacity(0);

		childSprite->addChildAtPosition(replacementSprite, Anchor::Center);
		replacementSprite->setID("dynamic-event-button"_spr);

		fields->m_eventIcon->setID("dynamic-event-icon"_spr);
		levelNameLabelShadow->setID("dynamic-event-label-shadow"_spr);
		levelNameLabel->setID("dynamic-event-label"_spr);

		if (!Mod::get()->getSettingValue<bool>("wordWrapping")) return;

		if (currentEventName.length() < 16) return log::info("level name is not over 15 characters long, exiting early");

		const std::vector<std::string>& splitName = string::split(currentEventName, " ");
		if (splitName.size() == 1) return log::info("level name is one word, exiting early");

		int longestWord = -1;
		int index = -1;
		int longestWordIndex = -1;
		std::vector<int> wordLengths {};
		std::string newLevelString = "";

		for (const std::string& word : splitName) wordLengths.push_back(static_cast<int>(word.length()));
		for (const int& length : wordLengths) {
			index++;
			if (length < longestWord) continue;
			longestWord = length;
			longestWordIndex = index;
		}
		log::info("longestWord: {}", longestWord);
		log::info("longestWordIndex: {}", longestWordIndex);

		index = -1;
		for (const std::string& word : splitName) {
			index++;
			newLevelString = newLevelString.append(word);
			if (index != longestWordIndex && index + 1 != longestWordIndex) {
				newLevelString = newLevelString.append(" ");
			} else {
				newLevelString = newLevelString.append("\n");
			}
		}

		levelNameLabel->setString(newLevelString.c_str());
		levelNameLabel->setAlignment(kCCTextAlignmentCenter);
		levelNameLabel->setScale(levelNameLabel->getScale() * 1.1f);
		levelNameLabel->setPositionY(levelNameLabel->getPositionY() + 2.f);

		levelNameLabelShadow->setString(newLevelString.c_str());
		levelNameLabelShadow->setAlignment(kCCTextAlignmentCenter);
		levelNameLabelShadow->setScale(levelNameLabelShadow->getScale() * 1.1f);
		levelNameLabelShadow->setPositionY(levelNameLabelShadow->getPositionY() + 2.f);
	}
	void onEventLevelIconShadowDownloadFinished(const Result<>& shadowResult, const EventButtonData& eventButtonData, CCSprite* replacementSprite) {
		const Fields* fields = this->m_fields.self();

		if (!fields || !fields->m_eventIconShadow || shadowResult.isErr()) return log::info("could not load level icon shadow for event level ID {} ({})", eventButtonData.eventID, eventButtonData.eventName);

		replacementSprite->addChild(fields->m_eventIconShadow);

		fields->m_eventIconShadow->setOpacity(96);
		fields->m_eventIconShadow->setColor({0, 0, 0});
		fields->m_eventIconShadow->setScale(fields->m_eventIcon->getScale());
		fields->m_eventIconShadow->setZOrder(fields->m_eventIcon->getZOrder() - 1);
		fields->m_eventIconShadow->setPosition(fields->m_eventIcon->getPosition());
		fields->m_eventIconShadow->setPositionX(fields->m_eventIconShadow->getPositionX() + 2.f);
		fields->m_eventIconShadow->setPositionY(fields->m_eventIconShadow->getPositionY() - 1.5f);
		fields->m_eventIconShadow->setID("dynamic-event-icon-shadow"_spr);
	}
};

class $modify(MyDailyLevelNode, DailyLevelNode) {
	void onClaimReward(CCObject* sender) {
		if (!Mod::get()->getSettingValue<bool>("enabled")) return DailyLevelNode::onClaimReward(sender);
		if (!m_level) return DailyLevelNode::onClaimReward(sender);

		const int glmEventID = GameLevelManager::get()->m_eventID - 200000;
		const int eventID = m_level->m_dailyID.value() - 200000;

		log::info("[claim] eventID: {}", eventID);
		log::info("[claim] glmEventID: {}", glmEventID);

		DailyLevelNode::onClaimReward(sender);

		SECURE_CONTAIN_PROTECT;
	}
	bool init(GJGameLevel* level, DailyLevelPage* dlp, bool p2) {
		if (!DailyLevelNode::init(level, dlp, p2)) return false;
		if (!Mod::get()->getSettingValue<bool>("enabled")) return true;
		if (!level || !dlp) return true;

		const int glmEventID = GameLevelManager::get()->m_eventID - 200000;
		const int eventID = level->m_dailyID.value() - 200000;
		log::info("[init] eventID: {}", eventID);
		log::info("[init] glmEventID: {}", glmEventID);
		if (eventID != glmEventID) return true;

		FIND_CREATORLAYER_IF_NULL_RETURN true;
		FIND_EVENT_BUTTON_FROM(cl) true;
		FIND_CHILD_SPRITE_FROM(eventButton) true;
		RESTORE_NODE_TRAITS(childSprite);

		MyCreatorLayer* mcl = static_cast<MyCreatorLayer*>(cl);
		LOAD_LAZY_SPRITE(mcl, eventID, level->m_levelName);

		return true;
	}
};

class $modify(MyDailyLevelPage, DailyLevelPage) {
	void skipDailyLevel(DailyLevelNode* dnn, GJGameLevel* level) {
		if (!level) return DailyLevelPage::skipDailyLevel(dnn, level);
		if (!Mod::get()->getSettingValue<bool>("enabled")) return DailyLevelPage::skipDailyLevel(dnn, level);

		const int glmEventID = GameLevelManager::get()->m_eventID - 200000;
		const int eventID = level->m_dailyID.value() - 200000;

		log::info("[skip] eventID: {}", eventID);
		log::info("[skip] glmEventID: {}", glmEventID);

		DailyLevelPage::skipDailyLevel(dnn, level);

		SECURE_CONTAIN_PROTECT;
	}
};

$on_mod(Loaded) {
	// nudge the game to wake up so that this mod can get the current event level's ID from the save file
	// if this line gets removed, so help me god, i will find your addresses (IPv4, IPv6, email, and IRL)
	GameLevelManager::get()->getGJDailyLevelState(GJTimedLevelType::Event);
	Mod::get()->setLoggingEnabled(Mod::get()->getSettingValue<bool>("logging"));
	listenForSettingChanges<bool>("logging", [](const bool newLogging) {
		Mod::get()->setLoggingEnabled(newLogging);
	});
}