/**
 * @file Tab5MicTalk.ino
 * @brief M5Stack Tab5 Audio Visualizer and Server (Waveform / VU Meter / Spectrum)
 * @author [Nathan Stevens / Gemini3 Pro/ M5 Mic Example]
 * @date 2025-11-28
 *
 * HARDWARE: M5Stack Tab5
 * LIBRARY DEPENDENCY: "arduinoFFT" by Enrique Condes (Version 2.x) - Install via Library Manager
 *
 * FEATURES:
 * 1. WiFi Data Server: Serves audio data to connected web clients.
 * 2. Visualizers: 
 * - Waveform: Real-time oscilloscope style.
 * - VU Meter: Split stereo-simulation peak/rms meter.
 * - Spectrum: 64-band FFT frequency analyzer.
 * 3. Touch Interface: 5 on-screen buttons for control.
 * 4. Recording/Playback: Records to RAM and plays back via speaker (Doesn't correctly work).
 */

#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h> 
#include <arduinoFFT.h> // REQUIRED: Install "arduinoFFT" Version 2.x

// Import HTML content for the web interface (must be in sketch folder)
#include "webapp.h"   
#include "spectrum.h" 

// --- WI-FI SETTINGS (FALLBACK) ---
// These are used if 'config.txt' is not found on the SD card.
String wifi_ssid = "YOUR_SSID_HERE";
String wifi_pass = "YOUR_PASSWORD_HERE";

WebServer server(80);

// --- AUDIO CONSTANTS ---
// record_length of 256 is chosen to divide evenly into the 1280px screen width.
// 1280 pixels / 256 samples = 5 pixels per sample bar.
static constexpr const size_t record_number     = 256;
static constexpr const size_t record_length     = 256; 
static constexpr const size_t record_size       = record_number * record_length; 
static constexpr const size_t record_samplerate = 17000; // 17kHz sample rate

// --- VISUALIZER BUFFERS (MEMORY) ---
// Buffers store the "previous state" of the screen.
// This allows us to "erase" only the pixels that were drawn last frame,
// which is significantly faster than clearing the whole screen every frame.

// Waveform Buffers
static int16_t prev_y[record_length]; // Y-position of the previous frame's bar
static int16_t prev_h[record_length]; // Height of the previous frame's bar

// VU Meter Buffers
static int32_t prev_vu_w[2] = {0, 0}; // [0]=Top Bar Width, [1]=Bottom Bar Width

// Spectrum Buffers
#define FFT_SAMPLES 256 // Must be a power of 2 (matches record_length)
#define FFT_BARS 64     // Display 64 distinct frequency bands

// --- FFT OBJECT SETUP (arduinoFFT v2.x) ---
// v2.x requires passing the arrays and size to the constructor.
double vReal[FFT_SAMPLES];
double vImag[FFT_SAMPLES];
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, FFT_SAMPLES, record_samplerate);

static int16_t prev_spec_y[FFT_BARS]; // Previous Y-positions for spectrum bars

// --- AUDIO POINTERS ---
// We use a large circular buffer in PSRAM to store audio.
static size_t rec_record_idx  = 2; // Where the mic is writing to
static size_t draw_record_idx = 0; // Where the screen is reading from
static int16_t *rec_data;          // Pointer to the large buffer in PSRAM

// --- STATE VARIABLES ---
const int scale_factors[] = {1, 2, 4, 6, 8, 12}; // Vertical zoom levels
int scale_idx = 0; 

// Visualizer Mode: 0 = WAVE, 1 = VU METER, 2 = SPECTRUM
int visualMode = 0; 
const char* modeNames[] = {"WAVE", "VU METER", "SPECTRUM"};

// --- PERFORMANCE MONITORING ---
unsigned long max_loop_time = 0;   // Track longest frame time
unsigned long loop_start_time = 0; // Start of current frame

// --- LAYOUT CONSTANTS (SCREEN GEOMETRY) ---
const int LAYOUT_STATUS_H = 50;           // Height of top status bar
const int LAYOUT_VISUALIZER_TOP = 50;     // Y-coordinate where visualizer starts
const int LAYOUT_VISUALIZER_HEIGHT = 550; // Height of the visualizer area
const int LAYOUT_BUTTON_Y = 610;          // Y-coordinate for buttons

// --- VIRTUAL BUTTON CLASS ---
// A simple structure to manage touch regions and drawing.
struct VirtualButton {
    int x, y, w, h;
    String label; 
    uint16_t color;
    bool pressed;

