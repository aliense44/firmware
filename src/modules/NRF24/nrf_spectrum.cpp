#include "nrf_spectrum.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/spectrum_plot.h"
#include "core/sd_functions.h"

#define CHANNELS 80
uint8_t channel[CHANNELS];
static bool lastRpd[CHANNELS];
#define NRF_FULL_SCALE 125

String scanChannels(bool web) {
    String result = "{";
    uint8_t rpdValues[CHANNELS] = {0};
    digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);
    for (int i = 0; i < CHANNELS; i++) {
        NRFradio.setChannel(i);
        NRFradio.startListening();
        delayMicroseconds(128);
        NRFradio.stopListening();
        int rpd = NRFradio.testRPD() ? 1 : 0;
        lastRpd[i] = rpd != 0;
        channel[i] = (channel[i] * 3 + rpd * NRF_FULL_SCALE) / 4;
        rpdValues[i] = channel[i];
    }
    digitalWrite(bruceConfigPins.NRF24_bus.io0, HIGH);
    if (web) {
        for (int i = 0; i < CHANNELS; i++) {
            if (i > 0) result += ",";
            result += String(rpdValues[i]);
        }
        result += "}";
    }
    return result;
}

// Frequency context only, not protocol identification.
static String nrf_rangeLabel(uint8_t ch) {
    if (ch == 2 || ch == 26) return "BLE adv / shared";
    if (ch >= 4 && (ch % 5 == 0 || ch % 5 == 1 || ch % 5 == 4))
        return "Zigbee / shared";
    if (ch >= 2) return "BLE / Wi-Fi range";
    return "2.4 GHz range";
}

// CSV v2: cumulative session snapshot. Elapsed time includes UI and SD pauses;
// it is not continuous receiver dwell time. Saving never clears detections.
static String nrf_save_report(const uint8_t *active, int count, uint32_t sessionElapsedMs) {
    if (count <= 0) return "No channels found to save";
    if (count > CHANNELS) return "Invalid report";
    String csv;
    if (!csv.reserve(160 + count * 48)) return "Not enough memory";
    csv = "format_version,nrf_channel,frequency_mhz,session_elapsed_ms,scan_min_channel,scan_max_channel\n";
    for (int i = 0; i < count; ++i) {
        if (active[i] >= CHANNELS) return "Invalid channel";
        csv += "2," + String((unsigned int)active[i]) + "," +
               String(2400u + active[i]) + "," + String(sessionElapsedMs) + ",0,79\n";
    }
    NRFradio.stopListening();
    digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);
    if (!setupSdCard()) return "SD unavailable";
    const char *folder = "/BruceNRF";
    if (!folderExists(SD, folder) && !SD.mkdir(folder)) return "Cannot create /BruceNRF";
    char path[40];
    bool found = false;
    for (unsigned int n = 1; n <= 9999; ++n) {
        if (check(EscPress) || returnToMenu) return "Save cancelled";
        snprintf(path, sizeof(path), "/BruceNRF/scan_%04u.csv", n);
        if (!SD.exists(path)) { found = true; break; }
        if (n % 32 == 0) vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (!found) return "No free report filename";
    File out = SD.open(path, FILE_WRITE);
    if (!out) return "Cannot open report file";
    size_t written = out.print(csv);
    out.flush();
    bool ok = written == csv.length() && out.size() == csv.length();
    out.close();
    if (ok) {
        File verify = SD.open(path, FILE_READ);
        ok = (bool)verify;
        if (ok) {
            ok = verify.size() == csv.length();
            for (size_t i = 0; ok && i < csv.length(); ++i)
                ok = verify.read() == (uint8_t)csv[i];
            verify.close();
        }
    }
    if (!ok) {
        bool removed = SD.remove(path);
        Serial.println(String("NRF CSV verification failed: ") + path);
        return removed ? "Write failed; file removed" : "Write failed; check SD file";
    }
    Serial.println(String("NRF report saved: ") + path);
    return String("Saved: ") + path;
}

