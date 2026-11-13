#include <gui/dashboardmedium_screen/DashboardMediumView.hpp>
#include <images/BitmapDatabase.hpp>
#include <images/SVGDatabase.hpp>
#include <gui/common/Constants.hpp>
#include <touchgfx/Color.hpp>

DashboardMediumView::DashboardMediumView() :
    oldBPM(0), oldMinute(0), oldSecond(0), oldDistance(0),
    alreadyOrange(false), oldBitmapID(0),
    dynamicBmpLanguageSelector(0), languageSelectorMoving(false),
    oldLidar(-999.0f), oldTemp(-1), oldFuel(-1)
{
}

void DashboardMediumView::setupScreen()
{
    switch (presenter->getSelectedButton())
    {
    case bpm:      gaugeButtonsGroup.setSelected(bpmButton);      bpmButtonClicked();      break;
    case distance: gaugeButtonsGroup.setSelected(distanceButton); distanceButtonClicked(); break;
    default: break;
    }
    updateTripInfo();
    updateTheme();
    updateBatteryLevel();
    if (presenter->getPower() > 22) { setOrangeBattery(); gauge.setOrangeGauge(); }
    else                            { setGreenBattery();  gauge.setGreenGauge();  }
    languageSelector.setTouchable(true);
    gauge.setSpeed(presenter->getSpeed());
    gauge.setPower(presenter->getSpeed());
    presenter->setSelectedScreenSize(mediumSize);
    dynamicBmpLanguageSelector = Bitmap::dynamicBitmapCreate(languageSelector.getWidth(), languageSelector.getHeight(), Bitmap::RGB888);
    languageSelector.setCacheBitmap(dynamicBmpLanguageSelector);
    languageSelector.enableCachedMode(true);
    languageSelector.updateCache();
    DashboardMediumViewBase::setupScreen();

    // --- Affichage LiDAR ---
    lidarValue.setPosition(383, 324, 140, 90);
    lidarValue.setColor(touchgfx::Color::getColorFromRGB(181, 215, 253));
    lidarValue.setTypedText(bpmValue.getTypedText());
    lidarValue.setWildcard(lidarValueBuffer);
    add(lidarValue);
    
    lidarValueBuffer[0] = '-'; lidarValueBuffer[1] = '-';
    lidarValueBuffer[2] = '-'; lidarValueBuffer[3] = 0;
    lidarValue.invalidate();
}

void DashboardMediumView::tearDownScreen()
{
    DashboardMediumViewBase::tearDownScreen();
    Bitmap::dynamicBitmapDelete(dynamicBmpLanguageSelector);
}

void DashboardMediumView::handleTickEvent()
{
    gauge.setSpeed(presenter->getSpeed());
    gauge.setPower(presenter->getSpeed());
    
    // Temperature
    int newTemp = (int)presenter->getTemperature();
    if (newTemp != oldTemp) {
        gaugetemp.setValue(newTemp);
        gaugetemp.invalidate();
        oldTemp = newTemp;
    }

    // LiDAR : affichage distance
    float newLidar = presenter->getLidar();
    if (newLidar >= 0.0f) {
        Unicode::snprintf(lidarValueBuffer, LIDARVALUE_SIZE, "%d", (int)newLidar);
        lidarValue.invalidate();
    }

    // Fuel: map 0-100 directly
    int newFuel = (int)presenter->getFuel();
    if (newFuel != oldFuel) {
        gaugefuel.setValue(newFuel);
        gaugefuel.invalidate();
        oldFuel = newFuel;
    }

    if (gauge.isOrange() && !alreadyOrange)       setOrangeBattery();
    else if (!gauge.isOrange() && alreadyOrange)  setGreenBattery();
    updateBatteryLevel();
    updateTripInfo();
    if (languageSelectorMoving && !languageSelector.isMoveAnimationRunning())
    {
        languageSelectorMoving = false;
        languageSelector.enableCachedMode(false);
    }
}

void DashboardMediumView::bpmButtonClicked()
{
    presenter->setSelectedButton(bpm);
    bpmText.invalidate(); timeText.invalidate(); distanceText.invalidate();
    bpmText.setVisible(true);
    timeText.setVisible(false);
    distanceText.setVisible(false);
    bpmText.invalidate(); timeText.invalidate(); distanceText.invalidate();
}

void DashboardMediumView::timeButtonClicked()
{
    presenter->setSelectedButton(time);
    bpmText.invalidate(); timeText.invalidate(); distanceText.invalidate();
    bpmText.setVisible(false);
    timeText.setVisible(true);
    distanceText.setVisible(false);
    bpmText.invalidate(); timeText.invalidate(); distanceText.invalidate();
}

