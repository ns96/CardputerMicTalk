const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>M5 Audio Workstation</title>
    <style>
        :root {
            /* --- SHARED THEME VARS --- */
            --bg-color: #111;
            --panel-bg: #1a1a1a;
            --text-color: #eee;
            --accent-color: #333;
            --border-color: #444;
            --font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;

            /* SPECTRUM SPECIFIC */
            --grid-color: #333;
            --bar-top: #ff3333;
            --bar-mid: #ffff00;
            --bar-bot: #00ff00;
            --peak-color: #ffffff;

            /* VU METER SPECIFIC */
            --meter-face-1: #2a2a2a;
            --meter-face-2: #111;
            --meter-bezel: #444;
            --meter-text: #ddd;
            --meter-needle: #ff3333;
            --meter-shadow: rgba(0,0,0,0.5);
            --led-off: #222;
        }

        /* LIGHT THEME */
        [data-theme="light"] {
            --bg-color: #f4f4f4;
            --panel-bg: #ffffff;
            --text-color: #333;
            --accent-color: #ddd;
            --border-color: #ccc;
            --grid-color: #e0e0e0;
            --bar-top: #d32f2f;
            --bar-mid: #fbc02d;
            --bar-bot: #388e3c;
            --peak-color: #000;
            
            --meter-face-1: #fffbf0; 
            --meter-face-2: #f0e6d2;
            --meter-bezel: #aaa;
            --meter-text: #333;
            --meter-needle: #cc0000;
            --led-off: #e0e0e0;
        }

        /* BLUE THEME */
        [data-theme="blue"] {
            --bg-color: #001e3c;
            --panel-bg: #051e30;
            --text-color: #90caf9;
            --accent-color: #1e4976;
            --border-color: #1769aa;
            --grid-color: #132f4c;
            --bar-top: #00e5ff;
            --bar-mid: #2979ff;
            --bar-bot: #1565c0;
            --peak-color: #00e5ff;

            --meter-face-1: #052030;
            --meter-face-2: #021019;
            --meter-bezel: #0d47a1;
            --meter-text: #00e5ff;
            --meter-needle: #00e5ff;
            --led-off: #062233;
        }

        /* GREEN THEME */
        [data-theme="green"] {
            --bg-color: #001000;
            --panel-bg: #0c260c;
            --text-color: #00ff00;
            --accent-color: #0a330a;
            --border-color: #1b5e20;
            --grid-color: #0f330f;
            --bar-top: #ccff90;
            --bar-mid: #69f0ae;
            --bar-bot: #00c853;
            --peak-color: #00ff00;

            --meter-face-1: #0c260c;
            --meter-face-2: #000;
            --meter-bezel: #33691e;
            --meter-text: #66bb6a;
            --meter-needle: #00ff00;
            --led-off: #122b12;
        }

        * { box-sizing: border-box; }

        body {
            background-color: var(--bg-color);
            color: var(--text-color);
            font-family: var(--font-family);
            margin: 0;
            display: flex;
            flex-direction: column;
            height: 100vh;
            width: 100vw;
            overflow: hidden;
            transition: all 0.3s ease;
        }

        /* TOOLBAR */
        .toolbar {
            background-color: var(--panel-bg);
            padding: 10px 15px;
            border-bottom: 1px solid var(--border-color);
            display: flex;
            gap: 15px;
            flex-wrap: wrap;
            align-items: center;
            justify-content: center;
            box-shadow: 0 4px 15px rgba(0,0,0,0.2);
            z-index: 100;
            flex-shrink: 0; 
        }

        .control-group {
            display: flex;
            align-items: center;
            gap: 8px;
        }

        /* Hide controls not relevant to current mode */
        .hidden-ctrl { display: none !important; }

        label {
            font-size: 0.7rem;
            font-weight: 700;
            text-transform: uppercase;
            opacity: 0.7;
        }

        select, input[type="text"] {
            background: var(--bg-color);
            color: var(--text-color);
            border: 1px solid var(--border-color);
            padding: 5px 8px;
            border-radius: 4px;
            font-family: inherit;
            font-size: 0.85rem;
            outline: none;
        }

        /* System Select Style (Highlight) */
        #appMode {
            background-color: var(--accent-color);
            border: 1px solid var(--text-color);
            font-weight: bold;
        }

        button {
            background: var(--accent-color);
            color: var(--text-color);
            border: 1px solid var(--border-color);
            padding: 5px 15px;
            border-radius: 4px;
            cursor: pointer;
            font-weight: bold;
            font-size: 0.8rem;
            transition: 0.2s;
        }
        button:hover { filter: brightness(1.2); }

        .status-light {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background-color: #444;
            margin-left: 5px;
            box-shadow: inset 0 1px 2px rgba(0,0,0,0.5);
            transition: all 0.3s;
        }
        .status-light.connected { background-color: #00e676; box-shadow: 0 0 8px #00e676; }
        .status-light.error { background-color: #ff1744; box-shadow: 0 0 8px #ff1744; }

        /* STAGE */
        .stage {
            flex-grow: 1;
            position: relative;
            width: 100%;
            height: 0; 
            background: radial-gradient(circle at center, var(--panel-bg), var(--bg-color));
        }

        canvas {
            display: block;
            width: 100%;
            height: 100%;
        }

        .overlay {
            position: absolute;
            top: 5px;
            right: 10px;
            font-family: monospace;
            font-size: 0.7rem;
            opacity: 0.4;
            pointer-events: none;
        }
    </style>
</head>
<body>

    <div class="toolbar">
        
        <div class="control-group">
            <label>System</label>
            <select id="appMode" onchange="switchMode()">
                <option value="spectrum">Spectrum Analyzer</option>
                <option value="vu" selected>VU Meter</option>
            </select>
        </div>

        <div class="control-group">
            <label>IP</label>
            <input type="text" id="ipAddress" placeholder="192.168.1.X" style="width: 110px;">
            <button onclick="toggleConnection()" id="connectBtn">LINK</button>
            <div class="status-light" id="statusIndicator"></div>
        </div>

        <div class="control-group hidden-ctrl" id="grp-spec-mode">
            <label>Viz</label>
            <select id="vizMode">
                <option value="bars">Bars</option>
                <option value="line">Line</option>
                <option value="mirror">Mirror</option>
            </select>
        </div>

        <div class="control-group" id="grp-vu-mode">
            <label>Meter</label>
            <select id="meterStyle">
                <option value="analog">Analog Pro</option>
                <option value="led">Digital LED</option>
            </select>
        </div>

        <div class="control-group" id="grp-vu-sim">
            <label title="Simulate Stereo separation">Sim</label>
            <input type="checkbox" id="stereoSim">
        </div>

        <div class="control-group">
            <label>Gain</label>
            <select id="gain">
                <option value="0.5">0.5x</option>
                <option value="1.0">1x</option>
                <option value="2.0">2x</option>
                <option value="4.0" selected>4x</option>
                <option value="8.0">8x</option>
                <option value="12.0">12x</option>
            </select>
        </div>

        <div class="control-group">
            <label>Decay</label>
            <select id="decay">
                <option value="slow">Slow</option>
                <option value="normal" selected>Normal</option>
                <option value="fast">Fast</option>
            </select>
        </div>

        <div class="control-group">
            <label>Theme</label>
            <select id="themeSelector">
                <option value="dark">Dark</option>
                <option value="light">Light</option>
                <option value="blue">Blue</option>
                <option value="green">Green</option>
            </select>
        </div>
    </div>

    <div class="stage" id="stage">
        <canvas id="mainCanvas"></canvas>
        <div class="overlay"><span id="modeLabel">VU METER</span> // <span id="fps">0</span> FPS</div>
    </div>

    <script>
        // --- GLOBAL CONFIG ---
        const canvas = document.getElementById('mainCanvas');
        const ctx = canvas.getContext('2d', { alpha: false });
        const ipInput = document.getElementById('ipAddress');
        const connectBtn = document.getElementById('connectBtn');
        const statusLight = document.getElementById('statusIndicator');
        const modeSelector = document.getElementById('appMode');
        
        let currentMode = 'vu'; // SET DEFAULT TO VU

        // --- STATE MANAGERS ---
        // Spectrum State
        const numBars = 64;
        const BASE_SENSITIVITY_SPEC = 2000;
        let barValues = new Array(numBars).fill(0);
        let peakValues = new Array(numBars).fill(0);
        let peakHoldTime = new Array(numBars).fill(0);

        // VU State
        const BASE_SENSITIVITY_VU = 18000;
        let volL = 0, volR = 0;
        let targetVolL = 0, targetVolR = 0;

        // Engine State
        let isConnected = false;
        let pollInterval = null;
        let animFrame = null;
        let lastTime = 0;
        let frameCount = 0;
        let lastFpsTime = 0;

        // --- INIT ---
        const isLocal = window.location.protocol === 'file:';
        if (!isLocal) {
            ipInput.value = window.location.hostname;
            window.onload = connect;
        } else {
            ipInput.value = "192.168.1.57";
        }

        // --- EVENTS ---
        document.getElementById('themeSelector').addEventListener('change', (e) => {
            document.documentElement.setAttribute('data-theme', e.target.value);
        });
        window.addEventListener('resize', resizeCanvas);

        function resizeCanvas() {
            const stage = document.getElementById('stage');
            const dpr = window.devicePixelRatio || 1;
            canvas.width = stage.clientWidth * dpr;
            canvas.height = stage.clientHeight * dpr;
            ctx.scale(dpr, dpr);
        }
        resizeCanvas();

        function switchMode() {
            currentMode = modeSelector.value;
            let labelText = (currentMode === 'spectrum') ? "SPECTRUM" : "VU METER";
            document.getElementById('modeLabel').innerText = labelText;

            // Toggle Toolbar Controls
            const specCtrls = document.getElementById('grp-spec-mode');
            const vuCtrls1 = document.getElementById('grp-vu-mode');
            const vuCtrls2 = document.getElementById('grp-vu-sim');

            if(currentMode === 'spectrum') {
                specCtrls.classList.remove('hidden-ctrl');
                vuCtrls1.classList.add('hidden-ctrl');
                vuCtrls2.classList.add('hidden-ctrl');
            } else {
                specCtrls.classList.add('hidden-ctrl');
                vuCtrls1.classList.remove('hidden-ctrl');
                vuCtrls2.classList.remove('hidden-ctrl');
            }

            // Reset States slightly to prevent visual glitches
            barValues.fill(0);
            volL = 0; volR = 0;
            
            // Clear Screen immediately
            const stage = document.getElementById('stage');
            ctx.clearRect(0,0, stage.clientWidth, stage.clientHeight);
        }

        // --- NETWORK ENGINE ---
        function toggleConnection() {
            if (isConnected) disconnect(); else connect();
        }

        function connect() {
            let ip = ipInput.value.trim();
            if(!ip) ip = window.location.hostname;
            connectBtn.innerText = "STOP";
            isConnected = true;
            statusLight.className = "status-light connected";

            let url = (ip === window.location.hostname && !isLocal) ? '/data' : `http://${ip}/data`;

            pollInterval = setInterval(() => {
                fetch(url)
                    .then(r => r.json())
                    .then(json => {
                        // Route data based on mode
                        if(currentMode === 'spectrum') processSpectrumData(json.data);
                        else processVuData(json.data);
                    })
                    .catch(e => {
                        console.error(e);
                        statusLight.className = "status-light error";
                    });
            }, 40);

            loop(0);
        }

        function disconnect() {
            clearInterval(pollInterval);
            cancelAnimationFrame(animFrame);
            isConnected = false;
            connectBtn.innerText = "LINK";
            statusLight.className = "status-light";
            // Reset visual states
            barValues.fill(0);
            volL = 0; volR = 0;
            draw();
        }

        // --- PHYSICS ENGINES ---
        
        // 1. Spectrum Processor (FFT)
        function processSpectrumData(waveform) {
            const N = waveform.length;
            const gain = parseFloat(document.getElementById('gain').value);
            
            for (let k = 0; k < numBars; k++) {
                let real = 0, imag = 0;
                let freqIndex = Math.floor((k + 1) * 1.5); 

                for (let n = 0; n < N; n++) {
                    const window = 0.5 * (1 - Math.cos((2 * Math.PI * n) / (N - 1)));
                    const sample = waveform[n] * window;
                    const angle = (2 * Math.PI * freqIndex * n) / N;
                    real += sample * Math.cos(angle);
                    imag += sample * Math.sin(angle);
                }
                
                let mag = Math.sqrt(real * real + imag * imag);
                let val = ((mag / N) * gain) / BASE_SENSITIVITY_SPEC; 
                
                if (val > 1.0) val = 1.0;
                if (val > barValues[k]) barValues[k] = val;
            }
            statusLight.className = "status-light connected";
        }

        // 2. VU Processor (Peak Detect)
        function processVuData(data) {
            let maxVal = 0;
            for (let v of data) {
                if (Math.abs(v) > maxVal) maxVal = Math.abs(v);
            }

            const gain = parseFloat(document.getElementById('gain').value);
            let baseVol = (maxVal * gain) / BASE_SENSITIVITY_VU;
            
            // Stereo Sim
            if (document.getElementById('stereoSim').checked) {
                const noise = 0.2; // Fixed noise amount for simplicity
                const jitterL = (Math.random() - 0.5) * 2.0 * noise; 
                const jitterR = (Math.random() - 0.5) * 2.0 * noise;
                targetVolL = baseVol + (baseVol * jitterL);
                targetVolR = baseVol + (baseVol * jitterR);
            } else {
                targetVolL = baseVol;
                targetVolR = baseVol;
            }

            // Cap
            if (targetVolL > 1.2) targetVolL = 1.2;
            if (targetVolR > 1.2) targetVolR = 1.2;
            if (targetVolL < 0) targetVolL = 0;
            if (targetVolR < 0) targetVolR = 0;
            
            statusLight.className = "status-light connected";
        }

        // --- MAIN LOOP ---
        function loop(timestamp) {
            if (!isConnected) return;

            const dt = (timestamp - lastTime) / 1000;
            lastTime = timestamp;

            // FPS Counter
            frameCount++;
            if (timestamp - lastFpsTime >= 1000) {
                document.getElementById('fps').innerText = frameCount;
                frameCount = 0;
                lastFpsTime = timestamp;
            }

            // Route Physics Updates
            if (currentMode === 'spectrum') updateSpectrumPhysics(dt);
            else updateVuPhysics(dt);

            draw();
            animFrame = requestAnimationFrame(loop);
        }

        // --- UPDATE LOGIC ---
        function updateSpectrumPhysics(dt) {
            // Spectrum uses Multiplicative Decay
            const setting = document.getElementById('decay').value;
            let decayRate = 0.8; // Normal
            if(setting === 'slow') decayRate = 0.9;
            if(setting === 'fast') decayRate = 0.6;

            const gravity = 0.5 * dt;

            for (let i = 0; i < numBars; i++) {
                barValues[i] *= decayRate;
                if (barValues[i] < 0.01) barValues[i] = 0;

                if (barValues[i] > peakValues[i]) {
                    peakValues[i] = barValues[i];
                    peakHoldTime[i] = 0.5; 
                } else {
                    if (peakHoldTime[i] > 0) peakHoldTime[i] -= dt;
                    else peakValues[i] -= gravity; 
                }
                if (peakValues[i] < 0) peakValues[i] = 0;
            }
        }

        function updateVuPhysics(dt) {
            // VU uses Ballistic Seeking
            const setting = document.getElementById('decay').value;
            let speed = 8.0; // Normal
            if(setting === 'slow') speed = 4.0;
            if(setting === 'fast') speed = 15.0;

            // Left
            if (targetVolL > volL) volL += (targetVolL - volL) * speed * dt;
            else volL += (targetVolL - volL) * (speed * 0.6) * dt;
            if (volL < 0.005) volL = 0;

            // Right
            if (targetVolR > volR) volR += (targetVolR - volR) * speed * dt;
            else volR += (targetVolR - volR) * (speed * 0.6) * dt;
            if (volR < 0.005) volR = 0;
        }

        // --- DRAWING ROUTER ---
        function draw() {
            const stage = document.getElementById('stage');
            const w = stage.clientWidth;
            const h = stage.clientHeight;

            ctx.clearRect(0, 0, w, h);

            if (currentMode === 'spectrum') {
                drawGrid(w, h);
                const style = document.getElementById('vizMode').value;
                if (style === 'mirror') drawBars(w, h, true);
                else if (style === 'line') drawLine(w, h);
                else drawBars(w, h, false);
            } else {
                const style = document.getElementById('meterStyle').value;
                if (style === 'led') {
                    const splitH = h / 2;
                    drawLedMeter(0, 0, w, splitH, volL, "LEFT CHANNEL");
                    drawLedMeter(0, splitH, w, splitH, volR, "RIGHT CHANNEL");
                } else {
                    const splitW = w / 2;
                    drawFancyAnalog(0, 0, splitW, h, volL, "LEFT");
                    drawFancyAnalog(splitW, 0, splitW, h, volR, "RIGHT");
                }
            }
        }

        // --- DRAW HELPERS: SPECTRUM ---
        function drawGrid(w, h) {
            const styles = getComputedStyle(document.body);
            ctx.strokeStyle = styles.getPropertyValue('--grid-color').trim();
            ctx.lineWidth = 1;
            ctx.beginPath();
            const PADDING_TOP = 20; const PADDING_BOTTOM = 10;
            const drawH = h - PADDING_TOP - PADDING_BOTTOM;
            for(let i=0; i<=10; i++) {
                let y = PADDING_TOP + (drawH/10) * i;
                ctx.moveTo(0, y); ctx.lineTo(w, y);
            }
            ctx.stroke();
        }

        function drawBars(w, h, mirrored) {
            const styles = getComputedStyle(document.body);
            const colTop = styles.getPropertyValue('--bar-top').trim();
            const colMid = styles.getPropertyValue('--bar-mid').trim();
            const colBot = styles.getPropertyValue('--bar-bot').trim();
            const colPeak = styles.getPropertyValue('--peak-color').trim();
            
            const gap = 2;
            const totalBarW = w / numBars;
            const barW = totalBarW - gap;
            const PADDING_TOP = 20; const PADDING_BOTTOM = 10;
            const drawH = h - PADDING_TOP - PADDING_BOTTOM;

            let grad = ctx.createLinearGradient(0, mirrored ? h/2 : h, 0, 0);
            grad.addColorStop(0, colBot); grad.addColorStop(0.6, colMid); grad.addColorStop(1, colTop);
            ctx.fillStyle = grad;

            for (let i = 0; i < numBars; i++) {
                let val = barValues[i];
                let peak = peakValues[i];
                let x = i * totalBarW + (gap/2);

                if (mirrored) {
                    let barH = (val * h) / 2;
                    let cy = h / 2;
                    ctx.fillRect(x, cy - barH, barW, barH * 2);
                    let peakH = (peak * h) / 2;
                    ctx.fillStyle = colPeak;
                    ctx.fillRect(x, cy - peakH - 2, barW, 2); 
                    ctx.fillRect(x, cy + peakH, barW, 2);     
                    ctx.fillStyle = grad; 
                } else {
                    let barH = val * drawH;
                    let y = h - PADDING_BOTTOM - barH;
                    ctx.fillRect(x, y, barW, barH);
                    let peakY = h - PADDING_BOTTOM - (peak * drawH);
                    ctx.fillStyle = colPeak;
                    ctx.fillRect(x, peakY - 2, barW, 2);
                    ctx.fillStyle = grad;
                }
            }
        }

        function drawLine(w, h) {
            const styles = getComputedStyle(document.body);
            const colLine = styles.getPropertyValue('--bar-top').trim();
            const colFill = styles.getPropertyValue('--bar-mid').trim(); 
            const colPeak = styles.getPropertyValue('--peak-color').trim();
            const PADDING_TOP = 20; const PADDING_BOTTOM = 10;
            const drawH = h - PADDING_TOP - PADDING_BOTTOM;
            const bottomY = h - PADDING_BOTTOM;

            ctx.beginPath();
            ctx.moveTo(0, bottomY);
            const step = w / (numBars - 1);
            for (let i = 0; i < numBars; i++) {
                let x = i * step;
                let y = bottomY - (barValues[i] * drawH);
                ctx.lineTo(x, y);
            }
            ctx.lineTo(w, bottomY);
            ctx.closePath();
            
            // Draw Fill
            ctx.save();
            ctx.globalAlpha = 0.2; ctx.fillStyle = colFill; ctx.fill();
            ctx.restore();
            
            // Draw Line
            ctx.strokeStyle = colLine; ctx.lineWidth = 3; ctx.stroke();

            // Draw Peaks (Dots)
            ctx.fillStyle = colPeak;
            for (let i = 0; i < numBars; i++) {
                let x = i * step;
                let y = bottomY - (peakValues[i] * drawH);
                ctx.beginPath();
                ctx.arc(x, y, 3, 0, Math.PI*2);
                ctx.fill();
            }
        }

        // --- DRAW HELPERS: VU METER ---
        function drawLedMeter(x, y, w, h, val, label) {
            const styles = getComputedStyle(document.body);
            const colOff = styles.getPropertyValue('--led-off').trim();
            const colText = styles.getPropertyValue('--text-color').trim();

            const pad = 20;
            const meterW = w - (pad*2);
            const meterH = h * 0.75;
            const meterX = x + pad;
            const meterY = y + (h - meterH)/2;

            ctx.fillStyle = colText;
            ctx.font = "bold 12px sans-serif";
            ctx.textAlign = "left";
            ctx.fillText(label, meterX, meterY - 8);

            const segs = 40;
            const gap = 2;
            const segW = (meterW / segs) - gap;

            for(let i=0; i<segs; i++) {
                const px = meterX + i * (segW + gap);
                const pct = i / segs;
                let col = colOff;
                if (val > pct) {
                    if (pct > 0.85) col = "#ff1744";
                    else if (pct > 0.6) col = "#ffea00";
                    else col = "#00e676";
                }
                ctx.fillStyle = col;
                ctx.fillRect(px, meterY, segW, meterH);
            }
        }

        function drawFancyAnalog(x, y, w, h, val, label) {
            const targetAspect = 1.8;
            const margin = 20;
            if (w <= margin * 2 || h <= margin * 2) return;
            
            let meterW = w - (margin * 2);
            let meterH = meterW / targetAspect;
            if (meterH > h - (margin * 2)) {
                meterH = h - (margin * 2);
                meterW = meterH * targetAspect;
            }

            const mx = x + (w - meterW) / 2;
            const my = y + (h - meterH) / 2;

            const styles = getComputedStyle(document.body);
            const colFace1 = styles.getPropertyValue('--meter-face-1').trim();
            const colFace2 = styles.getPropertyValue('--meter-face-2').trim();
            const colBezel = styles.getPropertyValue('--meter-bezel').trim();
            const colText = styles.getPropertyValue('--meter-text').trim();
            const colNeedle = styles.getPropertyValue('--meter-needle').trim();

            ctx.save();

            // Bezel
            roundedRect(ctx, mx, my, meterW, meterH, 12);
            let gradBezel = ctx.createLinearGradient(mx, my, mx, my + meterH);
            gradBezel.addColorStop(0, colBezel); gradBezel.addColorStop(0.5, "#666"); gradBezel.addColorStop(1, colBezel);
            ctx.fillStyle = gradBezel; ctx.fill();

            // Face
            const border = 6;
            roundedRect(ctx, mx + border, my + border, meterW - (border*2), meterH - (border*2), 8);
            let gradFace = ctx.createRadialGradient(
                mx + meterW/2, my + meterH, 10, 
                mx + meterW/2, my + meterH/1.5, meterW
            );
            gradFace.addColorStop(0, colFace1); gradFace.addColorStop(1, colFace2);
            ctx.fillStyle = gradFace; ctx.fill();

            // Ticks
            const pivotX = mx + meterW / 2;
            const pivotY = my + meterH * 0.85;
            const radius = meterH * 0.75;
            const startAngle = Math.PI * 1.2; 
            const endAngle = Math.PI * 1.8;   
            const totalAngle = endAngle - startAngle;

            ctx.textAlign = "center"; ctx.textBaseline = "middle"; ctx.font = `bold ${meterW * 0.035}px sans-serif`;
            const ticks = 10;
            for (let i = 0; i <= ticks; i++) {
                const pct = i / ticks;
                const theta = startAngle + (pct * totalAngle);
                const isMajor = (i % 5 === 0);
                const tickLen = isMajor ? meterH * 0.1 : meterH * 0.05;
                const x1 = pivotX + Math.cos(theta) * (radius - tickLen);
                const y1 = pivotY + Math.sin(theta) * (radius - tickLen);
                const x2 = pivotX + Math.cos(theta) * radius;
                const y2 = pivotY + Math.sin(theta) * radius;

                if (pct > 0.7) ctx.strokeStyle = "#ff3333"; else ctx.strokeStyle = colText;
                ctx.lineWidth = isMajor ? 3 : 1.5;
                ctx.beginPath(); ctx.moveTo(x1, y1); ctx.lineTo(x2, y2); ctx.stroke();

                if (isMajor) {
                    const labelDist = radius - tickLen - 15;
                    const lx = pivotX + Math.cos(theta) * labelDist;
                    const ly = pivotY + Math.sin(theta) * labelDist;
                    let txt = (i===0) ? "-20" : (i===5) ? "0" : (i===10) ? "+3" : "";
                    ctx.fillStyle = (pct > 0.7) ? "#ff3333" : colText;
                    ctx.fillText(txt, lx, ly);
                }
            }

            // Text
            ctx.fillStyle = colText; ctx.font = `bold ${meterW * 0.05}px sans-serif`;
            ctx.fillText("VU", pivotX, my + meterH * 0.35);
            ctx.font = `${meterW * 0.03}px sans-serif`; ctx.fillStyle = "#888";
            ctx.fillText(label, pivotX, my + meterH * 0.5);

            // Needle
            let safeVal = Math.min(Math.max(val, 0), 1.2);
            let needlePct = (safeVal > 1) ? 1 + (safeVal-1)*0.5 : safeVal;
            const needleAngle = startAngle + (needlePct * totalAngle);

            // Shadow
            ctx.save(); ctx.translate(5, 5);
            drawNeedleShape(ctx, pivotX, pivotY, radius * 0.95, needleAngle, "rgba(0,0,0,0.3)");
            ctx.restore();

            // Real Needle
            drawNeedleShape(ctx, pivotX, pivotY, radius * 0.95, needleAngle, colNeedle);
            
            // Cap
            ctx.beginPath(); ctx.arc(pivotX, pivotY, meterW * 0.03, 0, Math.PI*2);
            let gradCap = ctx.createLinearGradient(pivotX, pivotY-10, pivotX, pivotY+10);
            gradCap.addColorStop(0, "#666"); gradCap.addColorStop(1, "#111");
            ctx.fillStyle = gradCap; ctx.fill();

            // Glass Reflection
            ctx.globalCompositeOperation = "source-over";
            let gradGlass = ctx.createLinearGradient(mx, my, mx, my + meterH * 0.6);
            gradGlass.addColorStop(0, "rgba(255,255,255,0.1)"); gradGlass.addColorStop(1, "rgba(255,255,255,0)");
            roundedRect(ctx, mx+border, my+border, meterW-(border*2), meterH*0.5, 6);
            ctx.fillStyle = gradGlass; ctx.fill();

            ctx.restore();
        }

        function drawNeedleShape(ctx, x, y, len, theta, color) {
            ctx.save(); ctx.translate(x, y); ctx.rotate(theta);
            ctx.beginPath(); ctx.moveTo(0, -2); ctx.lineTo(len, 0); ctx.lineTo(0, 2); ctx.closePath();
            ctx.fillStyle = color; ctx.fill(); ctx.restore();
        }

        function roundedRect(ctx, x, y, width, height, radius) {
            ctx.beginPath();
            ctx.moveTo(x + radius, y);
            ctx.lineTo(x + width - radius, y);
            ctx.quadraticCurveTo(x + width, y, x + width, y + radius);
            ctx.lineTo(x + width, y + height - radius);
            ctx.quadraticCurveTo(x + width, y + height, x + width - radius, y + height);
            ctx.lineTo(x + radius, y + height);
            ctx.quadraticCurveTo(x, y + height, x, y + height - radius);
            ctx.lineTo(x, y + radius);
            ctx.quadraticCurveTo(x, y, x + radius, y);
            ctx.closePath();
        }
    </script>
</body>
</html>
)rawliteral";