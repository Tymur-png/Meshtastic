#include "StandaloneUI.h"

#ifdef STANDALONE_UI

#include "MessageStore.h"
#include "PowerStatus.h"
#include "buzz/buzz.h"
#include "gps/RTC.h"
#include "graphics/Backlight.h"
#include "graphics/Screen.h"
#include "graphics/ScreenFonts.h"
#include "main.h"
#include "mesh/Channels.h"
#include "mesh/MeshService.h"
#include "mesh/NodeDB.h"
#include "mesh/Router.h"
#include "mesh/generated/meshtastic/config.pb.h"
#include "mesh/generated/meshtastic/device_ui.pb.h"
#include "modules/TextMessageModule.h"

namespace graphics
{

StandaloneUI *standaloneUI = nullptr;

// Main menu: Сообщения / Контакты / Сеть / Настройки / О устройстве
const char *const StandaloneUI::MAIN_MENU_ITEMS[] = {"Сообщения", "Контакты", "Сеть", "Настройки", "О устройстве"};

// Settings: matches the standalone firmware spec
const char *const StandaloneUI::SETTINGS_ITEMS[] = {"Имя устройства", "Канал",        "Регион",   "Мощность", "Звук",
                                                    "Язык",           "Яркость экрана", "Автоблокировка", "Информация"};

// Regions offered by the built-in picker (EU_868 default for Poland/EU)
static const struct {
    const char *label;
    meshtastic_Config_LoRaConfig_RegionCode code;
} REGION_OPTIONS[] = {
    {"EU_868", meshtastic_Config_LoRaConfig_RegionCode_EU_868},
    {"EU_433", meshtastic_Config_LoRaConfig_RegionCode_EU_433},
    {"RU", meshtastic_Config_LoRaConfig_RegionCode_RU},
    {"US", meshtastic_Config_LoRaConfig_RegionCode_US},
};
static const uint8_t REGION_OPTIONS_COUNT = sizeof(REGION_OPTIONS) / sizeof(REGION_OPTIONS[0]);

StandaloneUI::StandaloneUI()
{
    bootStartMs = millis();
    keyboard.setCallback([this](const std::string &text) {
        if (editingDeviceName) {
            // Save the new device name into the local user record
            if (!text.empty()) {
                strncpy(owner.long_name, text.c_str(), sizeof(owner.long_name) - 1);
                owner.long_name[sizeof(owner.long_name) - 1] = '\0';
                nodeDB->saveToDisk();
                service->refreshLocalMeshNode();
            }
            editingDeviceName = false;
            keyboardVisible = false;
            currentScreen = Screen::SETTINGS;
        } else {
            if (!text.empty())
                sendChatMessage(text);
            keyboardVisible = false;
        }
        keyboard.reset();
        requestRedraw();
    });
    if (textMessageModule)
        textMessageObserver.observe(textMessageModule);
}

// ------------------------------------------------------------------
// helpers
// ------------------------------------------------------------------

std::string StandaloneUI::nodeName(uint32_t nodeNum) const
{
    if (nodeNum == nodeDB->getNodeNum())
        return "Я";
    meshtastic_NodeInfoLite *node = nodeDB->getMeshNode(nodeNum);
    if (node && node->long_name[0])
        return std::string(node->long_name);
    if (node && node->short_name[0])
        return std::string(node->short_name);
    char id[12];
    snprintf(id, sizeof(id), "!%08x", nodeNum);
    return std::string(id);
}

size_t StandaloneUI::nodeCount() const
{
    return nodeDB ? nodeDB->getNumMeshNodes() : 0;
}

meshtastic_NodeInfoLite *StandaloneUI::nodeByIndex(size_t index) const
{
    if (!nodeDB)
        return nullptr;
    // The radio thread can evict nodes concurrently, shrinking numMeshNodes.
    // Snapshot the live count and re-check just before dereferencing so a
    // stale loop index cannot trip the assert inside getMeshNodeByIndex().
    if (index >= nodeDB->getNumMeshNodes())
        return nullptr;
    return nodeDB->getMeshNodeByIndexSafe(index);
}

bool StandaloneUI::isRelayRole(meshtastic_Config_DeviceConfig_Role role)
{
    return role == meshtastic_Config_DeviceConfig_Role_ROUTER || role == meshtastic_Config_DeviceConfig_Role_REPEATER ||
           role == meshtastic_Config_DeviceConfig_Role_ROUTER_CLIENT ||
           role == meshtastic_Config_DeviceConfig_Role_ROUTER_LATE ||
           role == meshtastic_Config_DeviceConfig_Role_CLIENT_BASE;
}

const char *StandaloneUI::roleLabel(meshtastic_Config_DeviceConfig_Role role)
{
    switch (role) {
    case meshtastic_Config_DeviceConfig_Role_ROUTER:
        return "ROUTER";
    case meshtastic_Config_DeviceConfig_Role_REPEATER:
        return "REPEATER";
    case meshtastic_Config_DeviceConfig_Role_ROUTER_LATE:
        return "RTR LATE";
    case meshtastic_Config_DeviceConfig_Role_ROUTER_CLIENT:
        return "RTR CLT";
    case meshtastic_Config_DeviceConfig_Role_CLIENT_BASE:
        return "CLT BASE";
    case meshtastic_Config_DeviceConfig_Role_CLIENT_MUTE:
        return "MUTE";
    default:
        return "CLIENT";
    }
}

void StandaloneUI::collectRemoteNodes(std::vector<size_t> &out, bool relaysFirst) const
{
    out.clear();
    if (!nodeDB)
        return;
    const uint32_t self = nodeDB->getNodeNum();
    const size_t n = nodeCount();
    if (relaysFirst) {
        for (size_t i = 0; i < n; i++) {
            meshtastic_NodeInfoLite *node = nodeByIndex(i);
            if (node && node->num != self && isRelayRole(node->role))
                out.push_back(i);
        }
    }
    for (size_t i = 0; i < n; i++) {
        meshtastic_NodeInfoLite *node = nodeByIndex(i);
        if (!node || node->num == self)
            continue;
        if (relaysFirst && isRelayRole(node->role))
            continue;
        out.push_back(i);
    }
}

int StandaloneUI::countRelays() const
{
    int count = 0;
    if (!nodeDB)
        return 0;
    const uint32_t self = nodeDB->getNodeNum();
    const size_t n = nodeCount();
    for (size_t i = 0; i < n; i++) {
        meshtastic_NodeInfoLite *node = nodeByIndex(i);
        if (node && node->num != self && isRelayRole(node->role))
            count++;
    }
    return count;
}

int StandaloneUI::countRemoteNodes() const
{
    int count = 0;
    if (!nodeDB)
        return 0;
    const uint32_t self = nodeDB->getNodeNum();
    const size_t n = nodeCount();
    for (size_t i = 0; i < n; i++) {
        meshtastic_NodeInfoLite *node = nodeByIndex(i);
        if (node && node->num != self)
            count++;
    }
    return count;
}

void StandaloneUI::playUiBeep()
{
#if HAS_SCREEN
    if (config.device.buzzer_mode != meshtastic_Config_DeviceConfig_BuzzerMode_DISABLED)
        playBeep();
#endif
}

void StandaloneUI::sendChatMessage(const std::string &text)
{
    // decoded.payload.bytes is a fixed 233-byte array (meshtastic_Data_payload_t);
    // Cyrillic is 2 bytes/char in UTF-8. Copying past it clobbers pool-allocated
    // mesh packets and panics the device, so clamp to the buffer size first.
    const size_t maxLen = sizeof(((meshtastic_MeshPacket *)nullptr)->decoded.payload.bytes);
    const size_t len = text.size() < maxLen ? text.size() : maxLen;

    meshtastic_MeshPacket *p = router->allocForSending();
    if (!p) {
        playLongBeep(); // error signal
        return;
    }
    p->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
    p->to = NODENUM_BROADCAST;
    p->channel = 0;
    p->want_ack = true;
    p->decoded.payload.size = len;
    memcpy(p->decoded.payload.bytes, text.c_str(), len);

    // Keep our own message in the local history so it survives a reboot
    messageStore.tryAddFromPacket(*p);

    service->sendToMesh(p, RX_SRC_LOCAL, true);

    // two short beeps = message sent
    if (config.device.buzzer_mode != meshtastic_Config_DeviceConfig_BuzzerMode_DISABLED) {
        playBeep();
        playBeep();
    }
    chatScroll = 0;
}

int StandaloneUI::onTextMessage(const meshtastic_MeshPacket *mp)
{
    if (!mp || mp->from == nodeDB->getNodeNum())
        return 0;

    // Popup with sender + text, short buzzer signal
    newMsgSender = nodeName(mp->from);
    const size_t len = mp->decoded.payload.size;
    newMsgText.assign((const char *)mp->decoded.payload.bytes, len);
    newMsgPopup = true;
    newMsgPopupMs = millis();

    if (config.device.buzzer_mode != meshtastic_Config_DeviceConfig_BuzzerMode_DISABLED)
        playBeep();

    requestRedraw();
    return 0;
}

// ------------------------------------------------------------------
// input
// ------------------------------------------------------------------

bool StandaloneUI::handleInput(const InputEvent *event)
{
    if (!event)
        return false;
    const input_broker_event e = event->inputEvent;

    // The CHAT button toggles chat <-> keyboard wherever it makes sense
    if (e == INPUT_BROKER_CHAT_TOGGLE) {
        if (currentScreen == Screen::CHAT || currentScreen == Screen::SETTING_NAME) {
            keyboardVisible = !keyboardVisible;
            playUiBeep();
            requestRedraw();
        } else {
            // Jump straight to the chat from any other screen
            currentScreen = Screen::CHAT;
            keyboardVisible = false;
            chatScroll = 0;
            requestRedraw();
        }
        return true;
    }

    // Popups swallow input first
    if (newMsgPopup || lowBattPopup) {
        onPopupInput(e);
        return true;
    }

    // Keyboard owns all input while visible; long-press LEFT (BACK) exits it
    if (keyboardVisible) {
        if (e == INPUT_BROKER_BACK) {
            keyboardVisible = false;
            editingDeviceName = false;
            goBack();
            return true;
        }
        if (keyboard.handleInput(e)) {
            playUiBeep();
            requestRedraw();
        }
        return true;
    }

    switch (currentScreen) {
    case Screen::BOOT:
        return true; // ignore input while booting
    case Screen::MAIN_MENU:
        onMenuInput(e);
        return true;
    case Screen::CHAT:
        onChatInput(e);
        return true;
    case Screen::CONTACTS:
    case Screen::CONTACT_DETAIL:
        onContactsInput(e);
        return true;
    case Screen::NETWORK:
    case Screen::REPEATER:
        onNetworkInput(e);
        return true;
    case Screen::SETTINGS:
    case Screen::SETTING_NAME:
    case Screen::SETTING_SOUND:
    case Screen::SETTING_BRIGHTNESS:
    case Screen::SETTING_REGION:
        onSettingsInput(e);
        return true;
    case Screen::ABOUT:
        if (e == INPUT_BROKER_SELECT || e == INPUT_BROKER_LEFT || e == INPUT_BROKER_BACK)
            goBack();
        return true;
    }
    return false;
}

void StandaloneUI::goBack()
{
    keyboardVisible = false;
    editingDeviceName = false;
    switch (currentScreen) {
    case Screen::CONTACT_DETAIL:
        currentScreen = Screen::CONTACTS;
        break;
    case Screen::REPEATER:
        currentScreen = Screen::NETWORK;
        break;
    case Screen::SETTING_NAME:
    case Screen::SETTING_SOUND:
    case Screen::SETTING_BRIGHTNESS:
    case Screen::SETTING_REGION:
        currentScreen = Screen::SETTINGS;
        break;
    default:
        currentScreen = Screen::MAIN_MENU;
        break;
    }
    requestRedraw();
}

void StandaloneUI::onPopupInput(input_broker_event e)
{
    if (e == INPUT_BROKER_SELECT) {
        if (newMsgPopup) {
            newMsgPopup = false;
            // OK opens the message
            currentScreen = Screen::CHAT;
            keyboardVisible = false;
            chatScroll = 0;
        }
        lowBattPopup = false;
    } else if (e == INPUT_BROKER_BACK || e == INPUT_BROKER_LEFT) {
        newMsgPopup = false;
        lowBattPopup = false;
    }
    requestRedraw();
}

void StandaloneUI::onMenuInput(input_broker_event e)
{
    switch (e) {
    case INPUT_BROKER_UP:
        menuIndex = (menuIndex + MAIN_MENU_COUNT - 1) % MAIN_MENU_COUNT;
        playUiBeep();
        break;
    case INPUT_BROKER_DOWN:
        menuIndex = (menuIndex + 1) % MAIN_MENU_COUNT;
        playUiBeep();
        break;
    case INPUT_BROKER_SELECT:
        playUiBeep();
        switch (menuIndex) {
        case 0:
            currentScreen = Screen::CHAT;
            chatScroll = 0;
            break;
        case 1:
            currentScreen = Screen::CONTACTS;
            contactIndex = 0;
            break;
        case 2:
            currentScreen = Screen::NETWORK;
            repeaterIndex = 0;
            break;
        case 3:
            currentScreen = Screen::SETTINGS;
            settingsIndex = 0;
            break;
        case 4:
            currentScreen = Screen::ABOUT;
            break;
        }
        break;
    default:
        break;
    }
    requestRedraw();
}

void StandaloneUI::onChatInput(input_broker_event e)
{
    switch (e) {
    case INPUT_BROKER_UP:
        chatScroll++;
        break;
    case INPUT_BROKER_DOWN:
        if (chatScroll > 0)
            chatScroll--;
        break;
    case INPUT_BROKER_SELECT:
        // OK opens the keyboard to type a reply
        keyboard.reset();
        keyboardVisible = true;
        break;
    case INPUT_BROKER_LEFT:
    case INPUT_BROKER_BACK:
        goBack();
        return;
    default:
        break;
    }
    requestRedraw();
}

void StandaloneUI::onContactsInput(input_broker_event e)
{
    const int nodeTotal = (int)nodeCount();
    if (contactIndex >= nodeTotal)
        contactIndex = nodeTotal > 0 ? nodeTotal - 1 : 0;
    if (currentScreen == Screen::CONTACT_DETAIL) {
        if (e == INPUT_BROKER_SELECT || e == INPUT_BROKER_LEFT || e == INPUT_BROKER_BACK)
            goBack();
        requestRedraw();
        return;
    }
    switch (e) {
    case INPUT_BROKER_UP:
        if (contactIndex > 0)
            contactIndex--;
        break;
    case INPUT_BROKER_DOWN:
        if (nodeTotal > 0 && contactIndex < nodeTotal - 1)
            contactIndex++;
        break;
    case INPUT_BROKER_SELECT:
        if (nodeTotal > 0)
            currentScreen = Screen::CONTACT_DETAIL;
        break;
    case INPUT_BROKER_LEFT:
    case INPUT_BROKER_BACK:
        goBack();
        return;
    default:
        break;
    }
    requestRedraw();
}

void StandaloneUI::onNetworkInput(input_broker_event e)
{
    if (currentScreen == Screen::REPEATER) {
        std::vector<size_t> nodes;
        collectRemoteNodes(nodes, true);
        const int total = (int)nodes.size();
        if (repeaterIndex >= total)
            repeaterIndex = total > 0 ? total - 1 : 0;
        switch (e) {
        case INPUT_BROKER_UP:
            if (repeaterIndex > 0)
                repeaterIndex--;
            break;
        case INPUT_BROKER_DOWN:
            if (total > 0 && repeaterIndex < total - 1)
                repeaterIndex++;
            break;
        case INPUT_BROKER_SELECT:
        case INPUT_BROKER_LEFT:
        case INPUT_BROKER_BACK:
            goBack();
            return;
        default:
            break;
        }
        requestRedraw();
        return;
    }
    switch (e) {
    case INPUT_BROKER_SELECT:
        if (countRemoteNodes() > 0) {
            repeaterIndex = 0;
            currentScreen = Screen::REPEATER;
        }
        break;
    case INPUT_BROKER_LEFT:
    case INPUT_BROKER_BACK:
        goBack();
        return;
    default:
        break;
    }
    requestRedraw();
}

void StandaloneUI::onSettingsInput(input_broker_event e)
{
    switch (currentScreen) {
    case Screen::SETTINGS:
        switch (e) {
        case INPUT_BROKER_UP:
            settingsIndex = (settingsIndex + SETTINGS_COUNT - 1) % SETTINGS_COUNT;
            playUiBeep();
            break;
        case INPUT_BROKER_DOWN:
            settingsIndex = (settingsIndex + 1) % SETTINGS_COUNT;
            playUiBeep();
            break;
        case INPUT_BROKER_SELECT:
            switch (settingsIndex) {
            case 0: // Имя устройства -> Russian keyboard
                editingDeviceName = true;
                keyboard.reset();
                keyboard.setInputText(owner.long_name);
                keyboardVisible = true;
                currentScreen = Screen::SETTING_NAME;
                break;
            case 2: // Регион
                regionIndex = 0;
                for (uint8_t i = 0; i < REGION_OPTIONS_COUNT; i++)
                    if (REGION_OPTIONS[i].code == config.lora.region)
                        regionIndex = i;
                currentScreen = Screen::SETTING_REGION;
                break;
            case 4: // Звук
                currentScreen = Screen::SETTING_SOUND;
                break;
            case 6: // Яркость экрана
                editBrightness = uiconfig.screen_brightness;
                currentScreen = Screen::SETTING_BRIGHTNESS;
#if HAS_PWM_BACKLIGHT
                // An unset (0) stored brightness means "still default"; start the
                // preview from the real default so the bar isn't empty on first use.
                if (editBrightness == 0)
                    editBrightness = PWM_BACKLIGHT_DEFAULT;
                backlightSet(editBrightness); // live preview
#endif
                break;
            case 8: // Информация
                currentScreen = Screen::ABOUT;
                break;
            default: // Канал / Мощность / Язык / Автоблокировка: read-only placeholders for now
                break;
            }
            break;
        case INPUT_BROKER_LEFT:
        case INPUT_BROKER_BACK:
            goBack();
            return;
        default:
            break;
        }
        break;

    case Screen::SETTING_SOUND:
        if (e == INPUT_BROKER_UP || e == INPUT_BROKER_DOWN || e == INPUT_BROKER_SELECT) {
            bool disabled = config.device.buzzer_mode == meshtastic_Config_DeviceConfig_BuzzerMode_DISABLED;
            config.device.buzzer_mode =
                disabled ? meshtastic_Config_DeviceConfig_BuzzerMode_ALL_ENABLED
                         : meshtastic_Config_DeviceConfig_BuzzerMode_DISABLED;
            nodeDB->saveToDisk(SEGMENT_CONFIG);
            if (!disabled)
                playBeep();
        } else if (e == INPUT_BROKER_LEFT || e == INPUT_BROKER_BACK) {
            goBack();
            return;
        }
        break;

    case Screen::SETTING_BRIGHTNESS:
        if (e == INPUT_BROKER_UP || e == INPUT_BROKER_RIGHT) {
            editBrightness = (editBrightness >= 250) ? 250 : editBrightness + 26;
        } else if (e == INPUT_BROKER_DOWN || e == INPUT_BROKER_LEFT) {
            editBrightness = (editBrightness <= 26) ? 0 : editBrightness - 26;
        } else if (e == INPUT_BROKER_SELECT) {
            uiconfig.screen_brightness = editBrightness;
#if HAS_PWM_BACKLIGHT
            backlightSet(editBrightness);
#endif
            nodeDB->saveProto("/prefs/uiconfig.proto", meshtastic_DeviceUIConfig_size, &meshtastic_DeviceUIConfig_msg,
                              &uiconfig);
            goBack();
            return;
        } else if (e == INPUT_BROKER_BACK) {
            goBack();
            return;
        }
#if HAS_PWM_BACKLIGHT
        backlightSet(editBrightness); // live preview
#endif
        break;

    case Screen::SETTING_REGION:
        if (e == INPUT_BROKER_UP)
            regionIndex = (regionIndex + REGION_OPTIONS_COUNT - 1) % REGION_OPTIONS_COUNT;
        else if (e == INPUT_BROKER_DOWN)
            regionIndex = (regionIndex + 1) % REGION_OPTIONS_COUNT;
        else if (e == INPUT_BROKER_SELECT) {
            config.lora.region = REGION_OPTIONS[regionIndex].code;
            nodeDB->saveToDisk(SEGMENT_CONFIG);
            goBack();
            return;
        } else if (e == INPUT_BROKER_LEFT || e == INPUT_BROKER_BACK) {
            goBack();
            return;
        }
        break;

    default:
        break;
    }
    requestRedraw();
}

// ------------------------------------------------------------------
// periodic tick (called from the Screen thread)
// ------------------------------------------------------------------

void StandaloneUI::tick()
{
    // Boot screen auto-advances once early init is done
    if (currentScreen == Screen::BOOT && millis() - bootStartMs > 4000) {
        currentScreen = Screen::MAIN_MENU;
        requestRedraw();
    }

    // New-message popup auto-hides after 30 s
    if (newMsgPopup && millis() - newMsgPopupMs > 30000) {
        newMsgPopup = false;
        requestRedraw();
    }
    if (lowBattPopup && millis() - lowBattPopupMs > 8000) {
        lowBattPopup = false;
        requestRedraw();
    }

    if (powerStatus && powerStatus->getHasBattery()) {
        const uint8_t pct = powerStatus->getBatteryChargePercent();
        // Low battery warning (~10%)
        if (pct > 0 && pct <= 10 && !lowBattWarned) {
            lowBattWarned = true;
            lowBattPopup = true;
            lowBattPopupMs = millis();
            playLongBeep();
            requestRedraw();
        } else if (pct > 20) {
            lowBattWarned = false;
        }

        // Charging state change redraws the screen (charging overlay)
        const bool charging = powerStatus->getIsCharging();
        if (charging != wasCharging) {
            wasCharging = charging;
            requestRedraw();
        }
    }
}

// ------------------------------------------------------------------
// drawing
// ------------------------------------------------------------------

void StandaloneUI::drawFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    if (standaloneUI)
        standaloneUI->draw(display);
}

