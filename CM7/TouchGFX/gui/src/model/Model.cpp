#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <cmath>
#ifndef SIMULATOR
#include "main.h"
#include "AudioPlayer.h"

// Variable globale hors classe
volatile bool new_speed_available = false;

// Callback HSEM — déclenché quand le CM4 relâche HSEM(1)
extern "C" void HAL_HSEM_FreeCallback(uint32_t SemMask)
{
    if (SemMask & __HAL_HSEM_SEMID_TO_MASK(1))
    {
        new_speed_available = true;
    }
}
#endif

Model::Model() :
    modelListener(0), hour(0), minute(0), second(0), bpm(0),
    distance(0), speed(0.0), temperature(0.0), fuel(0.0), power(16.0),
    lidar(0.0),
    gaugeButtonSelected(time), batteryPct(100), isLight(false),
    selectedWeatherItem(12), selectedScreenSize(smallSize), count(1),
    audioTickCounter(0)
{}

void Model::tick()
{
#ifndef SIMULATOR
    // Lecture directe en polling à chaque tick (plus robuste que l'IRQ HSEM)
    SCB_InvalidateDCache_by_Addr((uint32_t*)0x38000000, 32); // étendu à 32 octets (couvre lidar offset 24)
    speed       = *(volatile float*)0x38000000;
    temperature = *(volatile float*)0x38000004;
    fuel        = *(volatile float*)0x38000008;
    lidar       = *(volatile float*)0x38000018; // offset 24 : distance LiDAR en cm
    
    if (modelListener) {
        modelListener->speedUpdated(speed);
    }

    /* ─── Mise à jour audio (~1 fois par seconde) ─────────────────── */
    audioTickCounter++;
    if (audioTickCounter >= AUDIO_UPDATE_INTERVAL)
    {
        audioTickCounter = 0;
        if (modelListener)
        {
            modelListener->audioInfoUpdated();
        }
    }
#else
    /* Simulation PC : animation sinusoïdale */
    if (!(count % 2))
        speed = 125.0f - 125.0f * cosf(count * 0.005f);
    temperature = 24.0f;
    fuel = 65.0f;
#endif

    power = 17.0f + (4 * sin(count * 0.007f) + 2 * sin(count * 0.008f + 0.5f) + 2 * sin(count * 0.010f + 0.5f));

    if (!(count % 60)) { second++; }
    if (second == 60)  { minute++; second = 0; }
    if (minute == 60)  { hour++;   minute = 0; }
    if (!(count % 4))
        bpm = (uint16_t)(150 + (30 * sin(count / 100 * 0.014f) + 15 * sin(count / 100 * 0.035f) + 15 * sin(count / 100 * 0.060f)));
    distance += speed / 150000.0f;
    if (distance >= 100) { distance = 0; }
    if (!(count % 1000) && batteryPct > 0) { batteryPct--; }

    // Petit jitter subtil pour garder la gauge "vivante" (±0.15°C)
    // Avec le nouveau multiplicateur x5 dans la vue, cela donnera ±0.75% de variation
    float temp_jitter = 0.1f * sinf(count * 0.13f) + 0.05f * sinf(count * 0.07f);
    temperature += temp_jitter;

    count++;
}

uint8_t  Model::getHour()     { return hour;     }
uint8_t  Model::getMinute()   { return minute;   }
uint8_t  Model::getSecond()   { return second;   }
uint16_t Model::getBPM()      { return bpm;      }
float    Model::getDistance() { return distance; }
float    Model::getSpeed()    { return speed; }
float    Model::getTemperature() { return temperature; }
float    Model::getFuel()     { return fuel; }
float    Model::getPower()    { return power;    }
float    Model::getLidar()    { return lidar;    }

void        Model::setSelectedGaugeButton(GaugeButton btn) { gaugeButtonSelected = btn;   }
GaugeButton Model::getSelectedGaugeButton()                { return gaugeButtonSelected;  }

void    Model::setBatteryPct(uint8_t level) { batteryPct = level;  }
uint8_t Model::getBatteryPct()              { return batteryPct;   }

void Model::setLight(bool light) { isLight = light; }
bool Model::getLight()           { return isLight;  }

void Model::setSelectedWeatherItem(int itemNo) { selectedWeatherItem = itemNo;   }
int  Model::getSelectedWeatherItem()           { return selectedWeatherItem;     }

void       Model::setSelectedScreenSize(ScreenSize s) { selectedScreenSize = s;   }
ScreenSize Model::getSelectedScreenSize()             { return selectedScreenSize; }

int Model::getHourTemperature(int hr) { return (int)(12 * pow(sin((hr - 9) / 4.0f), 3) + 9); }
int Model::getHourWindSpeed(int hr)   { return (int)(7 * sin(1 / 8.0f * hr - 2) + 8);        }

/* ═══════════════════════════════════════════════════════════════════════
   Audio player API
   ═══════════════════════════════════════════════════════════════════════ */

void Model::audioActivate()
{
#ifndef SIMULATOR
    AudioInfo_t* info = AudioPlayer_GetInfo();
    AudioState_t state = AudioPlayer_GetState();

    if (state == AUDIO_STATE_IDLE || state == AUDIO_STATE_STOPPED)
    {
        /* Lancer la première chanson si la liste n'est pas vide */
        if (info->total_songs > 0)
        {
            AudioPlayer_Next();   /* AudioPlayer_Next() joue song[current_index+1] */
        }
    }
    else if (state == AUDIO_STATE_PAUSED)
    {
        AudioPlayer_Resume();
    }
    /* Si déjà en lecture, on ne touche à rien */
#endif
}

void Model::audioPause()
{
#ifndef SIMULATOR
    if (AudioPlayer_GetState() == AUDIO_STATE_PLAYING)
    {
        AudioPlayer_Pause();
    }
#endif
}

void Model::audioPlayPause()
{
#ifndef SIMULATOR
    AudioState_t state = AudioPlayer_GetState();
    if (state == AUDIO_STATE_PLAYING)
    {
        AudioPlayer_Pause();
    }
    else if (state == AUDIO_STATE_PAUSED)
    {
        AudioPlayer_Resume();
    }
    else
    {
        /* Idle / Stopped : démarrer */
        AudioInfo_t* info = AudioPlayer_GetInfo();
        if (info->total_songs > 0)
        {
            AudioPlayer_Next();
        }
    }
#endif
}

void Model::audioNext()
{
#ifndef SIMULATOR
    AudioPlayer_Next();
#endif
}

void Model::audioPrev()
{
#ifndef SIMULATOR
    AudioPlayer_Prev();
#endif
}

const char* Model::getAudioSongName()
{
#ifndef SIMULATOR
    return AudioPlayer_GetInfo()->name;
#else
    return "demo.wav";
#endif
}

uint32_t Model::getAudioPosition()
{
#ifndef SIMULATOR
    return AudioPlayer_GetInfo()->position_sec;
#else
    return 0;
#endif
}

uint32_t Model::getAudioDuration()
{
#ifndef SIMULATOR
    return AudioPlayer_GetInfo()->duration_sec;
#else
    return 180;
#endif
}

bool Model::isAudioPlaying()
{
#ifndef SIMULATOR
    return (AudioPlayer_GetState() == AUDIO_STATE_PLAYING);
#else
    return false;
#endif
}
