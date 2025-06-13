#include "fmod_audio.hpp"
#include "fmod_sound.hpp"

#include <ranges>

namespace ignite {

    static FMOD_RESULT result;
    static FmodAudio *s_fmod_audio = nullptr;

    void FmodAudio::SetMasterVolume(const float volume)
    {
        result = s_fmod_audio->m_MasterGroup->setVolume(volume); 
        FMOD_CHECK(result);
    }

    void FmodAudio::MuteMaster(const bool mute)
    {
        result = s_fmod_audio->m_MasterGroup->setMute(mute);
        FMOD_CHECK(result);
    }

    void FmodAudio::Update(const float delta_time)
    {
        s_fmod_audio->m_System->update();
        for (const auto& val : s_fmod_audio->m_SoundMap | std::views::values)
        {
            val->Update(delta_time);
        }
    }

    void FmodAudio::Init()
    {
        s_fmod_audio = new FmodAudio();

        result = FMOD::System_Create(&s_fmod_audio->m_System);
        FMOD_CHECK(result);

        result = s_fmod_audio->m_System->init(32, FMOD_INIT_NORMAL, nullptr);
        FMOD_CHECK(result);

        s_fmod_audio->m_MasterGroup = FmodAudio::CreateChannelGroup("Master");

        // Initialize listener position
        s_fmod_audio->listenerPos = { 0.0f,0.0f,0.0f };
        s_fmod_audio->listenerVel = { 0.0f,0.0f,0.0f };
        s_fmod_audio->listenerForward = { 0.0f,0.0f,1.0f };
        s_fmod_audio->listenerUp = { 0.0f,1.0f,0.0f };
    }

    void FmodAudio::Shutdown()
    {
        for (auto &s : s_fmod_audio->m_SoundMap | std::views::values)
        {
            s->Release();
        }

        s_fmod_audio->m_SoundMap.clear();

        result = s_fmod_audio->m_System->close();
        FMOD_CHECK(result);
        result = s_fmod_audio->m_System->release();
        FMOD_CHECK(result);
        
        delete s_fmod_audio;
    }

    FMOD::ChannelGroup* FmodAudio::CreateChannelGroup(const std::string &name)
    {
        FMOD::ChannelGroup* group = nullptr;
        s_fmod_audio->m_System->createChannelGroup(name.c_str(), &group);
        group->setMode(FMOD_LOOP_NORMAL);
        s_fmod_audio->m_ChannelGroups[name] = group;
        return group;
    }
    
    std::unordered_map<std::string, FMOD::ChannelGroup*> FmodAudio::GetChannelGroupMap()
    {
        return s_fmod_audio->m_ChannelGroups;    
    }

    FMOD::ChannelGroup* FmodAudio::GetChannelGroup(const std::string& name)
    {
        if (s_fmod_audio->m_ChannelGroups.contains(name))
            return s_fmod_audio->m_ChannelGroups[name];
        return nullptr;
    }

    FmodAudio &FmodAudio::GetInstance()
    {
        return *s_fmod_audio;
    }

    FMOD::System* FmodAudio::GetFmodSystem()
    {
        return s_fmod_audio->m_System;
    }

    FMOD::ChannelGroup* FmodAudio::GetMasterChannel()
    {
        return s_fmod_audio->m_MasterGroup;
    }

    float FmodAudio::GetMasterVolume()
    {
        float volume = 0.0f;
        s_fmod_audio->m_MasterGroup->getVolume(&volume);
        return volume;
    }

    void FmodAudio::InsertFmodSound(const std::string &name, const Ref<FmodSound>& sound)
    {
        s_fmod_audio->m_SoundMap[name] = sound;
    }

    void FmodAudio::RemoveFmodSound(const std::string &name)
    {
        if (const auto it = s_fmod_audio->m_SoundMap.find(name); it != s_fmod_audio->m_SoundMap.end())
        {
            s_fmod_audio->m_SoundMap.erase(it);
        }
    }
}