void StandaloneUI::drawFrameBox(OLEDDisplay *display, const char *title)
{
    display->setColor(WHITE);
    display->drawRect(0, 0, display->width(), display->height());
    display->drawLine(0, FONT_HEIGHT_SMALL + 4, display->width(), FONT_HEIGHT_SMALL + 4);
    display->setFont(FONT_SMALL);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(display->width() / 2, 2, title);
}

void StandaloneUI::drawStatusBar(OLEDDisplay *display)
{
    const int16_t y = display->height() - FONT_HEIGHT_SMALL - 2;
    display->drawLine(0, y - 2, display->width(), y - 2);
    display->setFont(FONT_SMALL);
    display->setTextAlignment(TEXT_ALIGN_LEFT);

    char batt[24];
    if (powerStatus && powerStatus->getHasBattery()) {
        if (powerStatus->getIsCharging())
            snprintf(batt, sizeof(batt), "Зарядка %u%%", powerStatus->getBatteryChargePercent());
        else
            snprintf(batt, sizeof(batt), "Бат %u%%", powerStatus->getBatteryChargePercent());
    } else {
        snprintf(batt, sizeof(batt), "Питание");
    }
    display->drawString(2, y, batt);

    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    char net[24];
    snprintf(net, sizeof(net), "Узлов: %u", (unsigned)nodeCount());
    display->drawString(display->width() - 2, y, net);
}

