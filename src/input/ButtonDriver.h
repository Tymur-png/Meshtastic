#pragma once
#include <Arduino.h>

enum ButtonEvent {
  BUTTON_EVENT_NONE = 0,
  BUTTON_EVENT_OK_PRESS,
  BUTTON_EVENT_CHAT_PRESS,
  BUTTON_EVENT_UP_PRESS,
  BUTTON_EVENT_DOWN_PRESS,
  BUTTON_EVENT_LEFT_PRESS,
  BUTTON_EVENT_RIGHT_PRESS,
  BUTTON_EVENT_OK_LONG,
  BUTTON_EVENT_CHAT_LONG,
};

class ButtonDriver {
public:
  void init();
  ButtonEvent poll();
};
