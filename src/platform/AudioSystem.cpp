#include "AudioSystem.h"
#include "Vfs.h"
#include <iostream>

namespace HeavyWeapon {

std::unordered_map<std::string, Mix_Chunk*> AudioSystem::sSounds;
Mix_Music* AudioSystem::sCurrentMusic = nullptr;
int AudioSystem::sMusicVolume = 100;
int AudioSystem::sSfxVolume = 100;

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
    for (auto& pair : sSounds) {
        if (pair.second) {
            Mix_FreeChunk(pair.second);
        }
    }
    sSounds.clear();
    Mix_CloseAudio();
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

void AudioSystem::PlaySound(const std::string& name, float volume, int loops) {
    auto it = sSounds.find(name);
    if (it == sSounds.end()) {
        PreloadSound(name);
        it = sSounds.find(name);
    }

    if (it != sSounds.end() && it->second) {
        int vol = (int)(sSfxVolume * volume);
        if (vol < 0) vol = 0;
        if (vol > 128) vol = 128;
        Mix_VolumeChunk(it->second, vol);
        Mix_PlayChannel(-1, it->second, loops);
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
    sMusicVolume = volume;
    Mix_VolumeMusic(sMusicVolume);
}

void AudioSystem::SetSfxVolume(int volume) {
    sSfxVolume = volume;
    Mix_Volume(-1, sSfxVolume);
}

} // namespace HeavyWeapon