void StandaloneUI::draw(OLEDDisplay *display)
{
    display->setColor(WHITE);

    switch (currentScreen) {
    case Screen::BOOT:
        drawBoot(display);
        return; // no overlays during boot
    case Screen::MAIN_MENU:
        drawMainMenu(display);
        break;
    case Screen::CHAT:
        drawChat(display);
        break;
    case Screen::CONTACTS:
        drawContacts(display);
        break;
    case Screen::CONTACT_DETAIL:
        drawContactDetail(display);
        break;
    case Screen::NETWORK:
        drawNetwork(display);
        break;
    case Screen::REPEATER:
        drawRepeater(display);
        break;
    case Screen::SETTINGS:
        drawSettings(display);
        break;
    case Screen::SETTING_NAME:
        drawSettingName(display);
        break;
    case Screen::SETTING_SOUND:
        drawSettingSound(display);
        break;
    case Screen::SETTING_BRIGHTNESS:
        drawSettingBrightness(display);
        break;
    case Screen::SETTING_REGION:
        drawSettingRegion(display);
        break;
    case Screen::ABOUT:
        drawAbout(display);
        break;
    }

    // Overlays on top of any screen
    if (powerStatus && powerStatus->getIsCharging() && currentScreen != Screen::BOOT)
        drawChargingOverlay(display);
    if (lowBattPopup)
        drawLowBattPopup(display);
    if (newMsgPopup)
        drawNewMsgPopup(display);
}

