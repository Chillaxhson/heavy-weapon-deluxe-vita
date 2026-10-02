#pragma once

#include <string>
#include <unordered_map>
#include <SDL2/SDL_mixer.h>

namespace HeavyWeapon {

class AudioSystem {
public:
    static void Init();
    static void Shutdown();

    // Sound effects
    static void PlaySound(const std::string& name, float volumeMultiplier = 1.0f, int loops = 0);
    static void PreloadSound(const std::string& name);

    // Engine diesel sound loop
    static void UpdateEngineSound(bool moving);

    // Music
    static void PlayMusic(const std::string& path, bool loop = true);
    static void StopMusic();
    static void PauseMusic();
    static void ResumeMusic();

    static void SetMusicVolume(int volume); // 0 to 128
    static void SetSfxVolume(int volume);   // 0 to 128

private:
    static std::unordered_map<std::string, Mix_Chunk*> sSounds;
    static Mix_Music* sCurrentMusic;
    static int sMusicVolume;
    static int sSfxVolume;
    static int sEngineChannel;

    static float GetDefaultVolume(const std::string& name);
};

} // namespace HeavyWeapon
