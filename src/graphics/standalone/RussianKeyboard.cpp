#include "RussianKeyboard.h"

#ifdef STANDALONE_UI

#include "graphics/ScreenFonts.h"

namespace graphics
{

const char *const RussianKeyboard::LETTERS[LETTER_ROWS][LETTER_COLS] = {
    {"А", "Б", "В", "Г", "Д", "Е", "Ж", "З", "И", "Й", "К"},
    {"Л", "М", "Н", "О", "П", "Р", "С", "Т", "У", "Ф", "Х"},
    {"Ц", "Ч", "Ш", "Щ", "Ъ", "Ы", "Ь", "Э", "Ю", "Я"},
};

// Upper bound on the editable text, in UTF-8 bytes. Messages are sent in a fixed
// 233-byte protocol payload, so don't let the user type past it: extra characters
// would be silently dropped at send-time, which reads as a lost keys bug.
namespace
{
constexpr size_t kMaxInputBytes = 233;
}

void RussianKeyboard::reset()
{
    inputText.clear();
    cursorRow = 0;
    cursorCol = 0;
}

bool RussianKeyboard::handleInput(input_broker_event event)
{
    switch (event) {
    case INPUT_BROKER_LEFT:
        if (cursorCol > 0)
            cursorCol--;
        return true;
    case INPUT_BROKER_RIGHT:
        if (cursorRow < LETTER_ROWS) {
            if (cursorCol < LETTER_COLS - 1)
                cursorCol++;
        } else if (cursorCol < 2) { // control row has 3 keys
            cursorCol++;
        }
        return true;
    case INPUT_BROKER_UP:
        if (cursorRow > 0) {
            cursorRow--;
            if (cursorRow == LETTER_ROWS && cursorCol > 2)
                cursorCol = 2; // clamp when moving off the control row
        }
        return true;
    case INPUT_BROKER_DOWN:
        if (cursorRow < TOTAL_ROWS - 1) {
            cursorRow++;
            if (cursorRow == LETTER_ROWS) {
                // entering control row: map wide column range onto 3 keys
                if (cursorCol > 2)
                    cursorCol = 2;
            }
        }
        return true;
    case INPUT_BROKER_SELECT:
        pressCurrentKey();
        return true;
    case INPUT_BROKER_SELECT_LONG:
        submit();
        return true;
    default:
        return false;
    }
}

void RussianKeyboard::pressCurrentKey()
{
    if (inputText.size() >= kMaxInputBytes)
        return; // at the message length limit: ignore further typing

    if (cursorRow < LETTER_ROWS) {
        const char *ch = LETTERS[cursorRow][cursorCol];
        if (inputText.size() + strlen(ch) > kMaxInputBytes)
            return; // this Cyrillic char (2 bytes) would exceed the cap
        inputText += ch;
        return;
    }
    switch (cursorCol) {
    case 0: // ПРОБЕЛ
        inputText += ' ';
        break;
    case 1: // backspace (UTF-8 aware: strip a whole codepoint)
        while (!inputText.empty()) {
            char c = inputText.back();
            inputText.pop_back();
            if ((c & 0xC0) != 0x80)
                break; // stop after removing the leading byte
        }
        break;
    case 2: // ОТПРАВИТЬ
        submit();
        break;
    }
}

void RussianKeyboard::submit()
{
    if (onSubmit)
        onSubmit(inputText);
}

void RussianKeyboard::drawInputLine(OLEDDisplay *display, int16_t x, int16_t y, int16_t width)
{
    display->setFont(FONT_SMALL);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->setColor(WHITE);
    display->fillRect(x, y, width, FONT_HEIGHT_SMALL + 4);
    display->setColor(BLACK);
    std::string shown = inputText + "_";
    // Keep only the tail that fits on screen
    while (shown.length() > 1 && display->getStringWidth(shown.c_str()) > width - 4)
        shown.erase(shown.begin());
    display->drawString(x + 2, y + 2, shown.c_str());
    display->setColor(WHITE);
}

void RussianKeyboard::draw(OLEDDisplay *display, int16_t startY)
{
    const int16_t w = display->width();
    const int16_t keyW = w / LETTER_COLS;
    const int16_t keyH = FONT_HEIGHT_SMALL + 6;

    display->setFont(FONT_SMALL);

    for (uint8_t row = 0; row < LETTER_ROWS; row++) {
        for (uint8_t col = 0; col < LETTER_COLS; col++) {
            const int16_t kx = col * keyW;
            const int16_t ky = startY + row * keyH;
            const bool selected = (cursorRow == row && cursorCol == col);
            if (selected) {
                display->setColor(WHITE);
                display->fillRect(kx + 1, ky + 1, keyW - 2, keyH - 2);
                display->setColor(BLACK);
            } else {
                display->setColor(WHITE);
                display->drawRect(kx + 1, ky + 1, keyW - 2, keyH - 2);
            }
            display->setTextAlignment(TEXT_ALIGN_CENTER);
            display->drawString(kx + keyW / 2, ky + 3, LETTERS[row][col]);
            display->setColor(WHITE);
        }
    }

    // Control row: ПРОБЕЛ / ⌫ / ОТПРАВИТЬ
    static const char *const ctrlLabels[3] = {"ПРОБЕЛ", "<-", "ОТПРАВИТЬ"};
    const int16_t ctrlY = startY + LETTER_ROWS * keyH;
    const int16_t ctrlW[3] = {(int16_t)(w / 3), (int16_t)(w / 6), (int16_t)(w - w / 3 - w / 6)};
    int16_t cx = 0;
    for (uint8_t i = 0; i < 3; i++) {
        const bool selected = (cursorRow == LETTER_ROWS && cursorCol == i);
        if (selected) {
            display->setColor(WHITE);
            display->fillRect(cx + 1, ctrlY + 1, ctrlW[i] - 2, keyH - 2);
            display->setColor(BLACK);
        } else {
            display->setColor(WHITE);
            display->drawRect(cx + 1, ctrlY + 1, ctrlW[i] - 2, keyH - 2);
        }
        display->setTextAlignment(TEXT_ALIGN_CENTER);
        display->drawString(cx + ctrlW[i] / 2, ctrlY + 3, ctrlLabels[i]);
        display->setColor(WHITE);
        cx += ctrlW[i];
    }
}

} // namespace graphics

#endif // STANDALONE_UI
