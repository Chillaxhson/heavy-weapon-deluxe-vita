#include "XmlLoader.h"
#include "Vfs.h"
#include <iostream>
#include <cassert>

using namespace HeavyWeapon;

int main() {
    std::cout << "=== Running Heavy Weapon XML Loader Tests ===" << std::endl;
    Vfs::Init("Heavy Weapon Deluxe/");

    // 1. Craft definitions
    std::unordered_map<std::string, CraftDef> crafts;
    bool okCraft = XmlLoader::LoadCrafts("data/craft.xml", crafts);
    assert(okCraft && "Failed to load craft.xml");
    assert(crafts.size() >= 15 && "Expected at least 15 craft definitions");
    std::cout << "[PASS] Loaded " << crafts.size() << " craft definitions." << std::endl;
    assert(crafts.find("PROPFIGHTER") != crafts.end());
    assert(crafts.find("BOMBER") != crafts.end());
    assert(crafts["PROPFIGHTER"].points == 50);
    assert(crafts["PROPFIGHTER"].armor == 1);
    std::cout << "[PASS] Craft attribute validation passed." << std::endl;

    // 2. Levels definitions
    std::vector<LevelDef> levels;
    bool okLevels = XmlLoader::LoadLevels("data/levels.xml", levels);
    assert(okLevels && "Failed to load levels.xml");
    assert(levels.size() == 19 && "Expected exactly 19 campaign missions");
    std::cout << "[PASS] Loaded " << levels.size() << " campaign missions." << std::endl;
    assert(levels[0].name == "FRIGISTAN");
    assert(levels[0].length == 10000);
    assert(levels[0].intelList.size() == 2);
    assert(levels[18].name == "RED STAR HQ");
    std::cout << "[PASS] Level attributes and intel briefings validated." << std::endl;

    // 3. Waves scheduling
    bool okWaves = XmlLoader::LoadWaves("data/waves.xml", levels);
    assert(okWaves && "Failed to load waves.xml");
    size_t totalWaves = 0;
    for (size_t i = 0; i < levels.size(); ++i) {
        assert(levels[i].waves.size() > 0 && "Level has no wave definitions");
        totalWaves += levels[i].waves.size();
        for (const auto& w : levels[i].waves) {
            for (const auto& entry : w.craftList) {
                assert(crafts.find(entry.craftId) != crafts.end() && "Unknown craft ID in wave");
                assert(entry.quantity > 0);
            }
        }
    }
    std::cout << "[PASS] Verified " << totalWaves << " scheduled waves across all 19 missions." << std::endl;

    // 4. Bosses definitions
    std::unordered_map<std::string, BossDef> bosses;
    bool okBosses = XmlLoader::LoadBosses("data/bosses.xml", bosses);
    assert(okBosses && "Failed to load bosses.xml");
    assert(bosses.size() >= 5 && "Expected at least 5 boss types");
    std::cout << "[PASS] Loaded " << bosses.size() << " multi-tier boss definitions." << std::endl;
    assert(bosses.find("Helicopter") != bosses.end());
    assert(bosses["Helicopter"].levels.size() >= 2);
    assert(bosses["Helicopter"].levels[0].armor == 300);

    // 5. Anims definitions
    std::vector<std::vector<AnimDef>> anims;
    bool okAnims = XmlLoader::LoadAnims("Images/Anims/Anims.xml", anims);
    assert(okAnims && "Failed to load Anims.xml");
    assert(anims.size() >= 9 && "Expected at least 9 level animation configurations");
    size_t totalAnimCount = 0;
    for (const auto& la : anims) totalAnimCount += la.size();
    std::cout << "[PASS] Loaded " << totalAnimCount << " ambient animations across " << anims.size() << " levels." << std::endl;

    std::cout << "\n>>> ALL UNIT & INTEGRATION TESTS PASSED (100%) <<<\n" << std::endl;
    return 0;
}
