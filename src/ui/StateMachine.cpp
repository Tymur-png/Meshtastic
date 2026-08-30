#include "ui.h"
#include "../display/LGFXWrapper.h"
#include "../input/ButtonDriver.h"
#include <Arduino.h>

static UIState current = UI_STATE_BOOT;
static LGFXWrapper display;
static ButtonDriver buttons;

void draw_boot()
{
  display.fillScreen(TFT_BLACK);
  display.setCursor(10, 10);
  display.setTextSize(2);
  display.print("MESHTASTIC");
  display.setCursor(10, 50);
  display.print("Загрузка...");
}

void draw_main_menu()
{
  display.fillScreen(TFT_BLACK);
  display.setCursor(10, 10);
  display.setTextSize(2);
  display.print("MESHTASTIC");
  display.setCursor(10, 40);
  display.setTextSize(1);
  display.print("> \xF0\x9F\x92\xAC Сообщения\n  \xF0\x9F\x91\xA5 Контакты\n  \xF0\x9F\x93\xA1 Сеть\n  \xE2\x9A\x99 Настройки");
}

void ui_init()
{
  display.init();
  buttons.init();
  draw_boot();
  delay(800);
  ui_set_state(UI_STATE_MAIN_MENU);
}

void ui_set_state(UIState s)
{
  current = s;
  switch (s) {
    case UI_STATE_MAIN_MENU: draw_main_menu(); break;
    case UI_STATE_CHAT_VIEW:
      display.fillScreen(TFT_BLACK);
      display.setCursor(10, 10);
      display.print("Чат: Друг");
      break;
    default:
      draw_main_menu();
  }
}

void ui_loop()
{
  ButtonEvent ev = buttons.poll();
  if (ev == BUTTON_EVENT_OK_PRESS) {
    if (current == UI_STATE_MAIN_MENU) ui_set_state(UI_STATE_CHAT_VIEW);
    else ui_set_state(UI_STATE_MAIN_MENU);
  }
}
