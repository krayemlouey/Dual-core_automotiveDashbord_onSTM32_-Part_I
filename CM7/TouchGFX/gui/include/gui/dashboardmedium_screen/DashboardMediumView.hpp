#ifndef DASHBOARDMEDIUMVIEW_HPP
#define DASHBOARDMEDIUMVIEW_HPP

#include <gui_generated/dashboardmedium_screen/DashboardMediumViewBase.hpp>
#include <gui/dashboardmedium_screen/DashboardMediumPresenter.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>

class DashboardMediumView : public DashboardMediumViewBase
{
public:
    DashboardMediumView();
    virtual ~DashboardMediumView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void bpmButtonClicked();
    virtual void timeButtonClicked();
    virtual void distanceButtonClicked();
    virtual void weatherButtonClicked();
    virtual void modeButtonClicked();
    virtual void languageButtonClicked();
    virtual void closeLanguageSelectorClicked();
    virtual void languageChanged();

private:
    void setOrangeBattery();
    void setGreenBattery();
    void updateBatteryLevel();
    void updateTripInfo();
    void updateTheme();

    uint16_t oldBPM;
    uint8_t oldMinute;
    uint8_t oldSecond;
    int oldDistance;
    bool alreadyOrange;
    uint16_t oldBitmapID;
    uint16_t dynamicBmpLanguageSelector;
    bool languageSelectorMoving;
    float oldLidar;
    int oldTemp;
    int oldFuel;

    touchgfx::TextAreaWithOneWildcard lidarValue;
    static const uint16_t LIDARVALUE_SIZE = 8;
    touchgfx::Unicode::UnicodeChar lidarValueBuffer[LIDARVALUE_SIZE];
};

#endif // DASHBOARDMEDIUMVIEW_HPP
