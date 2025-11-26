/**
 * @file Tab5MicTalk.ino
 * @brief M5Stack Tab5 Audio Visualizer And Data Server
 *
 * * WHAT THIS SKETCH DOES:
 * 1. Records audio from the built-in microphones.
 * 2. Displays a real-time audio waveform on the 1280x720 screen.
 * 3. Connects to WiFi (SD Card config first, then Hardcoded fallback).
 * 4. Serves Web Apps (Root/Spectrum/Data) just like the Cardputer version.
 * 5. TOUCH SCREEN CONTROL:
 * - SCALE -/+: Adjusts waveform height.
 * - INFO: Toggles CPU Load & Loop Time display.
 * - NOISE FLT: Adjusts microphone noise gate.
 * - PLAY: Replays the last ~3 seconds of audio.
 *
 */

#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h> 

// Ensure webapp.h and spectrum.h are in your sketch folder
#include "webapp.h"   
#include "spectrum.h" 

// --- WI-FI SETTINGS (FALLBACK) ---
// Used if config.txt is missing from SD Card
String wifi_ssid = "YOUR_SSID_HERE";
String wifi_pass = "YOUR_PASSWORD_HERE";

WebServer server(80);

// --- AUDIO CONSTANTS ---
// We keep record_length at 240 to maintain compatibility with the Web App logic
// The Tab5 stretches this 240-point buffer across 1280 pixels
static constexpr const size_t record_number     = 256;
static constexpr const size_t record_length     = 240; 
static constexpr const size_t record_size       = record_number * record_length;
static constexpr const size_t record_samplerate = 17000;

// Visual buffers to store previous frame state for efficient erasing
static int16_t prev_y[record_length];
static int16_t prev_h[record_length];
static size_t rec_record_idx  = 2;
static size_t draw_record_idx = 0;
static int16_t *rec_data; // Pointer to the large audio buffer in PSRAM

// --- STATE VARIABLES ---
const int scale_factors[] = {1, 2, 4, 6, 8, 12};
int scale_idx = 0; 

// --- PERFORMANCE MONITORING ---
unsigned long max_loop_time = 0;
unsigned long loop_start_time = 0;
bool showCpu = false;

// --- LAYOUT CONSTANTS ---
// Defines the three distinct zones of the Tab5 screen (1280 x 720)
const int LAYOUT_STATUS_H = 50;           // Top bar (IP, Battery)
const int LAYOUT_VISUALIZER_TOP = 50;     // Start of waveform area
const int LAYOUT_VISUALIZER_HEIGHT = 550; // Height of waveform (Ends at y=600)
const int LAYOUT_BUTTON_Y = 610;          // Start of Virtual Deck (Bottom)

// --- VIRTUAL BUTTON CLASS ---
// Handles hit detection and drawing for the touch interface
struct VirtualButton {
    int x, y, w, h;
    String label;
    uint16_t color;
    bool pressed;

    // Draws the button (Inverted colors if pressed)
    void draw() {
        M5.Display.fillRoundRect(x, y, w, h, 10, pressed ? WHITE : color);
        M5.Display.drawRoundRect(x, y, w, h, 10, LIGHTGREY);
        M5.Display.setTextColor(pressed ? BLACK : WHITE);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextSize(2);
        M5.Display.drawString(label, x + (w/2), y + (h/2));
        M5.Display.setTextDatum(top_center); // Reset datum
    }

    // Hit detection: Returns true if touch coordinates are inside this button
    bool contains(int tx, int ty) {
        return (tx >= x && tx <= x + w && ty >= y && ty <= y + h);
    }
};

// Array of 5 buttons for the "Deck"
VirtualButton keys[5];

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
    
    // Apply current scaling factor to the JSON data sent to browser
    int current_scale = scale_factors[scale_idx];
    
    String json = "{\"data\":[";
    for (int i = 0; i < record_length; i++) {
        json += String(data[i] * current_scale);
        if (i < record_length - 1) json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
}

// --- SETUP HELPERS ---

void setupButtons() {
    // Dynamically calculate button width based on screen size
    int screenW = M5.Display.width();
    int btnH = 100; 
    int btnY = LAYOUT_BUTTON_Y; 
    int margin = 10;
    int btnW = (screenW - (margin * 6)) / 5; // 5 buttons with margins

    // Initialize Button Definitions
    keys[0] = {margin, btnY, btnW, btnH, "SCALE -", 0x3186, false};      
    keys[1] = {margin*2 + btnW, btnY, btnW, btnH, "SCALE +", 0x3186, false}; 
    keys[2] = {margin*3 + btnW*2, btnY, btnW, btnH, "INFO", 0x630C, false}; 
    keys[3] = {margin*4 + btnW*3, btnY, btnW, btnH, "NOISE FLT", 0x2424, false}; 
    keys[4] = {margin*5 + btnW*4, btnY, btnW, btnH, "PLAY", 0x14A0, false};   
    
    // Draw initial state
    for(int i=0; i<5; i++) keys[i].draw();
}

