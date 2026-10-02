#include "XmlLoader.h"
#include "Vfs.h"
#include <tinyxml2.h>
#include <iostream>
#include <algorithm>

namespace HeavyWeapon {

using namespace tinyxml2;

bool XmlLoader::LoadCrafts(const std::string& path, std::unordered_map<std::string, CraftDef>& outCrafts) {
    std::string fullPath = Vfs::Resolve(path);
    XMLDocument doc;
    if (doc.LoadFile(fullPath.c_str()) != XML_SUCCESS) {
        std::cerr << "[XmlLoader] Failed to load craft file: " << fullPath << std::endl;
        return false;
    }

    outCrafts.clear();
    for (XMLElement* elem = doc.FirstChildElement("Craft"); elem != nullptr; elem = elem->NextSiblingElement("Craft")) {
        CraftDef craft;
        const char* name = elem->Attribute("name");
        const char* desc = elem->Attribute("desc");
        const char* arms = elem->Attribute("arms");
        if (name) craft.name = name;
        if (desc) craft.desc = desc;
        if (arms) craft.arms = arms;
        elem->QueryIntAttribute("points", &craft.points);
        elem->QueryIntAttribute("armor", &craft.armor);

        if (!craft.name.empty()) {
            outCrafts[craft.name] = craft;
        }
    }
    std::cout << "[XmlLoader] Loaded " << outCrafts.size() << " craft definitions." << std::endl;
    return true;
}

bool XmlLoader::LoadLevels(const std::string& path, std::vector<LevelDef>& outLevels) {
    std::string fullPath = Vfs::Resolve(path);
    XMLDocument doc;
    if (doc.LoadFile(fullPath.c_str()) != XML_SUCCESS) {
        std::cerr << "[XmlLoader] Failed to load levels file: " << fullPath << std::endl;
        return false;
    }

    outLevels.clear();
    for (XMLElement* elem = doc.FirstChildElement("Level"); elem != nullptr; elem = elem->NextSiblingElement("Level")) {
        LevelDef level;
        const char* name = elem->Attribute("name");
        if (name) level.name = name;
        elem->QueryIntAttribute("length", &level.length);

        std::string lowerName = level.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        level.bgTheme = lowerName;

        for (XMLElement* intel = elem->FirstChildElement("Intel"); intel != nullptr; intel = intel->NextSiblingElement("Intel")) {
            const char* text = intel->Attribute("text");
            if (text) {
                level.intelList.push_back({ text });
            }
        }
        outLevels.push_back(level);
    }
    std::cout << "[XmlLoader] Loaded " << outLevels.size() << " level definitions." << std::endl;
    return true;
}

bool XmlLoader::LoadWaves(const std::string& path, std::vector<LevelDef>& inOutLevels) {
    std::string fullPath = Vfs::Resolve(path);
    XMLDocument doc;
    if (doc.LoadFile(fullPath.c_str()) != XML_SUCCESS) {
        std::cerr << "[XmlLoader] Failed to load waves file: " << fullPath << std::endl;
        return false;
    }

    size_t levelIdx = 0;
    for (XMLElement* lvlElem = doc.FirstChildElement("Level"); lvlElem != nullptr && levelIdx < inOutLevels.size();
         lvlElem = lvlElem->NextSiblingElement("Level"), levelIdx++) {
        
        inOutLevels[levelIdx].waves.clear();
        for (XMLElement* wElem = lvlElem->FirstChildElement("Wave"); wElem != nullptr; wElem = wElem->NextSiblingElement("Wave")) {
            WaveDef wave;
            wElem->QueryIntAttribute("length", &wave.length);

            for (XMLElement* cElem = wElem->FirstChildElement("Craft"); cElem != nullptr; cElem = cElem->NextSiblingElement("Craft")) {
                const char* id = cElem->Attribute("id");
                int qty = 1;
                cElem->QueryIntAttribute("qty", &qty);
                if (id) {
                    wave.craftList.push_back({ id, qty });
                }
            }
            inOutLevels[levelIdx].waves.push_back(wave);
        }
    }
    std::cout << "[XmlLoader] Assigned wave data to " << levelIdx << " levels." << std::endl;
    return true;
}

bool XmlLoader::LoadBosses(const std::string& path, std::unordered_map<std::string, BossDef>& outBosses) {
    std::string fullPath = Vfs::Resolve(path);
    XMLDocument doc;
    if (doc.LoadFile(fullPath.c_str()) != XML_SUCCESS) {
        std::cerr << "[XmlLoader] Failed to load bosses file: " << fullPath << std::endl;
        return false;
    }

    outBosses.clear();
    for (XMLElement* bossElem = doc.FirstChildElement(); bossElem != nullptr; bossElem = bossElem->NextSiblingElement()) {
        BossDef boss;
        boss.type = bossElem->Name();
        const char* info = bossElem->Attribute("info");
        if (info) boss.info = info;

        for (int lvl = 1; lvl <= 3; ++lvl) {
            std::string lvlTag = "Level" + std::to_string(lvl);
            XMLElement* lvlElem = bossElem->FirstChildElement(lvlTag.c_str());
            if (!lvlElem) continue;

            BossLevelDef bLevel;
            lvlElem->QueryIntAttribute("armor", &bLevel.armor);
            lvlElem->QueryIntAttribute("score", &bLevel.score);
            lvlElem->QueryIntAttribute("turretarmor", &bLevel.turretArmor);
            lvlElem->QueryIntAttribute("fire", &bLevel.fireInterval);

            // Turrets
            for (XMLElement* tElem = lvlElem->FirstChildElement("Turret"); tElem != nullptr; tElem = tElem->NextSiblingElement("Turret")) {
                BossTurretDef turret;
                tElem->QueryIntAttribute("armor", &turret.armor);
                tElem->QueryIntAttribute("fire", &turret.fireInterval);
                tElem->QueryFloatAttribute("speed", &turret.speed);
                const char* stat = tElem->Attribute("stationary");
                turret.stationary = (stat && std::string(stat) == "yes");
                bLevel.turrets.push_back(turret);
            }

            // Launchers
            for (XMLElement* lElem = lvlElem->FirstChildElement("Launcher"); lElem != nullptr; lElem = lElem->NextSiblingElement("Launcher")) {
                BossLauncherDef launcher;
                lElem->QueryIntAttribute("armor", &launcher.armor);
                lElem->QueryIntAttribute("fire", &launcher.fireInterval);
                lElem->QueryFloatAttribute("speed", &launcher.speed);
                bLevel.launchers.push_back(launcher);
            }

            // Dishes / Special weapons
            XMLElement* dishElem = lvlElem->FirstChildElement("Dish");
            if (dishElem) {
                dishElem->QueryIntAttribute("down", &bLevel.dishDown);
                dishElem->QueryIntAttribute("up", &bLevel.dishUp);
                dishElem->QueryIntAttribute("meteors", &bLevel.dishMeteors);
            }

            boss.levels.push_back(bLevel);
        }
        outBosses[boss.type] = boss;
    }
    std::cout << "[XmlLoader] Loaded " << outBosses.size() << " boss definitions." << std::endl;
    return true;
}

bool XmlLoader::LoadAnims(const std::string& path, std::vector<std::vector<AnimDef>>& outLevelAnims) {
    std::string fullPath = Vfs::Resolve(path);
    XMLDocument doc;
    if (doc.LoadFile(fullPath.c_str()) != XML_SUCCESS) {
        std::cerr << "[XmlLoader] Failed to load anims file: " << fullPath << std::endl;
        return false;
    }

    outLevelAnims.clear();
    for (XMLElement* lvlElem = doc.FirstChildElement("Level"); lvlElem != nullptr; lvlElem = lvlElem->NextSiblingElement("Level")) {
        std::vector<AnimDef> anims;
        for (XMLElement* aElem = lvlElem->FirstChildElement("Anim"); aElem != nullptr; aElem = aElem->NextSiblingElement("Anim")) {
            AnimDef anim;
            const char* name = aElem->Attribute("name");
            const char* type = aElem->Attribute("type");
            if (name) anim.name = name;
            if (type) anim.type = type;

            aElem->QueryIntAttribute("frames", &anim.frames);
            aElem->QueryFloatAttribute("speed", &anim.speed);
            aElem->QueryIntAttribute("plane", &anim.plane);
            aElem->QueryIntAttribute("offset", &anim.offset);
            aElem->QueryIntAttribute("y", &anim.y);
            aElem->QueryFloatAttribute("mx", &anim.mx);

            const char* nuke = aElem->Attribute("nuke");
            anim.nuke = (nuke && std::string(nuke) == "yes");

            const char* rare = aElem->Attribute("rare");
            anim.rare = (rare && std::string(rare) == "yes");

            for (XMLElement* dElem = aElem->FirstChildElement("Delay"); dElem != nullptr; dElem = dElem->NextSiblingElement("Delay")) {
                AnimDelayDef delay;
                dElem->QueryIntAttribute("frame", &delay.frame);
                dElem->QueryFloatAttribute("speed", &delay.speed);
                anim.delays.push_back(delay);
            }
            anims.push_back(anim);
        }
        outLevelAnims.push_back(anims);
    }
    std::cout << "[XmlLoader] Loaded anims for " << outLevelAnims.size() << " levels." << std::endl;
    return true;
}

} // namespace HeavyWeapon
