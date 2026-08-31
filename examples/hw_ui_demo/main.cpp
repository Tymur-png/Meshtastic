#include <Arduino.h>
#include "ui/ui.h"

void setup() {
  Serial.begin(115200);
  delay(50);
  ui_init();
}

void loop() {
  ui_loop();
  delay(20);
}
