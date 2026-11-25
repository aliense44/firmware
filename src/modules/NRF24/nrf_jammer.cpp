#include "nrf_jammer.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "nrf_common.h"
#include <globals.h>

// NOWE: Zmienne dla załadowanych kanałów
static uint8_t loadedChannels[124]; // Maksymalna liczba kanałów
static uint8_t loadedChannelCount = 0;
static bool scanFileLoaded = false;
static uint8_t loadedCategoryMask[124];
static const char *SCAN_FOLDER_PATH = "/BruceNRF";

static constexpr unsigned long NRF_SAFETY_TIMEOUT_MS = 300000UL;
static unsigned long nrfSelectedTimeoutMs = NRF_SAFETY_TIMEOUT_MS;

static void shuffleChannels(uint8_t *arr, size_t count) {
    for (size_t i = count - 1; i > 0; i--) {
        size_t j = esp_random() % (i + 1);
        uint8_t tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}
static String formatNrfTimeout(unsigned long ms) {
    unsigned long seconds = ms / 1000UL;

    if (seconds < 60UL) { return String(seconds) + " s"; }

    unsigned long minutes = seconds / 60UL;
    unsigned long restSeconds = seconds % 60UL;

    if (restSeconds == 0UL) { return String(minutes) + " min"; }

    return String(minutes) + " min " + String(restSeconds) + " s";
}

// Formatowanie liczby kanałów do wyświetlania
static String formatChannelCount(uint8_t count) {
    if (count < 10) return "0" + String(count);
    return String(count);
}

// Załaduj plik CSV ze skanera
static bool loadScanFile(const String &selectedFilePath) {
    loadedChannelCount = 0;
    scanFileLoaded = false;

    // Sprawdź czy karta microSD jest włożona
    if (!SD.begin()) {
        Serial.println("SD card not found, using default channels");
        return false;
    }

    // Otwórz plik
    File scanFile = SD.open(selectedFilePath, FILE_READ);
    if (!scanFile) {
        Serial.println("Cannot open scan file: " + selectedFilePath);
        return false;
    }
    String line;
    bool firstLine = true;

    while (scanFile.available() && loadedChannelCount < 124) {
        // Odczytaj wiersz
        char buffer[256];
        int charsRead = 0;

        while (scanFile.available() && charsRead < 255) {
            char c = scanFile.read();
            if (c == '\n' || c == '\r') break;
            buffer[charsRead++] = c;
        }
        buffer[charsRead] = '\0';
        line = String(buffer);

        line.trim();
        if (line.isEmpty()) continue;

        // Pomiń nagłówek
        if (firstLine) {
            firstLine = false;
            continue;
        }

        // Pętla odczytu już przerywa na \n/\r, więc line nie zawiera znaków nowej linii
        String data = line;

        if (data.isEmpty()) continue;

        // Rozdziel kolumny po przecinku
        // Plik ma format: format_version,nrf_channel,frequency_mhz,...
        // Pomijamy pierwszą kolumnę (format_version) i bierzemy drugą jako kanał, trzecią jako częstotliwość
        int firstComma = data.indexOf(',');
        int secondComma = data.indexOf(',', firstComma + 1);
        int thirdComma = data.indexOf(',', secondComma + 1);
        int fourthComma = data.indexOf(',', thirdComma + 1);
        int fifthComma = data.indexOf(',', fourthComma + 1);
        int sixthComma = data.indexOf(',', fifthComma + 1);
        int seventhComma = data.indexOf(',', sixthComma + 1);
        // CSV v5 ma 8 kolumn (7 przecinków) - category_mask jest po siódmym przecinku

        uint8_t categoryMask = 0xFF; // Domyślnie wszystkie kategorie jeśli brak danych

        if (seventhComma > sixthComma && seventhComma >= 0) {
            String maskStr = data.substring(seventhComma + 1);
            maskStr.trim();
            maskStr.replace("\"", "");
            if (maskStr.length() > 0) {
                uint8_t parsedMask = (uint8_t)maskStr.toInt();
                // Przy nie-numerycznej wartości toInt() zwraca 0 - zachowaj domyślną maskę
                if (parsedMask > 0 || maskStr == "0") categoryMask = parsedMask;
            }
        }

        if (firstComma > 0 && secondComma > firstComma + 1) {
            // Druga kolumna to nrf_channel
            String channelStr = data.substring(firstComma + 1, secondComma);
            // Trzecia kolumna to frequency_mhz (do trzeciej przecinki)
            String freqStr = data.substring(secondComma + 1, thirdComma);

            if (channelStr.length() > 0 && freqStr.length() > 0) {
                uint8_t nrfChannel = channelStr.toInt();
                float frequency = freqStr.toFloat();

                // Walidacja: kanał 1-127, częstotliwość 2.4-2.5 GHz
                if (nrfChannel >= 1 && nrfChannel <= 127 && frequency >= 2400 && frequency <= 2500) {
                    loadedChannels[loadedChannelCount] = nrfChannel;
                    loadedCategoryMask[loadedChannelCount] = categoryMask;
                    loadedChannelCount++;
                }
            }
        }
    }

    scanFile.close();

    Serial.print("Loaded ");
    Serial.print(loadedChannelCount);
    Serial.println(" channels from scan file");

    scanFileLoaded = (loadedChannelCount > 0);
    return scanFileLoaded;
}

void nrf_jammer() {
    for (;;) {
        int selected = -1;
        std::vector<Option> jammerOptions = {
            {"Start",                                            [&]() { selected = 1; }},
            {"Load Scan File",
             [&]() {
                 String selectedFilePath = "";

                 if (!SD.begin()) {
                     Serial.println("SD card not found");
                     selected = 3;
                     return;
                 }

                 File folder = SD.open(SCAN_FOLDER_PATH);
                 if (!folder || !folder.isDirectory()) {
                     Serial.println("Cannot open folder: " + String(SCAN_FOLDER_PATH));
                     selected = 3;
                     return;
                 }

                 std::vector<String> fileNames;
                 File entry;

                 while ((entry = folder.openNextFile())) {
                     String name = String(entry.name());

                     if (name.endsWith(".csv")) { fileNames.push_back(name); }

                     entry.close();
                 }
                 folder.close();

                 if (fileNames.empty()) {
                     Serial.println("No CSV files found in: " + String(SCAN_FOLDER_PATH));
                     selected = 3;
                     return;
                 }

                 std::vector<Option> fileOptions;

                 for (const String &name : fileNames) {
                     fileOptions.push_back({name, [&selectedFilePath, name]() {
                                                selectedFilePath = String(SCAN_FOLDER_PATH) + "/" + name;
                                            }});
                 }

                 fileOptions.push_back({"Back", []() {}});

                 loopOptions(fileOptions, MENU_TYPE_SUBMENU, "Select scan file");

                 if (!selectedFilePath.isEmpty()) {
                     if (loadScanFile(selectedFilePath)) {
                         Serial.println("Scan file loaded successfully");
                     } else {
                         Serial.println("Failed to load scan file");
                     }
                 }

                 selected = 3;
             }                                                                          },
            {"Limit: " + formatNrfTimeout(nrfSelectedTimeoutMs), [&]() { selected = 2; }},
            {"Back",                                             [&]() { selected = 0; }}
        };

        loopOptions(jammerOptions, MENU_TYPE_SUBMENU, "NRF Jammer");

        if (selected <= 0) return;
        if (selected == 1) break;
        if (selected == 3) continue;

        std::vector<Option> timeoutOptions = {
            {"30 s",  []() { nrfSelectedTimeoutMs = 30000UL; } },
            {"1 min", []() { nrfSelectedTimeoutMs = 60000UL; } },
            {"2 min", []() { nrfSelectedTimeoutMs = 120000UL; }},
            {"3 min", []() { nrfSelectedTimeoutMs = 180000UL; }},
            {"5 min", []() { nrfSelectedTimeoutMs = 300000UL; }},
            {"Back",  []() {}                                  }
        };

        loopOptions(timeoutOptions, MENU_TYPE_SUBMENU, "Safety timeout");
    }
    int OnX = 0;
    uint8_t hopping_mode = 0; // 0: sequential, 1: random
    NRF24_MODE mode = nrf_setMode();
    int NRFOnline = 1;

    byte Test_channels[] = {50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 2,  4,  6,  8,
                            10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48};

    byte wifi_channels[] = {2, 7, 12, 17, 22, 27, 32, 37, 42, 47, 52, 57, 62, 67, 72, 77};

    byte ble_channels[] = {2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                           22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41};

    byte ble_adv_priority[] = {37, 38, 39, 1, 2, 3, 25, 26, 27, 79, 80, 81};

    byte bluetooth_channels[] = {2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
                                 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
                                 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
                                 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
                                 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80};
    byte usb_channels[] = {32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70};
    byte video_channels[] = {60, 62, 64, 66,  68,  70,  72,  74,  76,  78,  80,  82,  84,  86,  88,  90, 92,
                             94, 96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124};
    byte rc_channels[] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39};

    byte full_channels[] = {1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,
                            17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
                            33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
                            49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,
                            65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,
                            81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,
                            97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
                            113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124};
    byte zigbee_channels[] = {
        4,  5,  6,  // ch11
        9,  10, 11, // ch12
        14, 15, 16, // ch13
        19, 20, 21, // ch14
        24, 25, 26, // ch15
        29, 30, 31, // ch16
        34, 35, 36, // ch17
        39, 40, 41, // ch18
        44, 45, 46, // ch19
        49, 50, 51, // ch20
        54, 55, 56, // ch21
        59, 60, 61, // ch22
        64, 65, 66, // ch23
        69, 70, 71, // ch24
        74, 75, 76, // ch25
        79, 80, 81  // ch26
    };

    struct jamMode {
        const char *name;
        byte *channels;
        size_t count;
    };

    jamMode modes[] = {
        {"Test        ", Test_channels,      sizeof(Test_channels) / sizeof(Test_channels[0])          },
        {"WiFi        ", wifi_channels,      sizeof(wifi_channels) / sizeof(wifi_channels[0])          },
        {"BLEch       ", ble_channels,       sizeof(ble_channels) / sizeof(ble_channels[0])            },
        {"BLE Adv Pri ", ble_adv_priority,   sizeof(ble_adv_priority) / sizeof(ble_adv_priority[0])    },
        {"Bluetooth   ", bluetooth_channels, sizeof(bluetooth_channels) / sizeof(bluetooth_channels[0])},
        {"USB         ", usb_channels,       sizeof(usb_channels) / sizeof(usb_channels[0])            },
        {"Video Stream", video_channels,     sizeof(video_channels) / sizeof(video_channels[0])        },
        {"RC          ", rc_channels,        sizeof(rc_channels) / sizeof(rc_channels[0])              },
        {"Zigbee      ", zigbee_channels,    sizeof(zigbee_channels) / sizeof(zigbee_channels[0])      },
        {"Full        ", full_channels,      sizeof(full_channels) / sizeof(full_channels[0])          }
    };

    if (nrf_start(mode)) {

        int modeIndex = 0;
        int hopIndex = 0;
        bool redraw = true;
        bool redrawTimer = true;

        unsigned long lastTimerBucket = ULONG_MAX;

        int timerX = 0;
        int timerY = -1;
        int timerHeight = 0;
        bool safetyTimeoutReached = false;
        unsigned long jamStartMs = millis();
        unsigned long lastRedrawMs = 0;

        // NOWE: Inicjalizacja kanałów ze skanera
        if (scanFileLoaded) {
            Serial.println("Using channels from scan file");
            // Ustaw hopping_mode na losowe jeśli mamy załadowany plik
            hopping_mode = 1;
        }

        uint8_t shuffled_idx[124];

        if (CHECK_NRF_SPI(mode)) {
            NRFradio.setPALevel(RF24_PA_MAX);
            NRFradio.startConstCarrier(RF24_PA_MAX, 50);
            NRFradio.setAddressWidth(5);
            NRFradio.setPayloadSize(2);
            if (!NRFradio.setDataRate(RF24_2MBPS)) {
                Serial.println("Failed to set data rate to 2Mbps, trying 1Mbps");
                if (!NRFradio.setDataRate(RF24_1MBPS)) {
                    Serial.println("Failed to set data rate to 1Mbps, trying 250kbps");
                    if (!NRFradio.setDataRate(RF24_250KBPS)) {
                        Serial.println("Failed to set data rate to 250kbps, giving up");
                    }
                }
            }
        }

        drawMainBorder();

        NRFSerial.println("RADIOS");
        vTaskDelay(50 / portTICK_PERIOD_MS);

        while (true) {
            const unsigned long timeoutMs =
                (nrfSelectedTimeoutMs > 0 && nrfSelectedTimeoutMs <= NRF_SAFETY_TIMEOUT_MS)
                    ? nrfSelectedTimeoutMs
                    : NRF_SAFETY_TIMEOUT_MS;

            if (millis() - jamStartMs >= timeoutMs) {
                safetyTimeoutReached = true;
                break;
            }

            if ((CHECK_NRF_UART(mode)) || (CHECK_NRF_BOTH(mode))) {
                if (OnX == 0) {
                    NRFSerial.println("RADIOS");
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                }

                if (NRFSerial.available()) {
                    String incomingNRFs = NRFSerial.readStringUntil('\n');
                    incomingNRFs.trim();
                    if (incomingNRFs.length() == 1 && isDigit(incomingNRFs.charAt(0))) {
                        OnX = 1;
                        NRFOnline = (incomingNRFs.toInt());
                        if (CHECK_NRF_BOTH(mode)) { NRFOnline = (incomingNRFs.toInt()) + 1; }
                    }
                }
            }

            unsigned long elapsedMs = millis() - jamStartMs;
            unsigned long remainingMs = (elapsedMs < timeoutMs) ? (timeoutMs - elapsedMs) : 0UL;
            unsigned long remainingSeconds = (remainingMs + 999UL) / 1000UL;

            const unsigned long timerBucket = (remainingSeconds + 9UL) / 10UL;

            if (timerBucket != lastTimerBucket) {
                lastTimerBucket = timerBucket;
                redrawTimer = true;
            }
            uint8_t filteredChannels[124];
            uint8_t ch_idx;
            size_t activeCount = 0;
            const uint8_t *activeChannels = nullptr;
            bool need_reshuffle = true;

            // Map trybów do pełnych masek kategorii (zgodnie z nrf_spectrum.cpp)
            static const uint8_t modeCategoryMasks[] = {
                0xFF, // Test - wszystkie
                0x01, // WiFi
                0x02, // BLE
                0x04, // BLE adv
                0x08, // Bluetooth
                0x10, // USB
                0x20, // Video
                0x40, // RC
                0x80, // Zigbee
                0xFF  // Full - wszystkie
            };

            // Aktualizacja filtrowania kanałów przed wyświetleniem (aby activeCount był poprawny)
            if (scanFileLoaded) {
                activeCount = 0;
                uint8_t requiredMask = modeCategoryMasks[modeIndex];

                for (size_t i = 0; i < loadedChannelCount; i++) {
                    if ((loadedCategoryMask[i] & requiredMask) == requiredMask) {
                        filteredChannels[activeCount] = loadedChannels[i];
                        activeCount++;
                    }
                }

                if (activeCount > 0) {
                    activeChannels = filteredChannels;
                } else {
                    activeChannels = loadedChannels;
                    activeCount = loadedChannelCount;
                }
            }

            if (redraw && (millis() - lastRedrawMs >= 500)) {
                drawMainBorderWithTitle("NRF JAMMER", false);
                printSubtitle("NRF function Jammer");

                const int contentX = tft.getCursorX();
                const int contentY = tft.getCursorY();
                const int contentHeight = tftHeight - contentY - 6;

                if (contentHeight > 0) {
                    tft.fillRect(6, contentY, tftWidth - 12, contentHeight, bruceConfig.bgColor);
                }

                tft.setCursor(contentX, contentY);

                padprintln("STATUS : " + String(NRFOnline) + " ACTIVE");
                String _modeName = String(modes[modeIndex].name);
                _modeName.trim();

                String _countText = " (" + String(activeCount) + " ch)";
                const unsigned int maxTextLength = 18;

                if (_modeName.length() + _countText.length() > maxTextLength) {
                    _modeName = _modeName.substring(0, maxTextLength - _countText.length());
                }

                padprintln("MODE : " + _modeName + _countText);
                padprintln("HOP  : " + String(hopping_mode == 0 ? "Sequential " : "FHSS        "));
                if (scanFileLoaded) {
                    padprintln("SCAN : " + formatChannelCount(loadedChannelCount) + " channels loaded");
                } else {
                    padprintln("SCAN : Using default channels");
                }
                timerX = tft.getCursorX();
                timerY = tft.getCursorY();
                timerHeight = tft.fontHeight();

                padprintln("TIME : " + formatNrfTimeout(remainingSeconds * 1000UL) + " (10 s refresh)");

                redrawTimer = false;
                padprintln("");
                padprintln("> Switch Mode: Next/Prev");
                padprintln("> Hop Mode: Sel");
                padprintln("> Exit: Esc");

                tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
                if ((CHECK_NRF_UART(mode)) || (CHECK_NRF_BOTH(mode))) {
                    String Mode = modes[modeIndex].name;
                    Mode.replace(" ", "");
                    NRFSerial.println(Mode);
                }
                redraw = false;
            }
            if (redrawTimer && timerY >= 0 && (millis() - lastRedrawMs >= 100)) {
                const int savedX = tft.getCursorX();
                const int savedY = tft.getCursorY();

                tft.fillRect(6, timerY, tftWidth - 12, timerHeight, bruceConfig.bgColor);

                tft.setCursor(timerX, timerY);

                padprintln("TIME : " + formatNrfTimeout(remainingSeconds * 1000UL) + " (10 s refresh)");

                tft.setCursor(savedX, savedY);
                redrawTimer = false;
            }
            hopIndex++;
            // Zawijanie hopIndex według liczby kanałów z CSV lub domyślnego trybu
            int channelLimit =
                (scanFileLoaded && activeCount > 0) ? (int)activeCount : (int)modes[modeIndex].count;
            if (hopIndex >= channelLimit) {
                hopIndex = 0;
                need_reshuffle = true;
            }

            // NOWE: Wybór kanału do zakłócania
            if (activeCount > 0 && activeChannels != nullptr) {
                if (hopping_mode == 1) {
                    if (need_reshuffle) {
                        for (size_t i = 0; i < activeCount; i++) shuffled_idx[i] = activeChannels[i];

                        shuffleChannels(shuffled_idx, activeCount);
                        need_reshuffle = false;
                    }

                    ch_idx = shuffled_idx[hopIndex];
                } else {
                    ch_idx = activeChannels[hopIndex];
                }
            } else {
                if (hopping_mode == 1) {
                    if (need_reshuffle) {
                        size_t cnt = sizeof(Test_channels) / sizeof(Test_channels[0]);

                        for (size_t i = 0; i < cnt; i++) shuffled_idx[i] = Test_channels[i];

                        shuffleChannels(shuffled_idx, cnt);
                        need_reshuffle = false;
                    }

                    ch_idx = shuffled_idx[hopIndex];
                } else {
                    ch_idx = hopIndex;
                }
            }
            if (CHECK_NRF_SPI(mode)) { NRFradio.setChannel(ch_idx); }

            if (check(NextPress)) {
                modeIndex++;
                if (modeIndex >= (int)(sizeof(modes) / sizeof(modes[0]))) modeIndex = 0;
                hopIndex = 0;
                need_reshuffle = true;
                redraw = true;
            }
            if (check(PrevPress)) {
                modeIndex--;
                if (modeIndex < 0) modeIndex = (sizeof(modes) / sizeof(modes[0])) - 1;
                hopIndex = 0;
                need_reshuffle = true;
                redraw = true;
            }
            if (check(EscPress)) break;
            if (check(SelPress)) {
                hopping_mode++;
                if (hopping_mode > 1) hopping_mode = 0;
                hopIndex = 0;
                need_reshuffle = true;
                redraw = true;
            }
        }

        if (CHECK_NRF_SPI(mode)) {
            NRFradio.stopConstCarrier();
            digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);
        }
        if ((CHECK_NRF_UART(mode)) || (CHECK_NRF_BOTH(mode))) { NRFSerial.println("OFF"); }

        if (safetyTimeoutReached) { displayWarning("Safety timeout reached", true); }

    } else {
        displayError("NRF24 not found", true);
    }
}
