#include "AudioSystem.h"
#include "Vfs.h"
#include <iostream>
#include <vector>
#include <algorithm>

namespace HeavyWeapon {

struct SoundMeta {
    const char* name;
    float defaultVolume;
};

static const SoundMeta sSoundMetaTable[] = {
    { "gunpowerup", 0.50f },
    { "tankfire", 0.70f },
    { "tankfire1", 0.60f },
    { "tankfire2", 0.60f },
    { "tankfire3", 0.80f },
    { "tankfire4", 0.70f },
    { "bigexplode", 1.00f },
    { "smallexplode", 0.60f },
    { "bombfall", 0.70f },
    { "bullethit", 0.50f },
    { "tankexplode", 0.90f },
    { "missile", 0.70f },
    { "nukeblast", 0.90f },
    { "powerup", 0.50f },
    { "sparking", 1.00f },
    { "laser", 0.50f },
    { "bossblast", 1.00f },
    { "buttondown", 0.50f },
    { "buttonup", 0.50f },
    { "smallmissile", 0.60f },
    { "tanklaser", 0.50f },
    { "denied", 0.50f },
    { "stats", 0.50f },
    { "alert", 0.50f },
    { "pupcopter", 0.70f },
    { "star", 0.70f },
    { "laserpowerup", 0.50f },
    { "satlaser", 0.50f },
    { "megalaser", 0.70f },
    { "enemytankgun", 0.80f },
    { "shorting", 0.35f },
    { "statichit", 0.50f },
    { "staticshot", 0.50f },
    { "energyfire", 0.70f },
    { "enemyfire", 1.00f },
    { "v_atomictank", 0.80f },
    { "v_gameover", 0.80f },
    { "v_getready", 0.80f },
    { "v_levelcomplete", 0.80f },
    { "v_megalaser", 0.80f },
    { "v_prepare", 0.80f },
    { "v_danger", 0.80f },
    { "orbhit", 0.50f },
    { "flak", 0.50f },
    { "airraid", 0.60f },
    { "diesel", 0.40f }
};

std::unordered_map<std::string, Mix_Chunk*> AudioSystem::sSounds;
Mix_Music* AudioSystem::sCurrentMusic = nullptr;
int AudioSystem::sMusicVolume = 100;
int AudioSystem::sSfxVolume = 100;
int AudioSystem::sEngineChannel = -1;

void AudioSystem::Init() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "[AudioSystem] Mix_OpenAudio error: " << Mix_GetError() << std::endl;
        return;
    }
    Mix_AllocateChannels(32);
    Mix_Volume(-1, sSfxVolume);
    Mix_VolumeMusic(sMusicVolume);
}

void AudioSystem::Shutdown() {
    StopMusic();
    if (sEngineChannel >= 0) {
        Mix_HaltChannel(sEngineChannel);
        sEngineChannel = -1;
    }
    for (auto& pair : sSounds) {
        if (pair.second) {
            Mix_FreeChunk(pair.second);
        }
    }
    sSounds.clear();
    Mix_CloseAudio();
}

float AudioSystem::GetDefaultVolume(const std::string& name) {
    for (const auto& meta : sSoundMetaTable) {
        if (name == meta.name) {
            return meta.defaultVolume;
        }
    }
    return 0.7f;
}

void AudioSystem::PreloadSound(const std::string& name) {
    if (sSounds.find(name) != sSounds.end()) return;

    std::string path = "Sounds/" + name;
    if (path.find('.') == std::string::npos) {
        path += ".ogg";
    }

    std::string resolved = Vfs::Resolve(path);
    if (!Vfs::Exists(resolved)) {
        return;
    }

    Mix_Chunk* chunk = Mix_LoadWAV(resolved.c_str());
    if (chunk) {
        sSounds[name] = chunk;
    }
}

void AudioSystem::PlaySound(const std::string& name, float volumeMultiplier, int loops) {
    auto it = sSounds.find(name);
    if (it == sSounds.end()) {
        PreloadSound(name);
        it = sSounds.find(name);
    }

    if (it != sSounds.end() && it->second) {
        float baseVol = GetDefaultVolume(name);
        int vol = (int)(sSfxVolume * baseVol * volumeMultiplier);
        if (vol < 0) vol = 0;
        if (vol > 128) vol = 128;
        Mix_VolumeChunk(it->second, vol);
        Mix_PlayChannel(-1, it->second, loops);
    }
}

void AudioSystem::UpdateEngineSound(bool moving) {
    if (moving) {
        if (sEngineChannel < 0 || !Mix_Playing(sEngineChannel)) {
            auto it = sSounds.find("diesel");
            if (it == sSounds.end()) {
                PreloadSound("diesel");
                it = sSounds.find("diesel");
            }
            if (it != sSounds.end() && it->second) {
                Mix_VolumeChunk(it->second, (int)(sSfxVolume * 0.35f));
                sEngineChannel = Mix_PlayChannel(-1, it->second, -1); // Loop indefinitely
            }
        }
    } else {
        if (sEngineChannel >= 0 && Mix_Playing(sEngineChannel)) {
            Mix_HaltChannel(sEngineChannel);
            sEngineChannel = -1;
        }
    }
}

void AudioSystem::PlayMusic(const std::string& path, bool loop) {
    StopMusic();

    std::string resolved = Vfs::Resolve(path);
    if (!Vfs::Exists(resolved)) {
        std::cerr << "[AudioSystem] Music not found: " << path << std::endl;
        return;
    }

    sCurrentMusic = Mix_LoadMUS(resolved.c_str());
    if (sCurrentMusic) {
        Mix_VolumeMusic(sMusicVolume);
        Mix_PlayMusic(sCurrentMusic, loop ? -1 : 1);
    } else {
        std::cerr << "[AudioSystem] Failed to load music: " << Mix_GetError() << std::endl;
    }
}

void AudioSystem::StopMusic() {
    if (sCurrentMusic) {
        Mix_HaltMusic();
        Mix_FreeMusic(sCurrentMusic);
        sCurrentMusic = nullptr;
    }
}

void AudioSystem::PauseMusic() {
    Mix_PauseMusic();
}

void AudioSystem::ResumeMusic() {
    Mix_ResumeMusic();
}

void AudioSystem::SetMusicVolume(int volume) {
    sMusicVolume = std::clamp(volume, 0, 128);
    Mix_VolumeMusic(sMusicVolume);
}

void AudioSystem::SetSfxVolume(int volume) {
    sSfxVolume = std::clamp(volume, 0, 128);
    Mix_Volume(-1, sSfxVolume);
}

} // namespace HeavyWeapon