void StandaloneUI::drawBoot(OLEDDisplay *display)
{
    const int16_t w = display->width();
    const int16_t h = display->height();
    display->setFont(FONT_MEDIUM);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(w / 2, h / 4, "MESHTASTIC");

    display->setFont(FONT_SMALL);
    display->drawString(w / 2, h / 2, "Загрузка...");

    // Progress bar driven by boot elapsed time (4 s total)
    const uint32_t elapsed = millis() - bootStartMs;
    const int16_t barW = w - 40;
    int16_t fill = (int16_t)((int32_t)barW * elapsed / 4000);
    if (fill > barW)
        fill = barW;
    display->drawRect(20, h * 3 / 4, barW, 10);
    display->fillRect(20, h * 3 / 4, fill, 10);
}

void StandaloneUI::drawMainMenu(OLEDDisplay *display)
{
    drawFrameBox(display, "MESHTASTIC");
    display->setFont(FONT_SMALL);
    const int16_t lineH = FONT_HEIGHT_SMALL + 6;
    int16_t y = FONT_HEIGHT_SMALL + 10;
    for (uint8_t i = 0; i < MAIN_MENU_COUNT; i++) {
        if (i == menuIndex) {
            display->fillRect(2, y - 1, display->width() - 4, lineH);
            display->setColor(BLACK);
            display->setTextAlignment(TEXT_ALIGN_LEFT);
            display->drawString(6, y + 2, MAIN_MENU_ITEMS[i]);
            display->setColor(WHITE);
        } else {
            display->setTextAlignment(TEXT_ALIGN_LEFT);
            display->drawString(6, y + 2, MAIN_MENU_ITEMS[i]);
        }
        y += lineH;
    }
    drawStatusBar(display);
}