void loadConfig() {
    // Try to load wifi.txt or config.txt from SD
    // Note: Tab5 SD is handled automatically by M5Unified if inserted
    SD.begin(); 
    File file = SD.open("/config.txt");
    if (file) {
        // Line 1: SSID
        if (file.available()) {
            String line = file.readStringUntil('\n'); line.trim();
            if (line.length() > 0) wifi_ssid = line;
        }
        // Line 2: Password
        if (file.available()) {
            String line = file.readStringUntil('\n'); line.trim();
            if (line.length() > 0) wifi_pass = line;
        }
        file.close();
    }
}

// --- MAIN SETUP ---

void setup(void) {
    auto cfg = M5.config();
    M5.begin(cfg);

    // --- DISPLAY CONFIGURATION ---
    // Rotation 3 is required for the Tab5 to be "right side up" in this orientation
    M5.Display.setRotation(3); 
    M5.Display.setTextSize(3); // Large text for high-DPI screen
    M5.Display.setTextColor(WHITE);
    
    M5.Display.fillScreen(BLACK);
    M5.Display.drawString("BOOTING MICTALK SYSTEM...", 640, 300);
    
    // --- LOAD CONFIG & WIFI ---
    loadConfig();

    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    int dots = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5.Display.print(".");
        // Timeout after 10 seconds (20 dots)
        if(++dots > 20) break; 
    }
    
    // Clear boot screen and draw the Interface
    M5.Display.fillScreen(BLACK);
    setupButtons();

    // --- SERVER & MEMORY ---
    server.on("/", handleRoot);
    server.on("/sv", handleSpectrum);
    server.on("/data", handleGetData);
    server.begin();

    // Allocate audio buffer in PSRAM (MALLOC_CAP_8BIT)
    rec_data = (typeof(rec_data))heap_caps_malloc(record_size * sizeof(int16_t), MALLOC_CAP_8BIT);
    memset(rec_data, 0, record_size * sizeof(int16_t));

    // --- AUDIO INIT ---
    M5.Speaker.setVolume(255);
    M5.Mic.begin();
}

// --- PLAYBACK ROUTINE ---
void playRecording() {
    if (!M5.Speaker.isEnabled()) return;
    
    // Draw "Playing" toast in the center (Safe Zone)
    M5.Display.fillRect(400, 250, 480, 100, RED);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString("PLAYING AUDIO...", 640, 300);
    M5.Display.setTextDatum(top_center);

    // Stop recording before playback
    while (M5.Mic.isRecording()) delay(1);
    M5.Mic.end();
    M5.Speaker.begin();
    
    // Play the circular buffer linearly
    int start_pos = rec_record_idx * record_length;
    if (start_pos < record_size) {
        M5.Speaker.playRaw(&rec_data[start_pos], record_size - start_pos, record_samplerate, false, 1, 0);
    }
    if (start_pos > 0) {
        M5.Speaker.playRaw(rec_data, start_pos, record_samplerate, false, 1, 0);
    }
    
    // Wait for finish
    do { delay(1); M5.update(); } while (M5.Speaker.isPlaying());
    
    // Restore Mic
    M5.Speaker.end();
    M5.Mic.begin();
    
    // Clear toast
    M5.Display.fillRect(400, 250, 480, 100, BLACK);
}

// --- MAIN LOOP ---