void DashboardMediumView::distanceButtonClicked()
{
    presenter->setSelectedButton(distance);
    bpmText.invalidate(); timeText.invalidate(); distanceText.invalidate();
    bpmText.setVisible(false);
    timeText.setVisible(false);
    distanceText.setVisible(true);
    bpmText.invalidate(); timeText.invalidate(); distanceText.invalidate();
}

void DashboardMediumView::weatherButtonClicked()
{
    // Weather screen not available in this project
}

void DashboardMediumView::modeButtonClicked()
{
    presenter->setLight(!presenter->getLight());
    updateTheme();
}

void DashboardMediumView::languageButtonClicked()
{
    languageSelectorMoving = true;
    languageSelector.enableCachedMode(true);
    languageSelector.updateCache();
    languageSelector.startMoveAnimation(396, 0, 40, EasingEquations::sineEaseInOut);
}

void DashboardMediumView::closeLanguageSelectorClicked()
{
    languageSelectorMoving = true;
    languageSelector.enableCachedMode(true);
    languageSelector.updateCache();
    languageSelector.startMoveAnimation(800, 0, 40, EasingEquations::sineEaseInOut);
}

void DashboardMediumView::languageChanged()
{
    gauge.updateTextAreas();
}

void DashboardMediumView::setOrangeBattery()
{
    batteryIcon.setBitmap(presenter->getLight() ? BITMAP_BATTERY_LIGHTNING_LIGHT_ID : BITMAP_BATTERY_LIGHTNING_ID);
    batteryIcon.invalidate();
    alreadyOrange = true;
}

void DashboardMediumView::setGreenBattery()
{
    batteryIcon.setBitmap(presenter->getLight() ? BITMAP_BATTERY_LEAF_LIGHT_ID : BITMAP_BATTERY_LEAF_ID);
    batteryIcon.invalidate();
    alreadyOrange = false;
}

void DashboardMediumView::updateBatteryLevel()
{
    const uint8_t batteryLvl = presenter->getBatteryPct();
    if (presenter->getLight())
    {
        if      (batteryLvl > 100/6*5) batteryLevel.setBitmap(alreadyOrange ? BITMAP_BATTERY_LEVEL_FULL_POWER_LIGHT_ID : BITMAP_BATTERY_LEVEL_FULL_ECO_LIGHT_ID);
        else if (batteryLvl > 100/6*4) batteryLevel.setBitmap(alreadyOrange ? BITMAP_BATTERY_LEVEL_4_POWER_LIGHT_ID    : BITMAP_BATTERY_LEVEL_4_ECO_LIGHT_ID);
        else if (batteryLvl > 100/6*3) batteryLevel.setBitmap(alreadyOrange ? BITMAP_BATTERY_LEVEL_3_POWER_LIGHT_ID    : BITMAP_BATTERY_LEVEL_3_ECO_LIGHT_ID);
        else if (batteryLvl > 100/6*2) batteryLevel.setBitmap(alreadyOrange ? BITMAP_BATTERY_LEVEL_2_POWER_LIGHT_ID    : BITMAP_BATTERY_LEVEL_2_ECO_LIGHT_ID);
        else if (batteryLvl > 100/6*1) batteryLevel.setBitmap(alreadyOrange ? BITMAP_BATTERY_LEVEL_1_POWER_LIGHT_ID    : BITMAP_BATTERY_LEVEL_1_ECO_LIGHT_ID);
        else                           batteryLevel.setBitmap(BITMAP_BATTERY_LEVEL_LOW_LIGHT_ID);
    }
    else
    {
        if      (batteryLvl > 100/6*5) batteryLevel.setBitmap(alreadyOrange ? BITMAP_BATTERY_LEVEL_FULL_POWER_ID : BITMAP_BATTERY_LEVEL_FULL_ECO_ID);
        else if (batteryLvl > 100/6*4) batteryLevel.setBitmap(alreadyOrange ? BITMAP_BATTERY_LEVEL_4_POWER_ID    : BITMAP_BATTERY_LEVEL_4_ECO_ID);
        else if (batteryLvl > 100/6*3) batteryLevel.setBitmap(alreadyOrange ? BITMAP_BATTERY_LEVEL_3_POWER_ID    : BITMAP_BATTERY_LEVEL_3_ECO_ID);
        else if (batteryLvl > 100/6*2) batteryLevel.setBitmap(alreadyOrange ? BITMAP_BATTERY_LEVEL_2_POWER_ID    : BITMAP_BATTERY_LEVEL_2_ECO_ID);
        else if (batteryLvl > 100/6*1) batteryLevel.setBitmap(alreadyOrange ? BITMAP_BATTERY_LEVEL_1_POWER_ID    : BITMAP_BATTERY_LEVEL_1_ECO_ID);
        else                           batteryLevel.setBitmap(BITMAP_BATTERY_LEVEL_LOW_ID);
    }
    if (oldBitmapID != batteryLevel.getBitmapId())
    {
        batteryLevel.invalidate();
        oldBitmapID = batteryLevel.getBitmapId();
    }
}