void StandaloneUI::drawChat(OLEDDisplay *display)
{
    const int16_t w = display->width();
    const int16_t h = display->height();

    if (keyboardVisible) {
        // Keyboard view: input line on top, keyboard below
        drawFrameBox(display, "Новое сообщение");
        keyboard.drawInputLine(display, 4, FONT_HEIGHT_SMALL + 8, w - 8);
        keyboard.draw(display, FONT_HEIGHT_SMALL * 2 + 14);
        return;
    }

    drawFrameBox(display, "Чат");

    // Render stored messages, newest at the bottom
    const auto &messages = messageStore.getMessages();
    display->setFont(FONT_SMALL);
    const int16_t lineH = FONT_HEIGHT_SMALL + 3;
    const int16_t top = FONT_HEIGHT_SMALL + 8;
    const int16_t bottom = h - FONT_HEIGHT_SMALL - 8; // above status bar
    const int maxLines = (bottom - top) / lineH;

    // Build wrapped lines (sender header + text), then show the tail
    std::vector<std::string> lines;
    for (const auto &m : messages) {
        if (!messageStore.isMessageVisible(m))
            continue;
        lines.push_back(nodeName(m.sender) + ":");
        std::string text = MessageStore::getText(m);
        // Wrap by display width; guard against non-shrinking loops when a
        // single "word" is wider than the screen (getStringWidth never shrinks).
        std::string rest = text;
        while (!rest.empty()) {
            size_t fit = rest.size();
            while (fit > 1 && display->getStringWidth(rest.substr(0, fit).c_str()) > w - 12)
                fit--;
            if (fit == 0)
                fit = 1; // one glyph per line, always progress
            lines.push_back(rest.substr(0, fit));
            rest.erase(0, fit);
        }
    }

    int total = (int)lines.size();
    if (chatScroll > total - maxLines)
        chatScroll = total > maxLines ? total - maxLines : 0;
    if (chatScroll < 0)
        chatScroll = 0;
    const int first = total - maxLines - chatScroll > 0 ? total - maxLines - chatScroll : 0;

    display->setTextAlignment(TEXT_ALIGN_LEFT);
    int16_t y = top;
    for (int i = first; i < total && y + lineH <= bottom; i++, y += lineH)
        display->drawString(4, y, lines[i].c_str());

    if (total == 0) {
        display->setTextAlignment(TEXT_ALIGN_CENTER);
        display->drawString(w / 2, h / 2, "Нет сообщений");
    }

    // Footer hint
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(w / 2, h - FONT_HEIGHT_SMALL - 4, "[OK - написать]");
}