static void nrf_active_report() {
    // Session-local state: reset only by leaving and reopening this view.
    bool seen[CHANNELS] = {false};
    uint8_t active[CHANNELS] = {0};
    int count = 0, page = 0;
    const int rowH = 12;
    int rows = (tft.height() - 70) / rowH;
    if (rows < 1) rows = 1;
    const uint32_t sessionStart = millis();
    uint32_t lastRefresh = millis();
    uint32_t lastSweep = millis() - 20;
    uint32_t statusStart = 0;
    String status;
    bool redraw = true;
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextWrap(false);
    check(SelPress);
    while (true) {
        if (check(EscPress) || returnToMenu) break;
        uint32_t now = millis();
        if (now - lastSweep >= 20) {
            lastSweep = now;
            scanChannels();
            bool changed = false;
            for (int i = 0; i < CHANNELS; ++i) {
                if (lastRpd[i] && !seen[i]) {
                    seen[i] = true;
                    changed = true;
                }
            }
            // Keep a sorted, duplicate-free list after every complete sweep.
            if (changed) {
                count = 0;
                for (int i = 0; i < CHANNELS; ++i)
                    if (seen[i]) active[count++] = (uint8_t)i;
            }
            if (check(EscPress) || returnToMenu) break;
        }
        if (millis() - lastRefresh >= 500) {
            lastRefresh = millis();
            redraw = true;
        }
        int pages = count ? (count + rows - 1) / rows : 1;
        if (check(NextPress)) { page = (page + 1) % pages; redraw = true; }
        if (check(PrevPress)) { page = (page + pages - 1) % pages; redraw = true; }
        if (check(SelPress)) {
            status = nrf_save_report(active, count, millis() - sessionStart);
            // Do not reset seen[], active[], count or sessionStart here.
            lastSweep = millis() - 20;
            statusStart = millis();
            redraw = true;
        }
        if (status.length() && millis() - statusStart >= 4000) {
            status = "";
            redraw = true;
        }
        if (redraw) {
            redraw = false;
            tft.fillScreen(bruceConfig.bgColor);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.setCursor(4, 4);
            tft.print("NRF REPORT  Session: " + String((millis() - sessionStart) / 1000) + "s");
            tft.setCursor(4, 16);
            tft.print("Found: " + String(count) + "  Page: " + String(page + 1) + "/" + String(pages));
            tft.setCursor(4, 28);
            tft.print("CH   MHz    Range*");
            if (count == 0) {
                tft.setCursor(4, 40);
                tft.print("Scanning... No channels found yet");
            }
            for (int r = 0; r < rows && page * rows + r < count; ++r) {
                unsigned int ch = active[page * rows + r];
                char number[20];
                snprintf(number, sizeof(number), "%02u   %u   ", ch, 2400u + ch);
                tft.setCursor(4, 40 + r * rowH);
                tft.print(String(number) + nrf_rangeLabel((uint8_t)ch));
            }
            tft.setCursor(4, tft.height() - 24);
            tft.print(status.length() ? status : String("*Range only; ENTER: Save SD"));
            tft.setCursor(4, tft.height() - 12);
            tft.print("Prev/Next: page  ESC: Back");
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    tft.setTextWrap(true);
    tft.setTextSize(FM);
}

static void nrf_envelope(const uint8_t *lvl, uint8_t *env, int plotW) {
    for (int i = 0; i < plotW; i++) {
        int32_t pos = (int32_t)i * (CHANNELS - 1) * 256 / (plotW - 1);
        int ci = pos >> 8;
        int frac = pos & 0xff;
        if (ci >= CHANNELS - 1) { ci = CHANNELS - 2; frac = 256; }
        int v = lvl[ci] + (lvl[ci + 1] - lvl[ci]) * frac / 256;
        v = v * 100 / NRF_FULL_SCALE;
        env[i] = (uint8_t)(v < 0 ? 0 : (v > 100 ? 100 : v));
    }
}

void nrf_spectrum(NrfSpectrumView view) {
    SpectrumPlot plot;
    if (!plot.begin("NRF Spectrum")) { displayError("Out of memory", true); return; }
    const int plotW = plot.width();
    uint8_t *env = (uint8_t *)malloc(plotW);
    uint8_t *envPeak = (uint8_t *)malloc(plotW);
    uint8_t peak[CHANNELS] = {0};
    if (!env || !envPeak) {
        free(env); free(envPeak); plot.end();
        displayError("Out of memory", true); return;
    }
    const int tickCount = 5;
    int cols[tickCount];
    String labels[tickCount];
    for (int i = 0; i < tickCount; i++) {
        int ch = i * (CHANNELS - 1) / (tickCount - 1);
        cols[i] = ch * (plotW - 1) / (CHANNELS - 1);
        labels[i] = String(2.400f + ch * 0.001f, 2);
    }
    plot.ruler(cols, labels, tickCount);
    plot.status("starting radio...");
    if (!nrf_start(NRF_MODE_SPI)) {
        Serial.println("Fail Starting radio");
        free(env); free(envPeak); plot.end();
        displayError("NRF24 not found"); delay(500); return;
    }
    NRFradio.setAutoAck(false);
    NRFradio.disableCRC();
    NRFradio.setAddressWidth(2);
    const uint8_t noiseAddress[][2] = {
        {0x55, 0x55}, {0xAA, 0xAA}, {0xA0, 0xAA},
        {0xAB, 0xAA}, {0xAC, 0xAA}, {0xAD, 0xAA}
    };
    for (uint8_t i = 0; i < 6; ++i) { NRFradio.openReadingPipe(i, noiseAddress[i]); }
    NRFradio.setDataRate(RF24_1MBPS);
    if (view == NrfSpectrumView::ACTIVE_ONLY) {
        nrf_active_report();
        NRFradio.stopListening();
        NRFradio.powerDown();
        free(env); free(envPeak); plot.end();
        return;
    }
    uint32_t lastFrame = 0, lastRow = 0;
    while (!check(EscPress)) {
        scanChannels();
        int maxCh = 0;
        for (int i = 0; i < CHANNELS; i++) {
            if (channel[i] > peak[i]) peak[i] = channel[i];
            else if (peak[i]) peak[i]--;
            if (channel[i] > channel[maxCh]) maxCh = i;
        }
        if (millis() - lastFrame >= 40) {
            lastFrame = millis();
            nrf_envelope(channel, env, plotW);
            nrf_envelope(peak, envPeak, plotW);
            int hlC = maxCh * (plotW - 1) / (CHANNELS - 1);
            int hlSpan = (2 * (plotW - 1)) / (CHANNELS - 1);
            plot.trace(env, envPeak, hlC - hlSpan, hlC + hlSpan);
            if (millis() - lastRow >= 120) {
                lastRow = millis();
                plot.pushRow(env);
                plot.status(
                    "peak ch" + String(maxCh) + "  " + String(2.400f + maxCh * 0.001f, 3) + "GHz  " +
                    String(env[hlC]) + "%"
                );
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    NRFradio.stopListening();
    NRFradio.powerDown();
    free(env); free(envPeak); plot.end();
    delay(250);
}
