#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelCell.hpp>

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

    // splitmix64: cheap, well distributed, and fully deterministic so a level
    // always gets the same pronouns on every launch.
    uint64_t mix(uint64_t x) {
        x += 0x9E3779B97F4A7C15ull;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        return x ^ (x >> 31);
    }

    std::string pronounsFor(GJGameLevel* level) {
        uint64_t seed = static_cast<uint64_t>(level->m_levelID.value());
        if (seed == 0) {
            // Local / editor levels have no online ID, so fall back to the name.
            seed = std::hash<std::string>{}(std::string(level->m_levelName));
        }

        uint64_t const h = mix(seed);
        bool const isNeo = (h % 100) < NEO_CHANCE;

        if (isNeo && Mod::get()->getSettingValue<bool>("neopronouns")) {
            return std::string(NEO_PRONOUNS[(h >> 8) % NEO_PRONOUNS.size()]);
        }
        // Neopronoun levels fall back to a normal set when the setting is off.
        return std::string(NORMAL_PRONOUNS[(h >> 24) % NORMAL_PRONOUNS.size()]);
    }

    // Puts a pronoun label directly to the right of `anchor` (the creator name),
    // as a child of `host`. Works regardless of how `anchor`'s parent is laid out.
    void attachLabel(CCNode* host, CCNode* anchor, GJGameLevel* level, ccColor3B color, float scale) {
        auto const id = "pronoun-label"_spr;

        if (auto old = host->getChildByID(id)) {
            old->removeFromParent();
        }
        if (!anchor || !anchor->getParent()) return;

        auto label = CCLabelBMFont::create(pronounsFor(level).c_str(), "bigFont.fnt");
        label->setID(id);
        label->setScale(scale);
        label->setColor(color);
        label->setAnchorPoint({0.f, 0.5f});

        // Right-middle edge of the anchor, in the anchor's parent space.
        auto const size = anchor->getScaledContentSize();
        auto const ap = anchor->getAnchorPoint();
        CCPoint edge = {
            anchor->getPositionX() + size.width * (1.f - ap.x),
            anchor->getPositionY() + size.height * (0.5f - ap.y)
        };

        auto world = anchor->getParent()->convertToWorldSpace(edge);
        auto local = host->convertToNodeSpace(world);
        local.x += 5.f;

        label->setPosition(local);
        host->addChild(label, 10);
    }
}

class $modify(PronounLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;
        if (!level) return true;

        auto creator = this->getChildByIDRecursive("creator-name");
        if (!creator) {
            log::warn("creator-name not found in LevelInfoLayer (is geode.node-ids up to date?)");
            return true;
        }

        attachLabel(this, creator, level, {255, 255, 255}, 0.3f);
        return true;
    }
};

class $modify(PronounLevelCell, LevelCell) {
    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);
        if (!level || !m_mainLayer) return;

        // Cells are recycled by the list view, so always clear the old label first.
        auto const id = "pronoun-label"_spr;
        if (auto old = m_mainLayer->getChildByID(id)) {
            old->removeFromParent();
        }

        auto creator = m_mainLayer->getChildByIDRecursive("creator-name");
        if (!creator) return; // e.g. unregistered creators / local levels

        bool const white = Mod::get()->getSettingValue<bool>("white-text");
        ccColor3B const color = white ? ccColor3B{255, 255, 255} : ccColor3B{0, 0, 0};

        attachLabel(m_mainLayer, creator, level, color, m_compactView ? 0.25f : 0.3f);
    }
};