void StandaloneUI::drawContacts(OLEDDisplay *display)
{
    drawFrameBox(display, "КОНТАКТЫ");
    display->setFont(FONT_SMALL);
    const int16_t lineH = FONT_HEIGHT_SMALL + 4;
    const int16_t top = FONT_HEIGHT_SMALL + 8;
    const int maxLines = (display->height() - top - FONT_HEIGHT_SMALL - 6) / lineH;
    const int nodeTotal = (int)nodeCount();
    if (contactIndex >= nodeTotal)
        contactIndex = nodeTotal > 0 ? nodeTotal - 1 : 0;

    if (nodeTotal == 0) {
        display->setTextAlignment(TEXT_ALIGN_CENTER);
        display->drawString(display->width() / 2, display->height() / 2, "Сеть пуста");
        return;
    }

    int first = 0;
    if (contactIndex >= maxLines)
        first = contactIndex - maxLines + 1;

    int16_t y = top;
    for (int i = first; i < nodeTotal && y + lineH <= display->height() - FONT_HEIGHT_SMALL - 4; i++, y += lineH) {
        meshtastic_NodeInfoLite *node = nodeByIndex(i);
        if (!node)
            continue;
        std::string name = nodeName(node->num);
        if (i == contactIndex) {
            display->fillRect(2, y - 1, display->width() - 4, lineH);
            display->setColor(BLACK);
            display->setTextAlignment(TEXT_ALIGN_LEFT);
            display->drawString(6, y + 1, name.c_str());
            display->setColor(WHITE);
        } else {
            display->setTextAlignment(TEXT_ALIGN_LEFT);
            display->drawString(6, y + 1, name.c_str());
        }
    }
    drawStatusBar(display);
}

void StandaloneUI::drawContactDetail(OLEDDisplay *display)
{
    meshtastic_NodeInfoLite *node = nodeByIndex(contactIndex);
    drawFrameBox(display, "КОНТАКТ");
    if (!node)
        return;

    display->setFont(FONT_SMALL);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    int16_t y = FONT_HEIGHT_SMALL + 10;
    const int16_t lineH = FONT_HEIGHT_SMALL + 4;
    char buf[64];

    snprintf(buf, sizeof(buf), "Имя: %s", nodeName(node->num).c_str());
    display->drawString(6, y, buf);
    y += lineH;
    snprintf(buf, sizeof(buf), "ID: !%08x", node->num);
    display->drawString(6, y, buf);
    y += lineH;
    snprintf(buf, sizeof(buf), "SNR: %.1f dB", node->snr);
    display->drawString(6, y, buf);
    y += lineH;
    if (node->has_hops_away) {
        snprintf(buf, sizeof(buf), "Hops: %u", node->hops_away);
        display->drawString(6, y, buf);
        y += lineH;
    }
    const uint32_t ago = getValidTime(RTCQualityFromNet) > 0 && node->last_heard > 0
                             ? getValidTime(RTCQualityFromNet) - node->last_heard
                             : 0;
    if (ago > 0) {
        snprintf(buf, sizeof(buf), "Контакт: %u мин назад", ago / 60);
        display->drawString(6, y, buf);
    }
}

