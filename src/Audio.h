//
// Created by kiyazz on 2/6/25.
//

#ifndef AUDIO_H
#define AUDIO_H

#include <unordered_map>

#include "SDL_mixer.h"
#include "AudioHelper.h"

class Audio {
public:
    static Mix_Chunk* loadAudio(const char* file, const std::string& clip) {
        Mix_Chunk* data = AudioHelper::Mix_LoadWAV(file);
        audioCache[clip] = data;
        return data;
    }

    static void playBackgroundLooped(Mix_Chunk* audio) {
        AudioHelper::Mix_PlayChannel(0, audio, -1);
    }

    static void playAudioOnce(Mix_Chunk* audio) {
        AudioHelper::Mix_PlayChannel(0, audio, 0);
    }

    static void playAudioOnceOnChannel(Mix_Chunk * mix_chunk, int i) {
        AudioHelper::Mix_PlayChannel(i, mix_chunk, 0);
    }

    static void playAudio(int channel, const std::string& clip, bool loop) {
        Mix_Chunk* chunk = nullptr;
        if (const auto it = audioCache.find(clip); it != audioCache.end()) {
            chunk = it->second;
        }
        else {
            std::string file = "resources/audio/" + clip;
            if (std::filesystem::exists(file+".wav")) {
                chunk = loadAudio((file+".wav").c_str(), clip);
            }
            else if (std::filesystem::exists(file+".ogg")) {
                chunk = loadAudio((file+".ogg").c_str(), clip);
            }
            else {
                std::cout << "error: failed to play audio clip " << clip;
                exit(0);
            }
        }
        AudioHelper::Mix_PlayChannel(channel, chunk, loop ? -1 : 0);
    }

    static void haltChannel(int channel) {
        AudioHelper::Mix_HaltChannel(channel);
    }

    static void setVol(int channel, int vol) {
        AudioHelper::Mix_Volume(channel, vol);
    }

private:
    static inline std::unordered_map<std::string, Mix_Chunk*> audioCache = std::unordered_map<std::string, Mix_Chunk*>();
};

#endif //AUDIO_H