    // Draw the button (Inverts text color if pressed)
    void draw() {
        M5.Display.fillRoundRect(x, y, w, h, 10, pressed ? WHITE : color);
        M5.Display.drawRoundRect(x, y, w, h, 10, LIGHTGREY); 
        M5.Display.setTextColor(pressed ? BLACK : WHITE);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextSize(2);
        M5.Display.drawString(label, x + (w/2), y + (h/2));
        M5.Display.setTextDatum(top_center); // Reset datum
    }

    // Check if a touch coordinate (tx, ty) is inside this button
    bool contains(int tx, int ty) {
        return (tx >= x && tx <= x + w && ty >= y && ty <= y + h); 
    }
};

VirtualButton keys[5]; // Array of 5 buttons

// --- WEB SERVER HANDLERS ---
// Serves the HTML pages stored in webapp.h and spectrum.h
void handleRoot() { server.send(200, "text/html", index_html); } 
void handleSpectrum() { server.send(200, "text/html", spectrum_html); }

// Serves raw JSON audio data to connected browsers
void handleGetData() {
    server.enableCORS(true); // Allow cross-origin requests (for testing)
    auto data = &rec_data[draw_record_idx * record_length];
    
    int current_scale = scale_factors[scale_idx];
    String json = "{\"data\":["; 
    for (int i = 0; i < record_length; i++) {
        // We apply the scaling factor server-side before sending
        json += String(data[i] * current_scale);
        if (i < record_length - 1) json += ","; 
    }
    json += "]}";
    server.send(200, "application/json", json); 
}

// --- INITIALIZATION HELPERS ---

void setupButtons() {
    int screenW = M5.Display.width();
    int btnH = 100; 
    int btnY = LAYOUT_BUTTON_Y; 
    int margin = 10;
    
    // Calculate button width dynamically to fit 5 buttons with margins
    int btnW = (screenW - (margin * 6)) / 5; 

    // Define button properties
    keys[0] = {margin, btnY, btnW, btnH, "SCALE -", 0x3186, false}; 
    keys[1] = {margin*2 + btnW, btnY, btnW, btnH, "SCALE +", 0x3186, false}; 
    keys[2] = {margin*3 + btnW*2, btnY, btnW, btnH, "MODE", 0x630C, false}; 
    keys[3] = {margin*4 + btnW*3, btnY, btnW, btnH, "NOISE FLT", 0x2424, false}; 
    keys[4] = {margin*5 + btnW*4, btnY, btnW, btnH, "PLAY", 0x14A0, false}; 

    // Draw initial state
    for(int i=0; i<5; i++) keys[i].draw(); 
}

void loadConfig() {
    // Attempt to read WiFi credentials from SD card
    SD.begin();
    File file = SD.open("/config.txt"); 
    if (file) {
        if (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim(); 
            if (line.length() > 0) wifi_ssid = line;
        }
        if (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim(); 
            if (line.length() > 0) wifi_pass = line;
        }
        file.close(); 
    }
}

// Completely black out the visualizer area and reset "previous" buffers
// This is necessary when switching modes to prevent "ghost" pixels
void clearVisualizerArea() {
    M5.Display.fillRect(0, LAYOUT_VISUALIZER_TOP, 1280, LAYOUT_VISUALIZER_HEIGHT, BLACK);
    memset(prev_y, 0, sizeof(prev_y));
    memset(prev_h, 0, sizeof(prev_h));
    prev_vu_w[0] = 0; prev_vu_w[1] = 0;
    memset(prev_spec_y, 0, sizeof(prev_spec_y));
}

// --- VISUALIZATION ENGINES ---

