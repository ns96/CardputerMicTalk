/**
 * @file mic_web.ino
 * @brief M5Cardputer Mic Web Server & Audio Visualizer
 * * WHAT THIS SKETCH DOES:
 * 1. Records audio from the built-in microphone into a circular buffer.
 * 2. Displays a real-time audio waveform on the Cardputer screen.
 * 3. Connects to WiFi (SD Card config first, then Hardcoded fallback).
 * 4. Serves two different Web Apps:
 * - Root (/): The sophisticated "Audio Console" (VU Meter Dashboard).
 * - Spectrum (/sv): A frequency spectrum analyzer.
 * 5. Provides an API endpoint (/data) that serves raw audio samples as JSON.
 * 6. Button A Logic:
 * - HOLD: Adjusts the microphone noise filter level.
 * - CLICK: Stops recording and plays back the last ~3 seconds of audio.
 * 7. Keyboard Logic:
 * - UP (';'): Increases Scaling Factor (SF) sent to clients.
 * - DOWN ('.'): Decreases Scaling Factor (SF).
 * - 'q': Displays CPU Load % and Loop Time (ms).
 * 8. Displays Host ID, Battery %, and feedback for NF/SF changes.
 * * @note Includes separate headers for VU Meter (webapp.h) and Spectrum (spectrum.h)
 */

#include <M5Cardputer.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h> // Added for SD Card support
#include "webapp.h"   // VU Meter (Root)
#include "spectrum.h" // Spectrum Analyzer (/sv)

// --- WI-FI SETTINGS (FALLBACK) ---
String wifi_ssid = "SA906";
String wifi_pass = "458C60F416";

// --- UI SETTINGS ---
const int ui_x_pos = 150; // X position for REC/Battery info (120=Center, 150=Right)

WebServer server(80);

static constexpr const size_t record_number     = 256;
static constexpr const size_t record_length     = 240;
static constexpr const size_t record_size       = record_number * record_length;
static constexpr const size_t record_samplerate = 17000;
static int16_t prev_y[record_length];
static int16_t prev_h[record_length];
static size_t rec_record_idx  = 2;
static size_t draw_record_idx = 0;
static int16_t *rec_data;

// --- SCALING FACTOR SETTINGS ---
const int scale_factors[] = {1, 2, 4, 6, 8, 12};
int scale_idx = 0; // Default to index 0 (1x)

// --- PERFORMANCE MONITORING ---
unsigned long max_loop_time = 0;
unsigned long loop_start_time = 0;

// --- WEB SERVER HANDLERS ---

void handleRoot() {
    server.send(200, "text/html", index_html);
}

void handleSpectrum() {
    server.send(200, "text/html", spectrum_html);
}

