#pragma once

#include "configuration.h"

#ifdef STANDALONE_UI

#include "input/InputBroker.h"
#include <OLEDDisplay.h>
#include <functional>
#include <string>

namespace graphics
{

/**
 * Russian on-screen keyboard for the standalone (phone-free) UI.
 *
 * Layout (33 Cyrillic letters + control row):
 *   А Б В Г Д Е Ж З И Й К
 *   Л М Н О П Р С Т У Ф Х
 *   Ц Ч Ш Щ Ъ Ы Ь Э Ю Я
 *   [ПРОБЕЛ]   [⌫]   [ОТПРАВИТЬ]
 *
 * Navigation: arrow keys move the cursor, OK (SELECT) types the key,
 * long OK submits the current text (same as choosing ОТПРАВИТЬ).
 * Text is kept as UTF-8; the OLED_RU font layer handles the CP1251 mapping.
 */
class RussianKeyboard
{
  public:
    enum ControlKey : uint8_t { CTRL_NONE = 0, CTRL_SPACE, CTRL_BACKSPACE, CTRL_SEND };

    void reset();
    void setInputText(const std::string &text) { inputText = text; }
    const std::string &getInputText() const { return inputText; }
    void setCallback(std::function<void(const std::string &)> callback) { onSubmit = callback; }

    // Returns true when the event was consumed by the keyboard
    bool handleInput(input_broker_event event);

    // Draw keyboard with the top edge at startY; the input line is drawn by the caller
    void draw(OLEDDisplay *display, int16_t startY);

    // Draw the one-line input preview (with cursor) at the given position
    void drawInputLine(OLEDDisplay *display, int16_t x, int16_t y, int16_t width);

  private:
    static const uint8_t LETTER_ROWS = 3;
    static const uint8_t LETTER_COLS = 11;
    static const uint8_t TOTAL_ROWS = 4; // 3 letter rows + control row
    // UTF-8 encoded Russian alphabet, row-major
    static const char *const LETTERS[LETTER_ROWS][LETTER_COLS];

    uint8_t cursorRow = 0;
    uint8_t cursorCol = 0;
    std::string inputText;
    std::function<void(const std::string &)> onSubmit;

    void pressCurrentKey();
    void submit();
};

} // namespace graphics

#endif // STANDALONE_UI