// 1. WAVEFORM RENDERER
// Draws the raw audio signal as a vertical bar graph.
void drawWaveform(int16_t *data) {
    int32_t midY = (LAYOUT_VISUALIZER_TOP + (LAYOUT_VISUALIZER_HEIGHT / 2));
    int32_t bar_width = 1280 / record_length; // Should be exactly 5px
    int shift = 6; // Bit-shift division for scaling raw audio
    int maxY = LAYOUT_VISUALIZER_TOP + LAYOUT_VISUALIZER_HEIGHT;

    for (int32_t i = 0; i < record_length; ++i) {
        int32_t x_pos = i * bar_width;
        
        // 1. Erase the previous frame's bar at this position (Dirty Rect)
        // This is much faster than clearing the whole screen.
        M5.Display.writeFastVLine(x_pos, prev_y[i], prev_h[i], BLACK); 
        if(bar_width > 1) M5.Display.fillRect(x_pos, prev_y[i], bar_width, prev_h[i], BLACK); 

        // 2. Calculate new bar height based on audio data
        int32_t y1 = (data[i] >> shift) * scale_factors[scale_idx];
        int32_t y2;
        
        // Use next sample to connect lines, or same sample if at end
        if (i < record_length - 1) {
             y2 = (data[i + 1] >> shift) * scale_factors[scale_idx]; 
        } else {
             y2 = y1;
        }

        // Sort y1/y2 so y1 is always top, y2 is bottom (for height calc)
        if (y1 > y2) { int32_t tmp = y1; y1 = y2; y2 = tmp; } 
        
        int32_t y = midY + y1;
        int32_t h = midY + y2 + 1 - y; 

        // 3. Clamp values to stay inside Visualizer Area
        if (y < LAYOUT_VISUALIZER_TOP) y = LAYOUT_VISUALIZER_TOP; 
        if ((y + h) > maxY) h = maxY - y;
        if (h < 0) h = 0; 

        // 4. Store current state for next frame's erasure
        prev_y[i] = y;
        prev_h[i] = h; 
        
        // 5. Draw the new bar (White)
        if (bar_width > 1) M5.Display.fillRect(x_pos, y, bar_width, h, WHITE);
        else M5.Display.writeFastVLine(x_pos, y, h, WHITE); 
    }
}

// 2. VU METER RENDERER
// Draws two horizontal bars. Top = Peak Volume, Bottom = Average Volume.
void drawVUMeter(int16_t *data) {
    long sum = 0;
    int peak = 0;
    
    // Calculate RMS-ish Average and Absolute Peak
    for(int i=0; i<record_length; i++) {
        int val = abs(data[i]);
        if(val > peak) peak = val;
        sum += val;
    }
    int avg = sum / record_length;

    // Scale values to screen width (1280px)
    int w_top = (peak * scale_factors[scale_idx]) / 2; 
    int w_bot = (avg * scale_factors[scale_idx]) * 2; 

    // Clamp to screen width
    if (w_top > 1280) w_top = 1280;
    if (w_bot > 1280) w_bot = 1280;

    int midY = LAYOUT_VISUALIZER_TOP + (LAYOUT_VISUALIZER_HEIGHT / 2);
    int barHeight = 100;
    int gap = 20;

    // Draw Top Bar (Optimized: Only draw the difference)
    int y1 = midY - gap - barHeight;
    if (w_top > prev_vu_w[0]) {
        // Growing: Draw White extension
        M5.Display.fillRect(prev_vu_w[0], y1, w_top - prev_vu_w[0], barHeight, WHITE);
    } else if (w_top < prev_vu_w[0]) {
        // Shrinking: Draw Black erasure
        M5.Display.fillRect(w_top, y1, prev_vu_w[0] - w_top, barHeight, BLACK);
    }
    prev_vu_w[0] = w_top; // Save state

    // Draw Bottom Bar (Optimized)
    int y2 = midY + gap;
    if (w_bot > prev_vu_w[1]) {
        M5.Display.fillRect(prev_vu_w[1], y2, w_bot - prev_vu_w[1], barHeight, WHITE);
    } else if (w_bot < prev_vu_w[1]) {
        M5.Display.fillRect(w_bot, y2, prev_vu_w[1] - w_bot, barHeight, BLACK);
    }
    prev_vu_w[1] = w_bot; // Save state

    // Labels
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(LIGHTGREY);
    M5.Display.setCursor(20, y1 + 35);
    M5.Display.print("PEAK");
    M5.Display.setCursor(20, y2 + 35);
    M5.Display.print("AVG");
    M5.Display.setTextColor(WHITE);
}

// 3. SPECTRUM RENDERER
// Uses FFT to break audio into frequencies and display 64 bars.
void drawSpectrum(int16_t *data) {
    // 1. Prepare FFT Data (Copy raw audio to real array)
    for (int i = 0; i < FFT_SAMPLES; i++) {
        vReal[i] = (double)data[i];
        vImag[i] = 0; // Imaginary part is 0 for raw audio
    }

    // 2. Perform FFT (arduinoFFT v2.x syntax)
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward); // Apply windowing to reduce leakage
    FFT.compute(FFTDirection::Forward);                       // Compute FFT
    FFT.complexToMagnitude();                                 // Convert to usable magnitude

    // 3. Render Bars
    int barWidth = 1280 / FFT_BARS; // 20px per bar
    int bottomY = LAYOUT_VISUALIZER_TOP + LAYOUT_VISUALIZER_HEIGHT;
    
    // Loop through the 64 display bars
    for (int i = 0; i < FFT_BARS; i++) {
        // We combine 2 bins per bar (128 usable bins -> 64 bars)
        // (vReal[i*2] + vReal[i*2+1]) / 2.0 is a simple average
        double val = (vReal[i*2] + vReal[i*2+1]) / 2.0; 
        
        // Scale height
        int h = (int)(val * scale_factors[scale_idx] * 0.05); 
        if (h > LAYOUT_VISUALIZER_HEIGHT) h = LAYOUT_VISUALIZER_HEIGHT;

        int x = i * barWidth;
        int y = bottomY - h; // Top Y position of the bar

        // Optimized Drawing (Dirty Pixel)
        int prev_y = prev_spec_y[i];
        
        if (y < prev_y) {
            // Bar got taller: Draw White from new Top down to old Top
            M5.Display.fillRect(x, y, barWidth - 2, prev_y - y, WHITE);
        } else if (y > prev_y) {
            // Bar got shorter: Draw Black from old Top down to new Top
            M5.Display.fillRect(x, prev_y, barWidth - 2, y - prev_y, BLACK);
        }
        
        prev_spec_y[i] = y; // Save state
    }
}

