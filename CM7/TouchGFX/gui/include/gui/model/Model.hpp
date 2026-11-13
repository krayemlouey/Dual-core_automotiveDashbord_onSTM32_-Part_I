#ifndef MODEL_HPP
#define MODEL_HPP

#include <touchgfx/hal/types.hpp>
#include <texts/TextKeysAndLanguages.hpp>

class ModelListener;

enum GaugeButton
{
    bpm,
    time,
    distance
};

enum ScreenSize
{
    smallSize,
    mediumSize,
    largeSize
};

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    uint8_t getHour();
    uint8_t getMinute();
    uint8_t getSecond();
    uint16_t getBPM();
    float getDistance();
    float getSpeed();
    float getTemperature();
    float getFuel();
    float getPower();
    float getLidar();
    void setSelectedGaugeButton(GaugeButton btn);
    GaugeButton getSelectedGaugeButton();
    void setBatteryPct(uint8_t batteryLevel);
    uint8_t getBatteryPct();
    void setLight(bool light);
    bool getLight();
    void setSelectedWeatherItem(int itemNo);
    int getSelectedWeatherItem();
    void setSelectedScreenSize(ScreenSize screenSize);
    ScreenSize getSelectedScreenSize();
    int getHourTemperature(int hr);
    int getHourWindSpeed(int hr);

    /* ─── Audio player API ─────────────────────────────────────────── */
    void audioActivate();   /* Appelé par MusicScreenPresenter::activate()   */
    void audioPause();      /* Appelé par MusicScreenPresenter::deactivate() */
    void audioPlayPause();  /* Bascule play / pause                          */
    void audioNext();       /* Chanson suivante                              */
    void audioPrev();       /* Chanson précédente                            */

    const char* getAudioSongName();
    uint32_t    getAudioPosition();
    uint32_t    getAudioDuration();
    bool        isAudioPlaying();

protected:
    ModelListener* modelListener;

private:
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t bpm;
    float distance;
    float speed;
    float temperature;
    float fuel;
    float power;
    float lidar;
    GaugeButton gaugeButtonSelected;
    uint8_t batteryPct;
    bool isLight;
    int selectedWeatherItem;
    ScreenSize selectedScreenSize;
    int count;

    /* ─── Audio tick counter ───────────────────────────────────────── */
    uint16_t audioTickCounter;
    static const uint16_t AUDIO_UPDATE_INTERVAL = 56; /* ~1 s à 56 Hz */
};

#endif // MODEL_HPP