void DashboardMediumView::updateTripInfo()
{
    uint16_t newBPM; float newDistance;
    switch (presenter->getSelectedButton())
    {
    case bpm:
        newBPM = presenter->getBPM();
        if (oldBPM != newBPM) { Unicode::snprintf(bpmValueBuffer, BPMVALUE_SIZE, "%d", newBPM); bpmValue.invalidate(); oldBPM = newBPM; }
        break;
    case time:
        newDistance = presenter->getDistance();
        if ((int)(newDistance * 10) != oldDistance)
        {
            Unicode::snprintfFloat(timeTextBuffer1, TIMETEXTBUFFER1_SIZE, "%.1f", newDistance);
            timeTextBuffer2[0] = 0; // Clear the second wildcard if not used
            timeText.invalidate();
            oldDistance = (int)(newDistance * 10);
        }
        break;
    case distance:
        newDistance = presenter->getDistance();
        if ((int)(newDistance*10) != oldDistance)
        {
            Unicode::snprintfFloat(distanceValueBuffer, DISTANCEVALUE_SIZE, "%.1f", newDistance);
            distanceValue.invalidate(); oldDistance = (int)(newDistance*10);
        }
        break;
    default: break;
    }
}

void DashboardMediumView::updateTheme()
{
    if (presenter->getLight())
    {
        gauge.setColor(Constants::LIGHT_BG_COLOR);
        backgroundColor.setColor(Constants::LIGHT_BG_COLOR);
        leftLines.setSVG(SVG_BACKGROUND_HEXAGON_LIGHT_ID);
        rightLines.setSVG(SVG_BACKGROUND_HEXAGON_LIGHT_ID);
        rightLines.setScaleX(-1); rightLines.setImagePosition(540, 0);
        languageSelector.setColor(Constants::LIGHT_BG_COLOR);
        bpmButton.setBitmaps(BITMAP_ICON_BPM_NORMAL_LIGHT_ID, BITMAP_ICON_BPM_NORMAL_LIGHT_ID, BITMAP_ICON_BPM_ACTIVE_LIGHT_ID, BITMAP_ICON_BPM_ACTIVE_LIGHT_ID);
        distanceButton.setBitmaps(BITMAP_ICON_DISTANCE_NORMAL_LIGHT_ID, BITMAP_ICON_DISTANCE_NORMAL_LIGHT_ID, BITMAP_ICON_DISTANCE_ACTIVE_LIGHT_ID, BITMAP_ICON_DISTANCE_ACTIVE_LIGHT_ID);
        bpmValue.setColor(Constants::LIGHT_MAIN_TEXTCOLOR); bpmSuffix.setColor(Constants::LIGHT_MAIN_TEXTCOLOR);
        timeText.setColor(Constants::LIGHT_MAIN_TEXTCOLOR); distanceValue.setColor(Constants::LIGHT_MAIN_TEXTCOLOR);
        distanceSuffix.setColor(Constants::LIGHT_MAIN_TEXTCOLOR);
        weatherButton.setBitmaps(BITMAP_BUTTON_NORMAL_LIGHT_ID, BITMAP_BUTTON_PRESSED_LIGHT_ID, BITMAP_ICON_WEATHER_LIGHT_ID, BITMAP_ICON_WEATHER_LIGHT_ID);
        batteryButton.setBitmaps(BITMAP_BUTTON_NORMAL_LIGHT_ID, BITMAP_BUTTON_PRESSED_LIGHT_ID, BITMAP_BATTERY_BACKGROUND_ID, BITMAP_BATTERY_BACKGROUND_ID);
        batteryButton.setIconXY(29, 48);
        modeButton.setBitmaps(BITMAP_BUTTON_SMALL_LEFT_NORMAL_LIGHT_ID, BITMAP_BUTTON_SMALL_LEFT_PRESSED_LIGHT_ID, BITMAP_ICON_MODE_LIGHT_ID, BITMAP_ICON_MODE_LIGHT_ID);
        modeButton.setIconXY(98, 20);
        sizeButton.setBitmaps(BITMAP_BUTTON_SMALL_RIGHT_NORMAL_LIGHT_ID, BITMAP_BUTTON_SMALL_RIGHT_PRESSED_LIGHT_ID, BITMAP_ICON_FORMAT_LIGHT_ID, BITMAP_ICON_FORMAT_LIGHT_ID);
        sizeButton.setIconXY(28, 20);
        lockButton.setBitmaps(BITMAP_BUTTON_NORMAL_LIGHT_ID, BITMAP_BUTTON_PRESSED_LIGHT_ID, BITMAP_ICON_LOCK_LIGHT_ID, BITMAP_ICON_LOCK_LIGHT_ID);
        languageButton.setBitmaps(BITMAP_BUTTON_NORMAL_LIGHT_ID, BITMAP_BUTTON_PRESSED_LIGHT_ID, BITMAP_ICON_LANGUAGE_LIGHT_ID, BITMAP_ICON_LANGUAGE_LIGHT_ID);
    }
    else
    {
        gauge.setColor(Constants::DARK_BG_COLOR);
        backgroundColor.setColor(Constants::DARK_BG_COLOR);
        leftLines.setSVG(SVG_BACKGROUND_HEXAGON_ID);
        rightLines.setSVG(SVG_BACKGROUND_HEXAGON_ID);
        rightLines.setScaleX(-1); rightLines.setImagePosition(540, 0);
        languageSelector.setColor(Constants::DARK_BG_COLOR);
        bpmButton.setBitmaps(BITMAP_ICON_BPM_NORMAL_ID, BITMAP_ICON_BPM_NORMAL_ID, BITMAP_ICON_BPM_ACTIVE_ID, BITMAP_ICON_BPM_ACTIVE_ID);
        distanceButton.setBitmaps(BITMAP_ICON_DISTANCE_NORMAL_ID, BITMAP_ICON_DISTANCE_NORMAL_ID, BITMAP_ICON_DISTANCE_ACTIVE_ID, BITMAP_ICON_DISTANCE_ACTIVE_ID);
        bpmValue.setColor(Constants::DARK_MAIN_TEXTCOLOR); bpmSuffix.setColor(Constants::DARK_MAIN_TEXTCOLOR);
        timeText.setColor(Constants::DARK_MAIN_TEXTCOLOR); distanceValue.setColor(Constants::DARK_MAIN_TEXTCOLOR);
        distanceSuffix.setColor(Constants::DARK_MAIN_TEXTCOLOR);
        weatherButton.setBitmaps(BITMAP_BUTTON_NORMAL_ID, BITMAP_BUTTON_PRESSED_ID, BITMAP_ICON_WEATHER_ID, BITMAP_ICON_WEATHER_ID);
        batteryButton.setBitmaps(BITMAP_BUTTON_NORMAL_ID, BITMAP_BUTTON_PRESSED_ID, BITMAP_BATTERY_BACKGROUND_ID, BITMAP_BATTERY_BACKGROUND_ID);
        batteryButton.setIconXY(29, 48);
        modeButton.setBitmaps(BITMAP_BUTTON_SMALL_LEFT_NORMAL_ID, BITMAP_BUTTON_SMALL_LEFT_PRESSED_ID, BITMAP_ICON_MODE_ID, BITMAP_ICON_MODE_ID);
        modeButton.setIconXY(98, 20);
        sizeButton.setBitmaps(BITMAP_BUTTON_SMALL_RIGHT_NORMAL_ID, BITMAP_BUTTON_SMALL_RIGHT_PRESSED_ID, BITMAP_ICON_FORMAT_ID, BITMAP_ICON_FORMAT_ID);
        sizeButton.setIconXY(28, 20);
        lockButton.setBitmaps(BITMAP_BUTTON_NORMAL_ID, BITMAP_BUTTON_PRESSED_ID, BITMAP_ICON_LOCK_ID, BITMAP_ICON_LOCK_ID);
        languageButton.setBitmaps(BITMAP_BUTTON_NORMAL_ID, BITMAP_BUTTON_PRESSED_ID, BITMAP_ICON_LANGUAGE_ID, BITMAP_ICON_LANGUAGE_ID);
    }
    presenter->getPower() > 22 ? setOrangeBattery() : setGreenBattery();
    updateBatteryLevel();
    invalidate();
}