// --- MAIN SETUP ---
void setup(void) {
    auto cfg = M5.config();
    M5.begin(cfg);
    
    // Tab5 needs Rotation 3 to be "right side up" in this orientation
    M5.Display.setRotation(3); 
    M5.Display.setTextSize(3); 
    M5.Display.setTextColor(WHITE);
    
    // Boot Screen
    M5.Display.fillScreen(BLACK);
    M5.Display.drawString("BOOTING MICTALK SYSTEM...", 640, 300); 

    loadConfig(); // Load WiFi settings from SD

    // Connect to WiFi
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    int dots = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5.Display.print("."); 
        if(++dots > 20) break; // Timeout after 10s
    }
    
    // Prepare User Interface
    M5.Display.fillScreen(BLACK);
    setupButtons();

    // Start Web Server
    server.on("/", handleRoot);
    server.on("/sv", handleSpectrum);
    server.on("/data", handleGetData);
    server.begin();
    
    // Allocate Audio Buffer in PSRAM (Heap Caps Malloc)
    // We use PSRAM because the buffer is large
    rec_data = (typeof(rec_data))heap_caps_malloc(record_size * sizeof(int16_t), MALLOC_CAP_8BIT); 
    memset(rec_data, 0, record_size * sizeof(int16_t)); 
    
    // Initialize Spectrum previous state to bottom of screen
    for(int i=0; i<FFT_BARS; i++) prev_spec_y[i] = LAYOUT_VISUALIZER_TOP + LAYOUT_VISUALIZER_HEIGHT;

    // Start Hardware Audio
    M5.Speaker.setVolume(255);
    M5.Mic.begin();
}

// --- PLAYBACK ROUTINE ---
// Stops recording, plays the buffer, then resumes recording.
void playRecording() {
    if (!M5.Speaker.isEnabled()) return;
    
    // UI Feedback: Red "Playing" Toast
    M5.Display.fillRect(400, 250, 480, 100, RED); 
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString("PLAYING AUDIO...", 640, 300); 
    M5.Display.setTextDatum(top_center);

    // Stop Mic to free up resources
    while (M5.Mic.isRecording()) delay(1);
    M5.Mic.end();
    
    M5.Speaker.begin();
    
    // Handle Buffer Wrap-around playback
    int start_pos = rec_record_idx * record_length; 
    if (start_pos < record_size) {
        // Play from current index to end of buffer
        M5.Speaker.playRaw(&rec_data[start_pos], record_size - start_pos, record_samplerate, false, 1, 0); 
    }
    if (start_pos > 0) {
        // Play from start of buffer to current index
        M5.Speaker.playRaw(rec_data, start_pos, record_samplerate, false, 1, 0); 
    }
    
    // Wait until audio finishes
    do { delay(1); M5.update(); } while (M5.Speaker.isPlaying()); 
    
    // Restore Mic
    M5.Speaker.end();
    M5.Mic.begin();
    
    // Clear UI Feedback
    M5.Display.fillRect(400, 250, 480, 100, BLACK); 
    
    // Force clear visualizer to remove any artifacts caused by the pause
    clearVisualizerArea();
}

