#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "core/serialcmds.h"
#include "sd_functions.h" // to catch FileList Struct
#include <FS.h>
#include <LittleFS.h>
#include <SD.h>
#include <globals.h>
#define BORDER_OFFSET_FROM_SCREEN_EDGE 5
#define BORDER_PAD_X (LW * FP + 4)
#define STATUS_BAR_HEIGHT (LH * FM + 14)
#define BORDER_PAD_Y (STATUS_BAR_HEIGHT - 2)
#define MENU_TYPE_MAIN 0
#define MENU_TYPE_SUBMENU 1
#define MENU_TYPE_REGULAR 2

// Columns of the main menu grid layout, set by MainMenu::begin().
// 0 means the carousel layout is active and navigation stays linear.
extern uint8_t mainMenuGridColumns;

// Set once by MainMenu's constructor. Lets loopOptions() offer a page-up/page-down tap zone for
// the grid without display.cpp depending on MainMenu's header. Returns true and writes newIndex
// when (x, y) landed in the page zone (only meaningful when mainMenuGridColumns > 1).
extern bool (*gridPageTapHandler)(int x, int y, int currentIndex, int &newIndex);

void panelSleep(bool on);
void turnOffDisplay();
bool wakeUpScreen();

struct Opt_Coord {
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t size = 10;
    uint16_t fgcolor = bruceConfig.priColor;
    uint16_t bgcolor = bruceConfig.bgColor;
};
void displayScrollingText(const String &text, Opt_Coord &coord, bool highlight = false);

#if !defined(LITE_VERSION)

#include <AnimatedGIF.h>

struct GifPosition {
    int x;
    int y;

    GifPosition(int xCoord, int yCoord) : x(xCoord), y(yCoord) {}
};

class Gif {
public:
    Gif();

    ~Gif();

    bool openGIF(FS *fs, const char *filename);

    int playFrame(int x = 0, int y = 0, bool bSync = true);

    int getInfo(GIFINFO *pInfo) { return gif->getInfo(pInfo); }

    void reset() { return gif->reset(); }

    void close() { return gif->close(); }

    int getCanvasWidth() { return gif->getCanvasWidth(); }

    int getCanvasHeight() { return gif->getCanvasHeight(); }

    int getLastError();

    AnimatedGIF *gif;

private:
    static FS *GifFs;

    GifPosition gifPosition;

    static void *openFile(const char *fname, int32_t *pSize);

    static void closeFile(void *pHandle);

    static int32_t readFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen);

    static int32_t seekFile(GIFFILE *pFile, int32_t iPosition);

    static void GIFDraw(GIFDRAW *pDraw);
};
#endif
/*
 * @name drawImg
 * @param fs: File system
 * @param filename: String of the file path
 * @param x: If center=false, x is the X coordinate to start drawing, if center=true, x is the x offset
 * @param y: If center=false, y is the Y coordinate to start drawing, if center=true, y is the y offset
 * @param center: draw the image at the center of the screen
 * @param playDurationMs: time that the GIF will be played
 */
bool drawImg(
    FS &fs, const String &filename, int x = 0, int y = 0, bool center = false, int playDurationMs = 0,
    bool resetButtonStatus = true
);
bool drawPNG(FS &fs, const String &filename, int x, int y, bool center);
bool preparePngBin(FS &fs, const String &filename);
bool drawBmp(FS &fs, const String &filename, int x = 0, int y = 0, bool center = false);
#if !defined(LITE_VERSION)
bool showGif(
    FS *fs, const char *filename, int x = 0, int y = 0, bool center = false, int playDurationMs = 0,
    bool clearButtonStatus = true
);
#endif
bool showJpeg(FS &fs, const String &filename, int x = 0, int y = 0, bool center = false);
bool showJpeg(const uint8_t *data_array, size_t data_size, int x, int y, bool center = false);

uint16_t getComplementaryColor(uint16_t color);
uint16_t getComplementaryColor2(uint16_t color);
uint16_t getColorVariation(uint16_t color, int delta = 10, int direction = 0);
// Linear RGB565 mix: t = 0 returns a, t = 255 returns b. Use it to derive dim
// or highlighted shades from the theme instead of hardcoding TFT_* constants.
uint16_t blendColors(uint16_t a, uint16_t b, uint8_t t);
// Fills lut[0..n-1] with a theme ramp going from the background up to a
// brightened primary, for waterfalls and other intensity plots.
void buildHeatPalette(uint16_t *lut, uint8_t n);

