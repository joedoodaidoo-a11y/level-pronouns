#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelCell.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

using namespace geode::prelude;

namespace {
    constexpr std::array<std::string_view, 7> NORMAL_PRONOUNS = {
        "She/Her", "He/Him", "They/Them", "He/They", "She/They", "It/Its", "Any/All"
    };

    constexpr std::array<std::string_view, 7> NEO_PRONOUNS = {
        "Ze/Hir", "Ze/Zir", "Xe/Xem", "E/Eir", "Ey/Em", "Per/Per", "Fae/Faer"
    };

    // Chance (percent) that a level gets a neopronoun set.
    constexpr uint64_t NEO_CHANCE = 20;

    // splitmix64: deterministic, so a level always gets the same pronouns.
    uint64_t mix(uint64_t x) {
        x += 0x9E3779B97F4A7C15ull;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        return x ^ (x >> 31);
    }

    std::string pronounsFor(GJGameLevel* level) {
        uint64_t seed = static_cast<uint64_t>(level->m_levelID.value());
        if (seed == 0) {
            seed = std::hash<std::string>{}(std::string(level->m_levelName));
        }

        uint64_t const h = mix(seed);
        bool const isNeo = (h % 100) < NEO_CHANCE;

        if (isNeo && Mod::get()->getSettingValue<bool>("neopronouns")) {
            return std::string(NEO_PRONOUNS[(h >> 8) % NEO_PRONOUNS.size()]);
        }
        return std::string(NORMAL_PRONOUNS[(h >> 24) % NORMAL_PRONOUNS.size()]);
    }

    // Bounding box of `node`, expressed in `host`'s coordinate space.
    CCRect boundsIn(CCNode* host, CCNode* node) {
        auto const box = node->boundingBox(); // in node's parent space
        auto* parent = node->getParent();
        if (!parent) return box;

        auto a = host->convertToNodeSpace(parent->convertToWorldSpace({box.getMinX(), box.getMinY()}));
        auto b = host->convertToNodeSpace(parent->convertToWorldSpace({box.getMaxX(), box.getMaxY()}));

        return CCRect(
            std::min(a.x, b.x), std::min(a.y, b.y),
            std::abs(b.x - a.x), std::abs(b.y - a.y)
        );
    }

    CCLabelBMFont* makeLabel(CCNode* host, GJGameLevel* level, ccColor3B color, float scale) {
        auto const id = "pronoun-label"_spr;
        if (auto old = host->getChildByID(id)) {
            old->removeFromParent();
        }
        auto label = CCLabelBMFont::create(pronounsFor(level).c_str(), "bigFont.fnt");
        label->setID(id);
        label->setScale(scale);
        label->setColor(color);
        return label;
    }
}

class $modify(PronounLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;
        if (!level) return true;

        auto title = this->getChildByIDRecursive("title-label");
        auto creator = this->getChildByIDRecursive("creator-name");
        if (!creator && !title) {
            log::warn("title-label / creator-name not found in LevelInfoLayer (is geode.node-ids up to date?)");
            return true;
        }

        auto label = makeLabel(this, level, {255, 255, 255}, 0.45f);
        label->setAnchorPoint({0.5f, 0.5f});

        // Centered between the level title and the creator name.
        float x, y;
        if (title && creator) {
            auto t = boundsIn(this, title);
            auto c = boundsIn(this, creator);
            x = t.getMidX();
            y = (t.getMinY() + c.getMaxY()) / 2.f;
        }
        else if (title) {
            auto t = boundsIn(this, title);
            x = t.getMidX();
            y = t.getMinY() - 10.f;
        }
        else {
            auto c = boundsIn(this, creator);
            x = c.getMidX();
            y = c.getMaxY() + 10.f;
        }

        label->setPosition({x, y});
        this->addChild(label, 10);
        return true;
    }
};

class $modify(PronounLevelCell, LevelCell) {
    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);
        if (!level || !m_mainLayer) return;

        // Cells are recycled by the list view, so always clear the old label first.
        if (auto old = m_mainLayer->getChildByID("pronoun-label"_spr)) {
            old->removeFromParent();
        }

        auto creator = m_mainLayer->getChildByIDRecursive("creator-name");
        if (!creator) return; // e.g. unregistered creators / local levels

        auto const c = boundsIn(m_mainLayer, creator);
        float rightEdge = c.getMaxX();

        // Stay clear of the copy / high-object-count icons if they sit on the
        // same line as the creator name.
        for (auto* indicatorId : {"copy-indicator", "high-object-indicator"}) {
            auto* indicator = m_mainLayer->getChildByIDRecursive(indicatorId);
            if (!indicator || !indicator->isVisible()) continue;

            auto const r = boundsIn(m_mainLayer, indicator);
            if (std::abs(r.getMidY() - c.getMidY()) < std::max(c.size.height, r.size.height)) {
                rightEdge = std::max(rightEdge, r.getMaxX());
            }
        }

        bool const white = Mod::get()->getSettingValue<bool>("white-text");
        ccColor3B const color = white ? ccColor3B{255, 255, 255} : ccColor3B{0, 0, 0};

        auto label = makeLabel(m_mainLayer, level, color, m_compactView ? 0.25f : 0.3f);
        label->setAnchorPoint({0.f, 0.5f});
        label->setPosition({rightEdge + 6.f, c.getMidY()});
        m_mainLayer->addChild(label, 10);
    }
};