// --- MAIN LOOP ---
void loop(void) {
    // Track loop timing
    loop_start_time = millis();
    
    // Update hardware buttons/touch
    M5.update();  
    
    // Process incoming web requests              
    server.handleClient();
    
    // --- 1. TOUCH INTERFACE LOGIC ---
    if (M5.Touch.getCount() > 0) {
        auto t = M5.Touch.getDetail(0);
        // Handle Button Press
        if (t.wasPressed()) {
            for(int i=0; i<5; i++) {
                if(keys[i].contains(t.x, t.y)) {
                    keys[i].pressed = true;
                    keys[i].draw(); // Redraw button pressed

                    // --- BUTTON ACTIONS ---
                    if (i == 0 && scale_idx > 0) scale_idx--;
                    // Scale Down
                    else if (i == 1 && scale_idx < 5) scale_idx++;
                    // Scale Up
                    else if (i == 2) { 
                        // MODE CYCLE: Wave -> VU -> Spectrum -> Wave
                        visualMode++;
                        if (visualMode > 2) visualMode = 0; 
                        
                        // Show visual feedback toast
                        M5.Display.fillRect(400, 300, 480, 60, BLUE);
                        M5.Display.drawString("MODE: " + String(modeNames[visualMode]), 640, 320); // [cite: 122]
                        delay(200);
                        M5.Display.fillRect(400, 300, 480, 60, BLACK);
                        
                        // Wipe screen for new mode [cite: 123]
                        clearVisualizerArea();
                    }
                    else if (i == 3) { 
                        // NOISE FILTER
                        auto micCfg = M5.Mic.config();
                        micCfg.noise_filter_level = (micCfg.noise_filter_level + 8) & 255;
                        M5.Mic.config(micCfg);
                        
                        M5.Display.fillRect(400, 300, 480, 60, BLUE); 
                        M5.Display.drawString("NF LEVEL: " + String(micCfg.noise_filter_level), 640, 320);
                        
                        delay(200);
                        
                        M5.Display.fillRect(400, 300, 480, 60, BLACK);
                    }
                    else if (i == 4) playRecording();
                }
            }
        }
        
        // Handle Button Release
        if (t.wasReleased()) {
             for(int i=0; i<5; i++) {
                 
                 if(keys[i].pressed) { 
                     keys[i].pressed = false;
                     keys[i].draw(); // Redraw button released
                 }
             }
        }
    }

    // --- 2. AUDIO PROCESSING & VISUALIZATION ---
    if (M5.Mic.isEnabled()) {
        auto data = &rec_data[rec_record_idx * record_length];
        // Attempt to record a chunk of audio
        if (M5.Mic.record(data, record_length, record_samplerate)) {
            // If successful, data is now updated.
            // Set draw pointer to current.
            data = &rec_data[draw_record_idx * record_length];
            // HARDWARE CLIPPING:
            // Crucial!
            // This tells the display driver to IGNORE any drawing attempts
            // outside the visualizer box.
            // This prevents the waveform from
            // accidentally drawing over the status bar or buttons.
            M5.Display.setClipRect(0, LAYOUT_VISUALIZER_TOP, 1280, LAYOUT_VISUALIZER_HEIGHT);
            M5.Display.startWrite(); 

            // Execute the selected visualizer
            switch (visualMode)
            {
            case 0: drawWaveform(data);
                break;
            case 1: drawVUMeter(data); 
                break;
            case 2: drawSpectrum(data); 
                break;
            }

            M5.Display.endWrite();
            M5.Display.clearClipRect(); // Disable clipping

            // Advance buffer pointers (Circular Buffer Logic)
            if (++draw_record_idx >= record_number) draw_record_idx = 0;
            if (++rec_record_idx >= record_number) rec_record_idx = 0;
        }
    }
    
    // --- 3. STATUS BAR UPDATE (Top of Screen) ---
    // We only update this every 500ms to save CPU and reduce flicker
    static unsigned long last_stat = 0;
    if(millis() - last_stat > 500) {  
         int bat = M5.Power.getBatteryLevel();
         String ip = WiFi.localIP().toString(); 
         
         // Calculate approx CPU load based on frame budget (40ms target)
         int load = (max_loop_time * 100) / 40;
         M5.Display.setTextSize(3);
         // Clear only the top status area
         M5.Display.fillRect(0,0, 1280, LAYOUT_STATUS_H, 0x18E3);
         M5.Display.setCursor(15, 12); 
         
         // Print Status String (UPDATED WITH SCALE FACTOR)
         M5.Display.print("IP: " + ip + 
                          " | BAT: " + String(bat) + "%" +
                          " | " + String(modeNames[visualMode]) + 
                          " | SCL: " + String(scale_factors[scale_idx]) + "x" + // ADDED SCALE HERE
                          " | CPU: " + String(load) + "% (" + String(max_loop_time) + "ms)");
                          
         last_stat = millis(); 
         max_loop_time = 0; // Reset max counter for next period
    }
    
    // Update Max Loop Time
    unsigned long loop_duration = millis() - loop_start_time;
    if (loop_duration > max_loop_time) max_loop_time = loop_duration;
}