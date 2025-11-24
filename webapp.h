const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>M5 Audio Console</title>
    <style>
        :root {
            /* --- THEME DEFINITIONS --- */
            
            /* DARK (Default) */
            --bg-color: #1a1a1a;
            --panel-bg: #252525;
            --text-color: #e0e0e0;
            --accent-color: #333;
            --border-color: #444;
            
            /* Analog Meter Specifics */
            --meter-face-1: #2a2a2a;
            --meter-face-2: #111;
            --meter-bezel: #444;
            --meter-text: #ddd;
            --meter-needle: #ff3333;
            --meter-shadow: rgba(0,0,0,0.5);
            
            /* LED Meter Specifics */
            --led-off: #222;
            
            --font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        }

        [data-theme="light"] {
            --bg-color: #f0f2f5;
            --panel-bg: #ffffff;
            --text-color: #333;
            --accent-color: #e0e0e0;
            --border-color: #ccc;
            
            --meter-face-1: #fffbf0; /* Vintage Paper */
            --meter-face-2: #f0e6d2;
            --meter-bezel: #aaa;
            --meter-text: #333;
            --meter-needle: #cc0000;
            --meter-shadow: rgba(0,0,0,0.2);
            
            --led-off: #e0e0e0;
        }

        [data-theme="blue"] {
            --bg-color: #021019;
            --panel-bg: #051e30;
            --text-color: #4fc3f7;
            --accent-color: #082840;
            --border-color: #0d47a1;
            
            --meter-face-1: #052030;
            --meter-face-2: #021019;
            --meter-bezel: #0d47a1;
            --meter-text: #00e5ff;
            --meter-needle: #00e5ff;
            --meter-shadow: rgba(0,229,255,0.2);
            
            --led-off: #062233;
        }

        [data-theme="green"] {
            --bg-color: #051405;
            --panel-bg: #0c260c;
            --text-color: #4caf50;
            --accent-color: #1b5e20;
            --border-color: #2e7d32;
            
            --meter-face-1: #0c260c;
            --meter-face-2: #000;
            --meter-bezel: #33691e;
            --meter-text: #66bb6a;
            --meter-needle: #00ff00;
            --meter-shadow: rgba(0,255,0,0.1);
            
            --led-off: #122b12;
        }

        body {
            background-color: var(--bg-color);
            color: var(--text-color);
            font-family: var(--font-family);
            margin: 0;
            display: flex;
            flex-direction: column;
            height: 100vh;
            transition: all 0.3s ease;
            overflow: hidden; /* Prevent scrollbars */
        }

        /* Top Control Bar */
        .toolbar {
            background-color: var(--panel-bg);
            padding: 10px 20px;
            border-bottom: 1px solid var(--border-color);
            display: flex;
            gap: 15px;
            flex-wrap: wrap;
            align-items: center;
            justify-content: center;
            box-shadow: 0 4px 15px rgba(0,0,0,0.3);
            z-index: 100;
        }

        .control-group {
            display: flex;
            align-items: center;
            gap: 8px;
        }

        label {
            font-size: 0.75rem;
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 1px;
            opacity: 0.7;
        }

        select, input[type="text"] {
            background: var(--bg-color);
            color: var(--text-color);
            border: 1px solid var(--border-color);
            padding: 6px 10px;
            border-radius: 4px;
            font-family: inherit;
            font-size: 0.9rem;
            outline: none;
            transition: border-color 0.2s;
        }
        select:hover, input:hover { border-color: var(--text-color); }
        input[type="text"] { width: 120px; text-align: center; }
        
        /* Checkbox Styling */
        input[type="checkbox"] {
            width: 16px;
            height: 16px;
            cursor: pointer;
            accent-color: var(--text-color);
        }

        button {
            background: var(--accent-color);
            color: var(--text-color);
            border: 1px solid var(--border-color);
            padding: 6px 20px;
            border-radius: 4px;
            cursor: pointer;
            font-weight: bold;
            font-size: 0.85rem;
            transition: all 0.2s;
        }
        button:hover { 
            filter: brightness(1.2); 
            transform: translateY(-1px);
        }
        button:active { transform: translateY(0); }

        /* Status Dot */
        .status-light {
            width: 10px;
            height: 10px;
            border-radius: 50%;
            background-color: #444;
            margin-left: 10px;
            box-shadow: inset 0 1px 2px rgba(0,0,0,0.5);
            transition: all 0.3s;
        }
        .status-light.connected { background-color: #00e676; box-shadow: 0 0 8px #00e676; }
        .status-light.error { background-color: #ff1744; box-shadow: 0 0 8px #ff1744; }

        /* Canvas Area */
        .meter-stage {
            flex: 1;
            position: relative;
            width: 100%;
            height: 100%;
            background: radial-gradient(circle at center, var(--bg-color), #000);
        }

        canvas {
            display: block;
            width: 100%;
            height: 100%;
        }

        .overlay-fps {
            position: absolute;
            bottom: 10px;
            right: 10px;
            font-family: monospace;
            font-size: 0.7rem;
            opacity: 0.3;
            pointer-events: none;
        }
    </style>
</head>
<body>

    <div class="toolbar">
        <div class="control-group">
            <label>IP Address</label>
            <!-- AUTO-DETECTS HOSTNAME IF EMPTY -->
            <input type="text" id="ipAddress" placeholder="192.168.1.X">
            <button onclick="toggleConnection()" id="connectBtn">CONNECT</button>
            <div class="status-light" id="statusIndicator"></div>
        </div>

        <div class="control-group">
            <label>Mode</label>
            <select id="meterStyle" onchange="requestDraw()">
                <option value="analog" selected>Analog Pro</option>
                <option value="led">Digital LED</option>
            </select>
        </div>

        <div class="control-group">
            <label>Gain</label>
            <select id="gain">
                <option value="0.5">0.5x</option>
                <option value="1.0">1.0x</option>
                <option value="2.0">2.0x</option>
                <option value="4.0" selected>4.0x</option>
                <option value="8.0">8.0x</option>
                <option value="12.0">12.0x</option>
            </select>
        </div>

        <!-- CONTROLS FOR STEREO SIM -->
        <div class="control-group">
            <label title="Add random variation to channels">Stereo Sim</label>
            <input type="checkbox" id="stereoSim">
            <select id="noiseAmt">
                <option value="0.10">10%</option>
                <option value="0.20" selected>20%</option>
                <option value="0.30">30%</option>
                <option value="0.40">40%</option>
            </select>
        </div>

        <div class="control-group">
            <label>Theme</label>
            <select id="themeSelector">
                <option value="dark">Studio Dark</option>
                <option value="light">Vintage Light</option>
                <option value="blue">Cyber Blue</option>
                <option value="green">Matrix Green</option>
            </select>
        </div>
    </div>

    <div class="meter-stage" id="stage">
        <canvas id="vuCanvas"></canvas>
        <div class="overlay-fps">RENDER: GPU // FPS: <span id="fps">--</span></div>
    </div>

    <script>
        // --- SYSTEM CONFIG ---
        const canvas = document.getElementById('vuCanvas');
        const ctx = canvas.getContext('2d', { alpha: false });
        const ipInput = document.getElementById('ipAddress');
        const connectBtn = document.getElementById('connectBtn');
        const statusLight = document.getElementById('statusIndicator');
        const themeSelector = document.getElementById('themeSelector');
        const stereoSimCheck = document.getElementById('stereoSim');
        const noiseAmtSelect = document.getElementById('noiseAmt');

        // Detect if running on device
        const isLocal = window.location.protocol === 'file:';
        
        if (!isLocal) {
            ipInput.value = window.location.hostname;
            // AUTO CONNECT ON LOAD IF ON NETWORK
            window.onload = connect; 
        } else {
            ipInput.value = "192.168.1.57"; // Default for local testing
        }

        // --- APP STATE ---
        let isConnected = false;
        let pollInterval = null;
        let animFrame = null;
        let lastTime = 0;
        let frameCount = 0;
        let lastFpsTime = 0;
        
        // Audio State
        let volL = 0;
        let volR = 0;
        let targetVolL = 0; 
        let targetVolR = 0;

        // --- EVENTS ---
        themeSelector.addEventListener('change', (e) => {
            document.documentElement.setAttribute('data-theme', e.target.value);
            requestDraw();
        });

        window.addEventListener('resize', resizeCanvas);

        // --- INITIALIZATION ---
        function resizeCanvas() {
            const stage = document.getElementById('stage');
            const dpr = window.devicePixelRatio || 1;
            canvas.width = stage.clientWidth * dpr;
            canvas.height = stage.clientHeight * dpr;
            ctx.scale(dpr, dpr);
            requestDraw();
        }

        // --- NETWORK LOGIC ---
        function toggleConnection() {
            if (isConnected) disconnect();
            else connect();
        }

        function connect() {
            let ip = ipInput.value.trim();
            if(!ip) ip = window.location.hostname;
            
            connectBtn.innerText = "DISCONNECT";
            isConnected = true;
            statusLight.className = "status-light connected";

            // Use relative path if IP matches current host (avoids CORS)
            let url = (ip === window.location.hostname && !isLocal) ? '/data' : `http://${ip}/data`;

            pollInterval = setInterval(() => {
                fetch(url)
                    .then(r => r.json())
                    .then(json => processData(json.data))
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
            connectBtn.innerText = "CONNECT";
            statusLight.className = "status-light";
            volL = 0; volR = 0; targetVolL = 0; targetVolR = 0;
            requestDraw();
        }

        function processData(data) {
            let maxVal = 0;
            for (let v of data) {
                if (Math.abs(v) > maxVal) maxVal = Math.abs(v);
            }

            const gain = parseFloat(document.getElementById('gain').value);
            let baseVol = (maxVal * gain) / 18000;
            
            // --- STEREO SIMULATION LOGIC ---
            if (stereoSimCheck.checked) {
                const noise = parseFloat(noiseAmtSelect.value);
                const jitterL = (Math.random() - 0.5) * 2.0 * noise; 
                const jitterR = (Math.random() - 0.5) * 2.0 * noise;
                
                targetVolL = baseVol + (baseVol * jitterL);
                targetVolR = baseVol + (baseVol * jitterR);
            } else {
                targetVolL = baseVol;
                targetVolR = baseVol;
            }

            if (targetVolL > 1.2) targetVolL = 1.2;
            if (targetVolR > 1.2) targetVolR = 1.2;
            if (targetVolL < 0) targetVolL = 0;
            if (targetVolR < 0) targetVolR = 0;

            statusLight.className = "status-light connected";
        }

        // --- PHYSICS LOOP ---
        function loop(timestamp) {
            if (!isConnected) return;

            const dt = (timestamp - lastTime) / 1000;
            lastTime = timestamp;

            // FPS
            frameCount++;
            if (timestamp - lastFpsTime >= 1000) {
                document.getElementById('fps').innerText = frameCount;
                frameCount = 0;
                lastFpsTime = timestamp;
            }

            // --- LEFT CHANNEL PHYSICS ---
            const speed = 8.0; 
            if (targetVolL > volL) {
                volL += (targetVolL - volL) * speed * dt; // Attack
            } else {
                volL += (targetVolL - volL) * (speed * 0.6) * dt; // Decay
            }
            if (volL < 0.005) volL = 0;

            // --- RIGHT CHANNEL PHYSICS ---
            if (targetVolR > volR) {
                volR += (targetVolR - volR) * speed * dt;
            } else {
                volR += (targetVolR - volR) * (speed * 0.6) * dt;
            }
            if (volR < 0.005) volR = 0;

            draw();
            animFrame = requestAnimationFrame(loop);
        }

        function requestDraw() {
            if (!isConnected) draw();
        }

        // --- RENDERER ---
        function draw() {
            const stage = document.getElementById('stage');
            const w = stage.clientWidth;
            const h = stage.clientHeight;
            const style = document.getElementById('meterStyle').value;

            ctx.clearRect(0, 0, w, h);

            if (style === 'led') {
                // Keep LED stacked (Vertical list)
                const splitH = h / 2;
                drawLedMeter(0, 0, w, splitH, volL, "LEFT CHANNEL");
                drawLedMeter(0, splitH, w, splitH, volR, "RIGHT CHANNEL");
            } else {
                // Make Analog Side-by-Side (Horizontal list)
                const splitW = w / 2;
                drawFancyAnalog(0, 0, splitW, h, volL, "LEFT");
                drawFancyAnalog(splitW, 0, splitW, h, volR, "RIGHT");
            }
        }

        // --- COMPONENT: FANCY ANALOG METER ---
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

            // 1. BEZEL
            roundedRect(ctx, mx, my, meterW, meterH, 12);
            let gradBezel = ctx.createLinearGradient(mx, my, mx, my + meterH);
            gradBezel.addColorStop(0, colBezel);
            gradBezel.addColorStop(0.5, "#666");
            gradBezel.addColorStop(1, colBezel);
            ctx.fillStyle = gradBezel;
            ctx.fill();

            // 2. FACE
            const border = 6;
            roundedRect(ctx, mx + border, my + border, meterW - (border*2), meterH - (border*2), 8);
            let gradFace = ctx.createRadialGradient(
                mx + meterW/2, my + meterH, 10, 
                mx + meterW/2, my + meterH/1.5, meterW
            );
            gradFace.addColorStop(0, colFace1);
            gradFace.addColorStop(1, colFace2);
            ctx.fillStyle = gradFace;
            ctx.fill();
            ctx.strokeStyle = "rgba(0,0,0,0.3)";
            ctx.lineWidth = 2;
            ctx.stroke();

            // 3. SCALE
            const pivotX = mx + meterW / 2;
            const pivotY = my + meterH * 0.85;
            const radius = meterH * 0.75;
            const startAngle = Math.PI * 1.2; 
            const endAngle = Math.PI * 1.8;   
            const totalAngle = endAngle - startAngle;

            // 4. TICKS
            ctx.strokeStyle = colText;
            ctx.fillStyle = colText;
            ctx.textAlign = "center";
            ctx.textBaseline = "middle";
            ctx.font = `bold ${meterW * 0.035}px sans-serif`;

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

                if (pct > 0.7) ctx.strokeStyle = "#ff3333";
                else ctx.strokeStyle = colText;

                ctx.lineWidth = isMajor ? 3 : 1.5;
                ctx.beginPath();
                ctx.moveTo(x1, y1);
                ctx.lineTo(x2, y2);
                ctx.stroke();

                if (isMajor) {
                    const labelDist = radius - tickLen - 15;
                    const lx = pivotX + Math.cos(theta) * labelDist;
                    const ly = pivotY + Math.sin(theta) * labelDist;
                    
                    let txt = "";
                    if (i===0) txt = "-20";
                    if (i===5) txt = "0";
                    if (i===10) txt = "+3";
                    
                    ctx.fillStyle = (pct > 0.7) ? "#ff3333" : colText;
                    ctx.fillText(txt, lx, ly);
                }
            }

            // 5. LABEL
            ctx.fillStyle = colText;
            ctx.font = `bold ${meterW * 0.05}px sans-serif`;
            ctx.fillText("VU", pivotX, my + meterH * 0.35);
            
            ctx.font = `${meterW * 0.03}px sans-serif`;
            ctx.fillStyle = "#888";
            ctx.fillText(label, pivotX, my + meterH * 0.5);

            // 6. NEEDLE
            let safeVal = Math.min(Math.max(val, 0), 1.2);
            let needlePct = safeVal; 
            if (needlePct > 1) needlePct = 1 + (safeVal-1)*0.5;

            const needleAngle = startAngle + (needlePct * totalAngle);

            ctx.save();
            ctx.translate(5, 5);
            drawNeedleShape(ctx, pivotX, pivotY, radius * 0.95, needleAngle, "rgba(0,0,0,0.3)");
            ctx.restore();

            drawNeedleShape(ctx, pivotX, pivotY, radius * 0.95, needleAngle, colNeedle);

            ctx.beginPath();
            ctx.arc(pivotX, pivotY, meterW * 0.03, 0, Math.PI*2);
            let gradCap = ctx.createLinearGradient(pivotX, pivotY-10, pivotX, pivotY+10);
            gradCap.addColorStop(0, "#666");
            gradCap.addColorStop(1, "#111");
            ctx.fillStyle = gradCap;
            ctx.fill();

            // 7. GLASS
            ctx.globalCompositeOperation = "source-over";
            let gradGlass = ctx.createLinearGradient(mx, my, mx, my + meterH * 0.6);
            gradGlass.addColorStop(0, "rgba(255,255,255,0.1)");
            gradGlass.addColorStop(1, "rgba(255,255,255,0)");
            roundedRect(ctx, mx+border, my+border, meterW-(border*2), meterH*0.5, 6);
            ctx.fillStyle = gradGlass;
            ctx.fill();

            ctx.restore();
        }

        function drawNeedleShape(ctx, x, y, len, theta, color) {
            ctx.save();
            ctx.translate(x, y);
            ctx.rotate(theta);
            ctx.beginPath();
            ctx.moveTo(0, -2);
            ctx.lineTo(len, 0);
            ctx.lineTo(0, 2);
            ctx.closePath();
            ctx.fillStyle = color;
            ctx.fill();
            ctx.restore();
        }

        // --- COMPONENT: LED METER ---
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

        resizeCanvas();
    </script>
</body>
</html>
)rawliteral";