void handleGetData() {
    server.enableCORS(true); 
    auto data = &rec_data[draw_record_idx * record_length];
    
    int current_scale = scale_factors[scale_idx];
    
    String json = "{\"data\":[";
    for (int i = 0; i < record_length; i++) {
        // Apply Scaling Factor here
        // (int16_t * int) promotes to int32, so no overflow risk for these values
        json += String(data[i] * current_scale);
        if (i < record_length - 1) json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
}

void loadConfig() {
    // Try to mount SD card
    // M5Cardputer SD CS pin is typically GPIO 12
    if (!SD.begin(GPIO_NUM_12, SPI, 25000000)) { 
        M5Cardputer.Display.drawString("No SD Card found.", 120, 80);
        delay(1000);
        return; 
    }

    File file = SD.open("/config.txt");
    if (file) {
        M5Cardputer.Display.drawString("Reading config...", 120, 80);
        
        // Read line 1: SSID
        if (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim(); // Remove \r or whitespace
            if (line.length() > 0) wifi_ssid = line;
        }
        
        // Read line 2: Password
        if (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim(); 
            if (line.length() > 0) wifi_pass = line;
        }
        
        file.close();
        M5Cardputer.Display.drawString("Config Loaded!", 120, 100);
        delay(1000);
    } else {
        M5Cardputer.Display.drawString("Config file missing", 120, 80);
        delay(1000);
    }
}

void setup(void) {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setTextDatum(top_center);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setFont(&fonts::FreeSansBoldOblique12pt7b);
    
    // --- LOAD CONFIG FROM SD ---
    loadConfig();
    
    M5Cardputer.Display.clear();
    M5Cardputer.Display.drawString("Connecting to:", 120, 40);
    M5Cardputer.Display.drawString(wifi_ssid, 120, 70);
    
    // Use c_str() to convert String object to char* for begin()
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5Cardputer.Display.print(".");
    }
    
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setTextSize(0.8);
    M5Cardputer.Display.drawString("IP: " + WiFi.localIP().toString(), 120, 60);
    delay(3000); 

    // --- REGISTER ROUTES ---
    server.on("/", handleRoot);         // VU Meter
    server.on("/sv", handleSpectrum);   // Spectrum Visualizer
    server.on("/data", handleGetData);  // Data API
    
    server.begin();

    rec_data = (typeof(rec_data))heap_caps_malloc(record_size * sizeof(int16_t), MALLOC_CAP_8BIT);
    memset(rec_data, 0, record_size * sizeof(int16_t));
    M5Cardputer.Speaker.setVolume(255);
    M5Cardputer.Speaker.end();
    M5Cardputer.Mic.begin();
    
    M5Cardputer.Display.clear();
    M5Cardputer.Display.fillCircle(70, 15, 8, RED);
    
    // Show REC-ID and Initial Battery Level using global X position
    String hostId = String(WiFi.localIP()[3]);
    int bat = M5.Power.getBatteryLevel();
    M5Cardputer.Display.drawString("REC-" + hostId + " " + String(bat) + "%", ui_x_pos, 3);
}