void StandaloneUI::drawNetwork(OLEDDisplay *display)
{
    drawFrameBox(display, "СЕТЬ");
    display->setFont(FONT_SMALL);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    int16_t y = FONT_HEIGHT_SMALL + 10;
    const int16_t lineH = FONT_HEIGHT_SMALL + 4;
    char buf[64];

    snprintf(buf, sizeof(buf), "Узлов: %u", (unsigned)nodeCount());
    display->drawString(6, y, buf);
    y += lineH;
    snprintf(buf, sizeof(buf), "Ретрансляторов: %d", countRelays());
    display->drawString(6, y, buf);
    y += lineH;
    snprintf(buf, sizeof(buf), "Чужих узлов: %d", countRemoteNodes());
    display->drawString(6, y, buf);
    y += lineH;

    const char *region = "EU_868";
    for (uint8_t i = 0; i < REGION_OPTIONS_COUNT; i++)
        if (REGION_OPTIONS[i].code == config.lora.region)
            region = REGION_OPTIONS[i].label;
    snprintf(buf, sizeof(buf), "Регион: %s", region);
    display->drawString(6, y, buf);
    y += lineH;

    const char *chName = channels.getName(channels.getPrimaryIndex());
    snprintf(buf, sizeof(buf), "Канал: %s", chName && chName[0] ? chName : "Primary");
    display->drawString(6, y, buf);
    y += lineH;

    if (countRemoteNodes() > 0) {
        display->setTextAlignment(TEXT_ALIGN_CENTER);
        display->drawString(display->width() / 2, display->height() - FONT_HEIGHT_SMALL * 2 - 6, "[OK - узлы сети]");
    }
    drawStatusBar(display);
}

void StandaloneUI::drawRepeater(OLEDDisplay *display)
{
    drawFrameBox(display, "УЗЛЫ СЕТИ");
    display->setFont(FONT_SMALL);

    std::vector<size_t> nodes;
    collectRemoteNodes(nodes, true);
    const int total = (int)nodes.size();
    if (repeaterIndex >= total)
        repeaterIndex = total > 0 ? total - 1 : 0;

    if (total == 0) {
        display->setTextAlignment(TEXT_ALIGN_CENTER);
        display->drawString(display->width() / 2, display->height() / 2, "Не найдены");
        return;
    }

    const int16_t lineH = FONT_HEIGHT_SMALL + 4;
    const int16_t top = FONT_HEIGHT_SMALL + 8;
    const int maxLines = (display->height() - top - FONT_HEIGHT_SMALL - 6) / lineH;
    int first = 0;
    if (maxLines > 0 && repeaterIndex >= maxLines)
        first = repeaterIndex - maxLines + 1;

    char buf[64];
    int16_t y = top;
    for (int i = first; i < total && y + lineH <= display->height() - FONT_HEIGHT_SMALL - 4; i++, y += lineH) {
        meshtastic_NodeInfoLite *node = nodeByIndex(nodes[i]);
        if (!node)
            continue;
        std::string name = nodeName(node->num);
        if (name.size() > 12)
            name = name.substr(0, 12);
        snprintf(buf, sizeof(buf), "%s %s", isRelayRole(node->role) ? "*" : " ", name.c_str());
        if (i == repeaterIndex) {
            display->fillRect(2, y - 1, display->width() - 4, lineH);
            display->setColor(BLACK);
            display->setTextAlignment(TEXT_ALIGN_LEFT);
            display->drawString(6, y + 1, buf);
            display->setTextAlignment(TEXT_ALIGN_RIGHT);
            display->drawString(display->width() - 4, y + 1, roleLabel(node->role));
            display->setColor(WHITE);
        } else {
            display->setTextAlignment(TEXT_ALIGN_LEFT);
            display->drawString(6, y + 1, buf);
            display->setTextAlignment(TEXT_ALIGN_RIGHT);
            display->drawString(display->width() - 4, y + 1, roleLabel(node->role));
        }
    }
    drawStatusBar(display);
}

void StandaloneUI::drawSettings(OLEDDisplay *display)
{
    drawFrameBox(display, "НАСТРОЙКИ");
    display->setFont(FONT_SMALL);
    const int16_t lineH = FONT_HEIGHT_SMALL + 4;
    const int16_t top = FONT_HEIGHT_SMALL + 8;
    const int maxLines = (display->height() - top - FONT_HEIGHT_SMALL - 6) / lineH;

    int first = 0;
    if (settingsIndex >= maxLines)
        first = settingsIndex - maxLines + 1;

    int16_t y = top;
    for (int i = first; i < SETTINGS_COUNT && y + lineH <= display->height() - FONT_HEIGHT_SMALL - 4; i++, y += lineH) {
        if (i == settingsIndex) {
            display->fillRect(2, y - 1, display->width() - 4, lineH);
            display->setColor(BLACK);
            display->setTextAlignment(TEXT_ALIGN_LEFT);
            display->drawString(6, y + 1, SETTINGS_ITEMS[i]);
            display->setColor(WHITE);
        } else {
            display->setTextAlignment(TEXT_ALIGN_LEFT);
            display->drawString(6, y + 1, SETTINGS_ITEMS[i]);
        }
    }
    drawStatusBar(display);
}

void StandaloneUI::drawSettingName(OLEDDisplay *display)
{
    drawFrameBox(display, "Имя устройства");
    keyboard.drawInputLine(display, 4, FONT_HEIGHT_SMALL + 8, display->width() - 8);
    keyboard.draw(display, FONT_HEIGHT_SMALL * 2 + 14);
}

