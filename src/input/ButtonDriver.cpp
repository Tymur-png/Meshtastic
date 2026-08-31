#include "ButtonDriver.h"
#include "variant.h"

// Simple polling driver with debounce and long press
static const uint32_t DEBOUNCE_MS = 30;
static const uint32_t LONG_MS = 800;

struct BtnState { int pin; uint32_t lastChange; bool lastLevel; uint32_t downAt; };

static BtnState btns[] = {
  { BUTTON_OK, 0, true, 0 },
  { BUTTON_CHAT, 0, true, 0 },
  { BUTTON_UP, 0, true, 0 },
  { BUTTON_DOWN, 0, true, 0 },
  { BUTTON_LEFT, 0, true, 0 },
  { BUTTON_RIGHT, 0, true, 0 },
};

void ButtonDriver::init()
{
  for (auto &b: btns) pinMode(b.pin, INPUT_PULLUP);
}

ButtonEvent ButtonDriver::poll()
{
  uint32_t t = millis();
  for (int i=0;i<6;i++) {
    bool level = digitalRead(btns[i].pin);
    if (level != btns[i].lastLevel) {
      if (t - btns[i].lastChange > DEBOUNCE_MS) {
        btns[i].lastLevel = level;
        btns[i].lastChange = t;
        if (!level) {
          // pressed (active low)
          btns[i].downAt = t;
        } else {
          // released
          uint32_t held = t - btns[i].downAt;
          // map index -> event
          if (i==0) return (held>=LONG_MS)?BUTTON_EVENT_OK_LONG:BUTTON_EVENT_OK_PRESS;
          if (i==1) return (held>=LONG_MS)?BUTTON_EVENT_CHAT_LONG:BUTTON_EVENT_CHAT_PRESS;
          if (i==2) return BUTTON_EVENT_UP_PRESS;
          if (i==3) return BUTTON_EVENT_DOWN_PRESS;
          if (i==4) return BUTTON_EVENT_LEFT_PRESS;
          if (i==5) return BUTTON_EVENT_RIGHT_PRESS;
        }
      }
    }
  }
  return BUTTON_EVENT_NONE;
}