void loop(void) {
    loop_start_time = millis(); // START TIMER

    M5Cardputer.update();
    server.handleClient();

    if (M5Cardputer.Mic.isEnabled()) {
        static constexpr int shift = 6;
        auto data = &rec_data[rec_record_idx * record_length];
        
        if (M5Cardputer.Mic.record(data, record_length, record_samplerate)) {
            data = &rec_data[draw_record_idx * record_length];

            int32_t w = M5Cardputer.Display.width();
            if (w > record_length - 1) w = record_length - 1;

            M5Cardputer.Display.startWrite();
            for (int32_t x = 0; x < w; ++x) {
                M5Cardputer.Display.writeFastVLine(x, prev_y[x], prev_h[x], TFT_BLACK);
                
                int32_t y1 = (data[x] >> shift);
                int32_t y2 = (data[x + 1] >> shift);
                if (y1 > y2) { int32_t tmp = y1; y1 = y2; y2 = tmp; }
                
                int32_t y = ((M5Cardputer.Display.height()) >> 1) + y1;
                int32_t h = ((M5Cardputer.Display.height()) >> 1) + y2 + 1 - y;
                
                prev_y[x] = y;
                prev_h[x] = h;
                M5Cardputer.Display.writeFastVLine(x, prev_y[x], prev_h[x], WHITE);
            }
            M5Cardputer.Display.endWrite();
            M5Cardputer.Display.display();
            
            // --- BATTERY UPDATE LOGIC ---
            static int bat_level = M5.Power.getBatteryLevel();
            static unsigned long last_bat_check = 0;
            if (millis() - last_bat_check > 2000) { // Check every 2 seconds
                bat_level = M5.Power.getBatteryLevel();
                last_bat_check = millis();
            }

            String hostId = String(WiFi.localIP()[3]);
            
            // --- FIX: Clear background before drawing text ---
            int box_w = 100;
            int box_h = 25;
            int box_x = ui_x_pos + 10;
            M5Cardputer.Display.fillRect(box_x, 0, box_w, box_h, BLACK);

            // Updated to use global variable ui_x_pos
            M5Cardputer.Display.drawString("REC-" + hostId + " " + String(bat_level) + "%", ui_x_pos, 3); 
            M5Cardputer.Display.fillCircle(70, 15, 8, RED);

            if (++draw_record_idx >= record_number) draw_record_idx = 0;
            if (++rec_record_idx >= record_number) rec_record_idx = 0;
        }
    }
    
    // --- KEYBOARD LOGIC ---
    if (M5Cardputer.Keyboard.isChange()) {
        if (M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
            bool scaleChanged = false;
            bool showCpu = false;
            
            for (auto i : status.word) {
                // Up Arrow (mapped to ';')
                if (i == ';') {
                    if (scale_idx < 5) {
                        scale_idx++;
                        scaleChanged = true;
                    }
                }
                // Down Arrow (mapped to '.')
                if (i == '.') {
                    if (scale_idx > 0) {
                        scale_idx--;
                        scaleChanged = true;
                    }
                }
                // 'q' Key - Show CPU Load
                if (i == 'q') {
                    showCpu = true;
                }
            }

            if (scaleChanged) {
                M5Cardputer.Display.clear();
                M5Cardputer.Display.fillCircle(70, 15, 8, YELLOW);
                M5Cardputer.Display.drawString("SF:" + String(scale_factors[scale_idx]), 120, 25);
            }
            
            if (showCpu) {
                M5Cardputer.Display.clear();
                M5Cardputer.Display.fillCircle(70, 15, 8, BLUE); // Blue for CPU/System
                
                // Calculate Load based on 40ms target (25fps)
                // If loop takes 40ms, CPU is "100% busy" for that frame rate goal
                int load_pct = (max_loop_time * 100) / 40;
                if (load_pct > 100) load_pct = 100;
                
                String debugInfo = "CPU:" + String(load_pct) + "% " + String(max_loop_time) + "ms";
                M5Cardputer.Display.drawString(debugInfo, 120, 25);
                
                max_loop_time = 0; // Reset max counter
            }
        }
    }

    // --- BUTTON LOGIC ---
    if (M5Cardputer.BtnA.wasHold()) {
        auto cfg = M5Cardputer.Mic.config();
        cfg.noise_filter_level = (cfg.noise_filter_level + 8) & 255;
        M5Cardputer.Mic.config(cfg);
        
        M5Cardputer.Display.clear();
        M5Cardputer.Display.fillCircle(70, 15, 8, GREEN);
        // NF display always centered at 120
        M5Cardputer.Display.drawString("NF:" + String(cfg.noise_filter_level), 120, 25);

    } else if (M5Cardputer.BtnA.wasClicked()) {
        if (M5Cardputer.Speaker.isEnabled()) {
            M5Cardputer.Display.clear();
            while (M5Cardputer.Mic.isRecording()) delay(1);
            
            M5Cardputer.Mic.end();
            M5Cardputer.Speaker.begin();

            M5Cardputer.Display.fillTriangle(70 - 8, 15 - 8, 70 - 8, 15 + 8, 70 + 8, 15, 0x1c9f);
            M5Cardputer.Display.drawString("PLAY", 120, 3);
            
            int start_pos = rec_record_idx * record_length;
            if (start_pos < record_size) {
                M5Cardputer.Speaker.playRaw(&rec_data[start_pos], record_size - start_pos, record_samplerate, false, 1, 0);
            }
            if (start_pos > 0) {
                M5Cardputer.Speaker.playRaw(rec_data, start_pos, record_samplerate, false, 1, 0);
            }
            do {
                delay(1);
                M5Cardputer.update();
            } while (M5Cardputer.Speaker.isPlaying());

            M5Cardputer.Speaker.end();
            M5Cardputer.Mic.begin();

            M5Cardputer.Display.clear();
            M5Cardputer.Display.fillCircle(70, 15, 8, RED);
            
            // Redraw ID and Battery after playback using global variable
            String hostId = String(WiFi.localIP()[3]);
            int bat = M5.Power.getBatteryLevel();
            M5Cardputer.Display.drawString("REC-" + hostId + " " + String(bat) + "%", ui_x_pos, 3);
        }
    }
    
    // END TIMER & UPDATE MAX
    unsigned long loop_duration = millis() - loop_start_time;
    if (loop_duration > max_loop_time) {
        max_loop_time = loop_duration;
    }
}