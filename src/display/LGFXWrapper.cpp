#include "LGFXWrapper.h"
#include "variant.h"

using namespace lgfx;

void LGFXWrapper::init()
{
  // Configure Parallel8 bus
  auto bus = new bus::Bus_Parallel8();
  bus->bus_width = 8;
  bus->pin_wr = TFT_WR;
  bus->pin_rs = TFT_DC;
  bus->pin_d0 = TFT_D0;
  bus->pin_d1 = TFT_D1;
  bus->pin_d2 = TFT_D2;
  bus->pin_d3 = TFT_D3;
  bus->pin_d4 = TFT_D4;
  bus->pin_d5 = TFT_D5;
  bus->pin_d6 = TFT_D6;
  bus->pin_d7 = TFT_D7;
  bus->freq_write = 20000000;
  bus->init();

  auto panel = new panel::Panel_ST7789();
  panel->bus = bus;
  panel->panel_width  = TFT_WIDTH;
  panel->panel_height = TFT_HEIGHT;
  panel->offset_x = 0;
  panel->offset_y = 0;
  panel->init();
  setPanel(panel);

  // init panel
  lgfx::LGFX_Device::init();
}
