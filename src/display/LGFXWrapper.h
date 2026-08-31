#pragma once
#include <LovyanGFX.hpp>

class LGFXWrapper : public lgfx::LGFX_Device {
public:
  void init();
  void fillScreen(uint16_t color) { _panel->fillScreen(color); }
  void setCursor(int x,int y) { _panel->setCursor(x,y); }
  void setTextSize(int s) { _panel->setTextSize(s); }
  void print(const char* t) { _panel->print(t); }
  void println(const char* t) { _panel->println(t); }
  void setTextColor(uint16_t c) { _panel->setTextColor(c); }
};