void loop(void) {
    loop_start_time = millis(); // START TIMER
    M5.update();
    server.handleClient();

    // --- 1. TOUCH INTERFACE ENGINE ---
    if (M5.Touch.getCount() > 0) {
        auto t = M5.Touch.getDetail(0);
        
        // Handle PRESS Events
        if (t.wasPressed()) {
            for(int i=0; i<5; i++) {
                if(keys[i].contains(t.x, t.y)) {
                    keys[i].pressed = true;
                    keys[i].draw(); // Redraw as pressed

                    // --- BUTTON LOGIC ---
                    if (i == 0 && scale_idx > 0) scale_idx--; // Scale -
                    else if (i == 1 && scale_idx < 5) scale_idx++; // Scale +
                    else if (i == 2) { // INFO Button
                        showCpu = !showCpu;
                        // Clear the top bar if turning off info
                        if(!showCpu) M5.Display.fillRect(0, 0, 1280, LAYOUT_STATUS_H, BLACK); 
                    }
                    else if (i == 3) { // NOISE FLT
                        auto micCfg = M5.Mic.config();
                        // Cycle noise filter (approx steps of 8)
                        micCfg.noise_filter_level = (micCfg.noise_filter_level + 8) & 255;
                        M5.Mic.config(micCfg);
                        
                        // Show temporary feedback toast
                        M5.Display.fillRect(400, 300, 480, 60, BLUE);
                        M5.Display.drawString("NF LEVEL: " + String(micCfg.noise_filter_level), 640, 320);
                        delay(200);
                        M5.Display.fillRect(400, 300, 480, 60, BLACK);
                    }
                    else if (i == 4) playRecording(); // PLAY
                }
            }
        }
        
        // Handle RELEASE Events
        if (t.wasReleased()) {
             for(int i=0; i<5; i++) {
                 if(keys[i].pressed) {
                     keys[i].pressed = false;
                     keys[i].draw(); // Redraw as released
                 }
             }
        }
    }

    // --- 2. AUDIO VISUALIZER ENGINE ---
    if (M5.Mic.isEnabled()) {
        static constexpr int shift = 6;
        auto data = &rec_data[rec_record_idx * record_length];
        
        if (M5.Mic.record(data, record_length, record_samplerate)) {
            data = &rec_data[draw_record_idx * record_length];
            
            // --- CRITICAL: HARDWARE CLIPPING ---
            // Force the display driver to IGNORE any drawing outside the visualizer area.
            // This guarantees the waveform cannot overwrite the Status Bar or Buttons.
            M5.Display.setClipRect(0, LAYOUT_VISUALIZER_TOP, 1280, LAYOUT_VISUALIZER_HEIGHT);

            int32_t midY = (LAYOUT_VISUALIZER_TOP + (LAYOUT_VISUALIZER_HEIGHT / 2));
            int32_t bar_width = 1280 / record_length; 
            
            M5.Display.startWrite();
            for (int32_t i = 0; i < record_length; ++i) {
                int32_t x_pos = i * bar_width;

                // Erase previous frame (Optimized)
                M5.Display.writeFastVLine(x_pos, prev_y[i], prev_h[i], BLACK);
                if(bar_width > 1) M5.Display.fillRect(x_pos, prev_y[i], bar_width, prev_h[i], BLACK);

                // Scale Audio Data
                int32_t y1 = (data[i] >> shift) * scale_factors[scale_idx];
                int32_t y2 = (data[i + 1] >> shift) * scale_factors[scale_idx];
                if (y1 > y2) { int32_t tmp = y1; y1 = y2; y2 = tmp; }
                
                // Calculate Height and Position
                int32_t y = midY + y1;
                int32_t h = midY + y2 + 1 - y;
                
                // --- SOFTWARE SAFETY CLAMP ---
                // Ensure coordinates remain sane even before hardware clipping
                int maxY = LAYOUT_VISUALIZER_TOP + LAYOUT_VISUALIZER_HEIGHT;
                
                if (y < LAYOUT_VISUALIZER_TOP) y = LAYOUT_VISUALIZER_TOP;
                if ((y + h) > maxY) h = maxY - y;
                if (h < 0) h = 0;

                // Store state for next erase cycle
                prev_y[i] = y;
                prev_h[i] = h;
                
                // Draw new frame
                if (bar_width > 1) {
                     M5.Display.fillRect(x_pos, y, bar_width, h, WHITE);
                } else {
                     M5.Display.writeFastVLine(x_pos, y, h, WHITE);
                }
            }
            M5.Display.endWrite();
            
            // Disable Clipping so we can draw other things again
            M5.Display.clearClipRect();

            // Advance Buffer Pointers
            if (++draw_record_idx >= record_number) draw_record_idx = 0;
            if (++rec_record_idx >= record_number) rec_record_idx = 0;
        }
    }
    
    // --- 3. STATUS BAR UPDATE ---
    // Update every 500ms to avoid flickering and save CPU
    static unsigned long last_stat = 0;
    if(millis() - last_stat > 500) {
         int bat = M5.Power.getBatteryLevel();
         String ip = WiFi.localIP().toString();
         
         M5.Display.setTextSize(3); 
         // Clear only the top bar area
         M5.Display.fillRect(0,0, 1280, LAYOUT_STATUS_H, 0x18E3); // Dark Blue BG
         M5.Display.setCursor(15, 12); 
         M5.Display.print("IP: " + ip + "  |  BAT: " + String(bat) + "%");
         
         // If Info enabled, show CPU Load and Loop Time
         if(showCpu) {
             int load = (max_loop_time * 100) / 40; // Approx load based on 25FPS target
             M5.Display.print(" | CPU: " + String(load) + "% (" + String(max_loop_time) + "ms) | SF: " + String(scale_factors[scale_idx]) + "x");
             max_loop_time = 0; // Reset max counter
         }
         last_stat = millis();
    }
    
    // END TIMER & UPDATE MAX
    unsigned long loop_duration = millis() - loop_start_time;
    if (loop_duration > max_loop_time) max_loop_time = loop_duration;
}