void resetTftDisplay(
    int x = 0, int y = 0, uint16_t fc = bruceConfig.priColor, int size = FM,
    uint16_t bg = bruceConfig.bgColor, uint16_t screen = bruceConfig.bgColor
);
void setTftDisplay(
    int x = 0, int y = 0, uint16_t fc = tft.getTextColor(), int size = tft.getTextSize(),
    uint16_t bg = tft.getTextBgColor()
);

void turnOffDisplay();
bool wakeUpScreen();

void displayRedStripe(const String &text, uint16_t fgcolor = TFT_WHITE, uint16_t bgcolor = TFT_RED);

int8_t displayMessage(
    const char *message, const char *leftButton, const char *centerButton, const char *rightButton,
    uint16_t color
);
void displayError(const String &txt, bool waitKeyPress = false);    // Red Stripe
void displayWarning(const String &txt, bool waitKeyPress = false);  // Yellow Stripe
void displayInfo(const String &txt, bool waitKeyPress = false);     // Blue Stripe
void displaySuccess(const String &txt, bool waitKeyPress = false);  // Green Strupe
void displayTextLine(const String &txt, bool waitKeyPress = false); // UI Colored stripe
void setPadCursor(int16_t padx = 1, int16_t pady = 0);

void padprintf(int16_t padx, const char *format, ...);
void padprintf(const char *format, ...);

void padprint(const String &s, int16_t padx = 1);
void padprint(const char str[], int16_t padx = 1);
void padprint(char c, int16_t padx = 1);
void padprint(unsigned char b, int base = DEC, int16_t padx = 1);
void padprint(int n, int base = DEC, int16_t padx = 1);
void padprint(unsigned int n, int base = DEC, int16_t padx = 1);
void padprint(long n, int base = DEC, int16_t padx = 1);
void padprint(unsigned long n, int base = DEC, int16_t padx = 1);
void padprint(long long n, int base = DEC, int16_t padx = 1);
void padprint(unsigned long long n, int base = DEC, int16_t padx = 1);
void padprint(double n, int digits, int16_t padx = 1);

void padprintln(const String &s, int16_t padx = 1);
void padprintln(const char str[], int16_t padx = 1);
void padprintln(char c, int16_t padx = 1);
void padprintln(unsigned char b, int base = DEC, int16_t padx = 1);
void padprintln(int n, int base = DEC, int16_t padx = 1);
void padprintln(unsigned int n, int base = DEC, int16_t padx = 1);
void padprintln(long n, int base = DEC, int16_t padx = 1);
void padprintln(unsigned long n, int base = DEC, int16_t padx = 1);
void padprintln(long long n, int base = DEC, int16_t padx = 1);
void padprintln(unsigned long long n, int base = DEC, int16_t padx = 1);
void padprintln(double n, int digits, int16_t padx = 1);

// loopOptions will now return the last index used in the function
int loopOptions(
    std::vector<Option> &options, uint8_t menuType, const char *subText, int index = 0,
    bool interpreter = false, bool letterShortcuts = false, uint16_t pageJumpSize = 0, bool border = true,
    bool compact = false
);
inline int loopOptions(std::vector<Option> &options, int _index) {
    return loopOptions(options, MENU_TYPE_REGULAR, "", _index, false);
}
inline int loopOptions(std::vector<Option> &options) {
    return loopOptions(options, MENU_TYPE_REGULAR, "", 0, false);
}

Opt_Coord drawOptions(
    int index, std::vector<Option> &options, uint16_t fgcolor, uint16_t selcolor, uint16_t bgcolor,
    bool firstRender = true, bool border = true, bool compact = false
);

void drawSubmenu(int index, std::vector<Option> &options, const char *title);

void drawStatusBar();
void drawMainBorder(bool clear = true);
void drawMainBorderWithTitle(const String &title, bool clear = true);
void printTitle(const String &title);
void printSubtitle(const String &subtitle, bool withLine = true);
void printFootnote(const String &text);
void printCenterFootnote(const String &text);

void drawWireguardStatus(int x, int y);

void progressHandler(int progress, size_t total, const String &message = "Running, Wait");

bool __attribute__((weak)) isCharging();

void drawBatteryStatus(uint8_t bat);

void drawWifiSmall(int x, int y);

void drawWebUISmall(int x, int y);

void drawBLESmall(int x, int y);

void drawBLE_beacon(int x, int y, uint16_t color);

void drawGPS(int x, int y);

void drawGpsSmall(int x, int y);

void drawSdSmall(int x, int y);

void drawCreditCard(int x, int y);

void drawMfkey32Icon(int x, int y);

void drawMfkey64Icon(int x, int y);

void TouchFooter(uint16_t color = bruceConfig.priColor);

void MegaFooter(uint16_t color = bruceConfig.priColor);


#endif
