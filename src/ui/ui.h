#pragma once

#include <stdint.h>

enum UIState {
  UI_STATE_BOOT,
  UI_STATE_MAIN_MENU,
  UI_STATE_CHAT_VIEW,
  UI_STATE_CONTACTS,
  UI_STATE_NETWORK,
  UI_STATE_SETTINGS,
};

void ui_init();
void ui_loop();
void ui_set_state(UIState s);