void StandaloneUI::drawSettingSound(OLEDDisplay *display)
{
    drawFrameBox(display, "Звук");
    display->setFont(FONT_SMALL);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    const bool on = config.device.buzzer_mode != meshtastic_Config_DeviceConfig_BuzzerMode_DISABLED;
    display->drawString(display->width() / 2, display->height() / 2 - FONT_HEIGHT_SMALL, on ? "> Вкл" : "  Вкл");
    display->drawString(display->width() / 2, display->height() / 2 + 2, !on ? "> Выкл" : "  Выкл");
}

void StandaloneUI::drawSettingBrightness(OLEDDisplay *display)
{
    drawFrameBox(display, "Яркость экрана");
    const int16_t w = display->width();
    const int16_t barW = w - 40;
    const int16_t barY = display->height() / 2;
    display->drawRect(20, barY, barW, 12);
    display->fillRect(20, barY, (int16_t)((int32_t)barW * editBrightness / 255), 12);
    display->setFont(FONT_SMALL);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    char buf[16];
    snprintf(buf, sizeof(buf), "%u%%", editBrightness * 100 / 255);
    display->drawString(w / 2, barY + 18, buf);
}

void StandaloneUI::drawSettingRegion(OLEDDisplay *display)
{
    drawFrameBox(display, "Регион");
    display->setFont(FONT_SMALL);
    const int16_t lineH = FONT_HEIGHT_SMALL + 4;
    int16_t y = FONT_HEIGHT_SMALL + 10;
    for (uint8_t i = 0; i < REGION_OPTIONS_COUNT; i++, y += lineH) {
        if (i == regionIndex) {
            display->fillRect(2, y - 1, display->width() - 4, lineH);
            display->setColor(BLACK);
            display->setTextAlignment(TEXT_ALIGN_LEFT);
            display->drawString(6, y + 1, REGION_OPTIONS[i].label);
            display->setColor(WHITE);
        } else {
            display->setTextAlignment(TEXT_ALIGN_LEFT);
            display->drawString(6, y + 1, REGION_OPTIONS[i].label);
        }
    }
}

void StandaloneUI::drawAbout(OLEDDisplay *display)
{
    drawFrameBox(display, "О УСТРОЙСТВЕ");
    display->setFont(FONT_SMALL);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    int16_t y = FONT_HEIGHT_SMALL + 10;
    const int16_t lineH = FONT_HEIGHT_SMALL + 4;
    char buf[64];

    snprintf(buf, sizeof(buf), "Имя: %s", owner.long_name);
    display->drawString(6, y, buf);
    y += lineH;
    snprintf(buf, sizeof(buf), "ID: !%08x", nodeDB->getNodeNum());
    display->drawString(6, y, buf);
    y += lineH;
    snprintf(buf, sizeof(buf), "Прошивка: %s", xstr(APP_VERSION_SHORT));
    display->drawString(6, y, buf);
    y += lineH;
    if (powerStatus && powerStatus->getHasBattery()) {
        snprintf(buf, sizeof(buf), "Батарея: %u%% (%.2f V)", powerStatus->getBatteryChargePercent(),
                 powerStatus->getBatteryVoltageMv() / 1000.0f);
        display->drawString(6, y, buf);
    }
}

void StandaloneUI::drawChargingOverlay(OLEDDisplay *display)
{
    // Small charging indicator in the top-right corner rather than a full screen,
    // so the device stays usable on the charger
    const int16_t w = display->width();
    display->setColor(WHITE);
    display->fillRect(w - 52, 2, 50, FONT_HEIGHT_SMALL + 4);
    display->setColor(BLACK);
    display->setFont(FONT_SMALL);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(w - 27, 4, "ЗАРЯДКА");
    display->setColor(WHITE);
}

void StandaloneUI::drawNewMsgPopup(OLEDDisplay *display)
{
    const int16_t w = display->width();
    const int16_t h = display->height();
    const int16_t boxH = h / 2;
    const int16_t boxY = (h - boxH) / 2;

    display->setColor(WHITE);
    display->fillRect(4, boxY, w - 8, boxH);
    display->setColor(BLACK);
    display->drawRect(6, boxY + 2, w - 12, boxH - 4);

    display->setFont(FONT_SMALL);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(w / 2, boxY + 6, "НОВОЕ СООБЩЕНИЕ");
    display->drawString(w / 2, boxY + 6 + FONT_HEIGHT_SMALL + 4, newMsgSender.c_str());

    // Message text, trimmed to fit two lines
    std::string text = newMsgText;
    const int16_t maxW = w - 24;
    while (text.size() > 1 && display->getStringWidth(text.c_str()) > maxW)
        text.pop_back();
    display->drawString(w / 2, boxY + 6 + (FONT_HEIGHT_SMALL + 4) * 2, text.c_str());
    display->drawString(w / 2, boxY + boxH - FONT_HEIGHT_SMALL - 6, "[OK]");
    display->setColor(WHITE);
}

void StandaloneUI::drawLowBattPopup(OLEDDisplay *display)
{
    const int16_t w = display->width();
    const int16_t h = display->height();
    const int16_t boxH = FONT_HEIGHT_SMALL * 3 + 16;
    const int16_t boxY = (h - boxH) / 2;

    display->setColor(WHITE);
    display->fillRect(4, boxY, w - 8, boxH);
    display->setColor(BLACK);
    display->drawRect(6, boxY + 2, w - 12, boxH - 4);
    display->setFont(FONT_SMALL);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(w / 2, boxY + 6, "! НИЗКИЙ ЗАРЯД");
    display->drawString(w / 2, boxY + 6 + FONT_HEIGHT_SMALL + 4, "Осталось ~10%");
    display->setColor(WHITE);
}

} // namespace graphics

#endif // STANDALONE_UI
