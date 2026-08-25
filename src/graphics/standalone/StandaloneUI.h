#pragma once

#include "configuration.h"

#ifdef STANDALONE_UI

#include "Observer.h"
#include "RussianKeyboard.h"
#include "input/InputBroker.h"
#include "mesh/generated/meshtastic/mesh.pb.h"
#include <OLEDDisplay.h>
#include <OLEDDisplayUi.h>
#include <string>
#include <vector>

namespace graphics
{

/**
 * Autonomous (phone-free) Meshtastic UI, Russian language.
 *
 * Screens (driven by 6 buttons: UP/DOWN/LEFT/RIGHT/OK/CHAT):
 *   BOOT -> MAIN_MENU -> CHAT / CONTACTS / NETWORK / SETTINGS / ABOUT
 * Overlays: NEW_MESSAGE popup, LOW_BATTERY popup, CHARGING screen.
 * The CHAT button toggles between the chat view and the Russian keyboard.
 *
 * Radio, mesh routing, encryption and message storage stay in the stock
 * Meshtastic core; this class only handles the screen and the controls.
 */
class StandaloneUI
{
  public:
    enum class Screen : uint8_t {
        BOOT,
        MAIN_MENU,
        CHAT,
        CONTACTS,
        CONTACT_DETAIL,
        NETWORK,
        REPEATER,
        SETTINGS,
        SETTING_NAME, // edit device name with the Russian keyboard
        SETTING_SOUND,
        SETTING_BRIGHTNESS,
        SETTING_REGION,
        ABOUT,
    };

    StandaloneUI();

    // Static frame callback installed into the Screen's frame list
    static void drawFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);

    // Input from Screen::handleInputEvent; returns true when consumed
    bool handleInput(const InputEvent *event);

    // Periodic housekeeping (battery warnings, charging detection, popup timeout)
    void tick();

    // Frame invalidation request after external events (new message, status change)
    void requestRedraw() { needsRedraw = true; }
    bool consumeRedrawRequest()
    {
        bool r = needsRedraw;
        needsRedraw = false;
        return r;
    }

    bool isKeyboardVisible() const { return keyboardVisible; }

  private:
    // ---- state ----
    Screen currentScreen = Screen::BOOT;
    Screen returnScreen = Screen::MAIN_MENU; // where SETTING_* screens go back to
    uint32_t bootStartMs = 0;
    bool keyboardVisible = false; // CHAT button toggles chat <-> keyboard
    bool needsRedraw = true;

    // menu navigation
    int16_t menuIndex = 0;
    int16_t settingsIndex = 0;
    int16_t contactIndex = 0;
    int16_t chatScroll = 0; // lines scrolled up from the newest message

    // popup state
    bool newMsgPopup = false;
    uint32_t newMsgPopupMs = 0;
    std::string newMsgSender;
    std::string newMsgText;
    bool lowBattWarned = false;
    bool lowBattPopup = false;
    uint32_t lowBattPopupMs = 0;
    bool wasCharging = false;

    // brightness editing
    uint8_t editBrightness = 0;

    // region picker
    int16_t regionIndex = 0;

    RussianKeyboard keyboard;
    bool editingDeviceName = false;

    CallbackObserver<StandaloneUI, const meshtastic_MeshPacket *> textMessageObserver =
        CallbackObserver<StandaloneUI, const meshtastic_MeshPacket *>(this, &StandaloneUI::onTextMessage);

    // ---- input routing ----
    void onMenuInput(input_broker_event e);
    void onChatInput(input_broker_event e);
    void onContactsInput(input_broker_event e);
    void onNetworkInput(input_broker_event e);
    void onSettingsInput(input_broker_event e);
    void onPopupInput(input_broker_event e);
    void goBack();

    // ---- drawing ----
    void draw(OLEDDisplay *display);
    void drawBoot(OLEDDisplay *display);
    void drawMainMenu(OLEDDisplay *display);
    void drawChat(OLEDDisplay *display);
    void drawContacts(OLEDDisplay *display);
    void drawContactDetail(OLEDDisplay *display);
    void drawNetwork(OLEDDisplay *display);
    void drawRepeater(OLEDDisplay *display);
    void drawSettings(OLEDDisplay *display);
    void drawSettingName(OLEDDisplay *display);
    void drawSettingSound(OLEDDisplay *display);
    void drawSettingBrightness(OLEDDisplay *display);
    void drawSettingRegion(OLEDDisplay *display);
    void drawAbout(OLEDDisplay *display);
    void drawChargingOverlay(OLEDDisplay *display);
    void drawNewMsgPopup(OLEDDisplay *display);
    void drawLowBattPopup(OLEDDisplay *display);
    void drawStatusBar(OLEDDisplay *display);
    void drawFrameBox(OLEDDisplay *display, const char *title);

    // ---- mesh helpers ----
    void sendChatMessage(const std::string &text);
    int onTextMessage(const meshtastic_MeshPacket *mp);
    std::string nodeName(uint32_t nodeNum) const;
    int countRouters() const;
    void playUiBeep();

    static const char *const MAIN_MENU_ITEMS[];
    static const uint8_t MAIN_MENU_COUNT = 5;
    static const char *const SETTINGS_ITEMS[];
    static const uint8_t SETTINGS_COUNT = 9;
};

extern StandaloneUI *standaloneUI;

} // namespace graphics

#endif // STANDALONE_UI
