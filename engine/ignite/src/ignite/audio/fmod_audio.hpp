#pragma once

#include <fmod.hpp>
#include <fmod_common.h>

#include <string>
#include <unordered_map>

#include "ignite/core/base.hpp"
#include "ignite/core/types.hpp"

#define FMOD_CHECK(x) if ((x) != FMOD_OK) DEBUGBREAK();

namespace ignite
{
    struct FmodSound;
    
    class FmodAudio
    {
    public:
        static void Init();
        static void Shutdown();

        static void SetMasterVolume(float volume);
        static void MuteMaster(bool mute);

        static void Update(float deltaTime);

        static FMOD::ChannelGroup *CreateChannelGroup(const std::string &name);
        static std::unordered_map<std::string, FMOD::ChannelGroup *> GetChannelGroupMap();
        
        static FMOD::ChannelGroup *GetChannelGroup(const std::string &name);
        static FmodAudio &GetInstance();
        static FMOD::System *GetFmodSystem();
        static FMOD::ChannelGroup *GetMasterChannel();
        static float GetMasterVolume();
        static void InsertFmodSound(const std::string &name, const Ref<FmodSound>& sound);
        static void RemoveFmodSound(const std::string &name);

    private:
        FMOD::System *m_System;
        FMOD::ChannelGroup *m_MasterGroup;
        std::unordered_map<std::string, FMOD::ChannelGroup *> m_ChannelGroups;
        std::unordered_map<std::string, Ref<FmodSound>> m_SoundMap;

        FMOD_VECTOR listenerPos;
        FMOD_VECTOR listenerVel;
        FMOD_VECTOR listenerForward;
        FMOD_VECTOR listenerUp;
        
    };
}