// RED808 Web Interface - Professional Drum Machine
// Sincronización bidireccional completa

const TRACK_NAMES = ['KICK', 'SNARE', 'CLHAT', 'OPHAT', 'CLAP', 'TOMLO', 'TOMHI', 'CYMBAL', 'PERCUSS', 'COWBELL', 'MARACAS', 'WHISTLE', 'CRASH', 'RIDE', 'CLAVES', 'RIMSHOT'];
const KIT_NAMES = ['808 CLASSIC', '808 BRIGHT', '808 DRY'];
const THEME_NAMES = ['RED808', 'NAVY', 'CYBER', 'EMERALD'];
const MAX_STEPS = 16;
const MAX_TRACKS = 16;
const MAX_PATTERNS = 16;

let currentPattern = 0;
let currentKit = 0;
let currentStep = 0;
let selectedTrack = 0;
let tempo = 120;
let volume = 15;
let currentTheme = 0;
let isPlaying = false;
let patterns = [];
let updateInterval = null;

// ============================================
// INICIALIZACIÓN
// ============================================
document.addEventListener('DOMContentLoaded', () => {
    initializePatterns();
    buildTrackList();
    buildStepGrid();
    buildStepNumbers();
    buildPatternGrid();
    buildLivePads();
    attachEventListeners();
    
    console.log('RED808 Professional Interface - Loaded');
    
    // Cargar estado inicial del ESP32 ANTES de iniciar sync loop
    setTimeout(() => {
        fetchStatus(); // Cargar BPM, volumen, etc
        fetchPatternData(); // Cargar patrón
        
        // Iniciar sync loop después de cargar estado inicial
        setTimeout(() => {
            startSyncLoop();
        }, 300);
    }, 500);
});

function initializePatterns() {
    patterns = Array(MAX_PATTERNS).fill(null).map(() => 
        Array(MAX_TRACKS).fill(null).map(() => 
            Array(MAX_STEPS).fill(false)
        )
    );
}

// ============================================
// CONSTRUCCIÓN DE UI MODERNA
// ============================================
function buildTrackList() {
    const container = document.getElementById('trackList');
    if (!container) return;
    
    container.innerHTML = '';
    
    TRACK_NAMES.forEach((name, index) => {
        const track = document.createElement('div');
        track.className = 'track-item';
        track.dataset.track = index;
        if (index === 0) track.classList.add('active');
        
        track.innerHTML = `
            <div class="track-color track-${index + 1}"></div>
            <span class="track-name">${name}</span>
            <div class="track-actions">
                <button class="track-action-btn" data-action="mute" title="Mute">M</button>
                <button class="track-action-btn" data-action="clear" title="Clear">×</button>
            </div>
        `;
        
        track.addEventListener('click', (e) => {
            if (!e.target.closest('.track-actions')) {
                selectTrack(index);
            }
        });
        
        // Acciones de track
        track.querySelector('[data-action="mute"]').addEventListener('click', () => muteTrack(index));
        track.querySelector('[data-action="clear"]').addEventListener('click', () => clearTrack(index));
        
        container.appendChild(track);
    });
}

function buildStepGrid() {
    const grid = document.getElementById('stepGrid');
    if (!grid) return;
    
    grid.innerHTML = '';
    
    // 8 tracks, cada uno con su fila de 16 steps
    for (let track = 0; track < MAX_TRACKS; track++) {
        const trackRow = document.createElement('div');
        trackRow.className = 'track-row';
        trackRow.dataset.track = track;
        
        for (let step = 0; step < MAX_STEPS; step++) {
            const cell = document.createElement('div');
            cell.className = 'step-cell';
            cell.dataset.track = track;
            cell.dataset.step = step;
            
            // Marcar beat divisions
            if (step % 4 === 0) cell.classList.add('beat-start');
            
            cell.addEventListener('click', () => toggleStep(track, step));
            
            trackRow.appendChild(cell);
        }
        
        grid.appendChild(trackRow);
    }
}

function buildStepNumbers() {
    const container = document.getElementById('stepNumbers');
    if (!container) return;
    
    container.innerHTML = '';
    
    for (let i = 1; i <= MAX_STEPS; i++) {
        const num = document.createElement('div');
        num.className = 'step-number';
        num.textContent = i;
        container.appendChild(num);
    }
}

function buildPatternGrid() {
    const container = document.getElementById('patternGrid');
    if (!container) return;
    
    container.innerHTML = '';
    
    for (let i = 0; i < MAX_PATTERNS; i++) {
        const btn = document.createElement('button');
        btn.className = 'pattern-btn';
        btn.textContent = (i + 1).toString().padStart(2, '0');
        btn.dataset.pattern = i;
        
        if (i === 0) btn.classList.add('active');
        
        btn.addEventListener('click', () => changePattern(i));
        
        container.appendChild(btn);
    }
}

function buildLivePads() {
    const container = document.getElementById('padsGrid');
    if (!container) return;
    
    container.innerHTML = '';
    
    for (let i = 0; i < MAX_TRACKS; i++) {
        const pad = document.createElement('div');
        pad.className = `live-pad track-${i + 1}`;
        pad.dataset.track = i;
        
        pad.innerHTML = `
            <div class="pad-number">${i + 1}</div>
            <div class="pad-name">${TRACK_NAMES[i]}</div>
        `;
        
        pad.addEventListener('click', () => triggerSample(i));
        
        container.appendChild(pad);
    }
}

function selectTrack(trackNum) {
    selectedTrack = trackNum;
    
    // Actualizar UI de track list
    document.querySelectorAll('.track-item').forEach((item, idx) => {
        item.classList.toggle('active', idx === trackNum);
    });
    
    // Enviar al ESP32
    sendCommand('/track', { track: trackNum });
    
    console.log(`Track ${trackNum + 1} selected: ${TRACK_NAMES[trackNum]}`);
}

// ============================================
// NAVIGATION
// ============================================
function setupNavigation() {
    document.querySelectorAll('.nav-item').forEach(item => {
        item.addEventListener('click', (e) => {
            e.preventDefault();
            const view = item.dataset.view;
            switchView(view);
        });
    });
}

function switchView(view) {
    // Actualizar navegación
    document.querySelectorAll('.nav-item').forEach(item => {
        item.classList.toggle('active', item.dataset.view === view);
    });
    
    // Actualizar vistas
    document.querySelectorAll('.view-content').forEach(v => {
        v.classList.toggle('active', v.id === `view${view.charAt(0).toUpperCase() + view.slice(1)}`);
    });
    
    // Actualizar título
    const titles = {
        'sequencer': 'Sequencer',
        'livepads': 'Live Pads',
        'settings': 'Settings',
        'diagnostic': 'Diagnostic'
    };
    
    const titleEl = document.getElementById('viewTitle');
    if (titleEl) titleEl.textContent = titles[view] || 'RED808';
    
    // Inicializar controles específicos de cada vista
    if (view === 'settings') {
        initializeWiFiControls();
    }
    
    console.log(`Switched to view: ${view}`);
}

// ============================================
// EVENT LISTENERS
// ============================================
function attachEventListeners() {
    // Navigation
    setupNavigation();
    
    // Transport controls
    const btnPlay = document.getElementById('transportPlay');
    const btnStop = document.getElementById('transportStop');
    
    if (btnPlay) btnPlay.addEventListener('click', () => {
        sendCommand('/play');
        console.log('Play clicked');
    });
    if (btnStop) btnStop.addEventListener('click', () => {
        sendCommand('/stop');
        console.log('Stop clicked');
    });
    
    // Tempo controls
    const btnTempoUp = document.getElementById('tempoUp');
    const btnTempoDown = document.getElementById('tempoDown');
    
    if (btnTempoUp) btnTempoUp.addEventListener('click', () => {
        sendCommand('/tempo', { delta: 5 });
    });
    if (btnTempoDown) btnTempoDown.addEventListener('click', () => {
        sendCommand('/tempo', { delta: -5 });
    });
    
    // Volume controls en header
    const btnVolumeUp = document.getElementById('volumeUp');
    const btnVolumeDown = document.getElementById('volumeDown');
    
    if (btnVolumeUp) btnVolumeUp.addEventListener('click', () => {
        volume = Math.min(30, volume + 1);
        updateVolumeDisplay();
        sendCommand('/volume', { value: volume });
    });
    if (btnVolumeDown) btnVolumeDown.addEventListener('click', () => {
        volume = Math.max(0, volume - 1);
        updateVolumeDisplay();
        sendCommand('/volume', { value: volume });
    });
    
    // Volume slider en settings
    const volumeSlider = document.getElementById('volumeSlider');
    const volumeValue = document.getElementById('volumeValue');
    
    if (volumeSlider) {
        volumeSlider.addEventListener('input', (e) => {
            const val = e.target.value;
            if (volumeValue) volumeValue.textContent = val;
        });
        
        volumeSlider.addEventListener('change', (e) => {
            const val = e.target.value;
            sendCommand('/volume', { value: val });
            console.log(`Volume changed to ${val}`);
        });
    }
    
    // Kit selector
    const kitSelector = document.getElementById('kitSelector');
    if (kitSelector) {
        kitSelector.addEventListener('change', (e) => {
            const kitNum = parseInt(e.target.value);
            sendCommand('/kit', { kit: kitNum });
            console.log(`Kit changed to ${kitNum}`);
        });
    }
    
    // Pattern buttons
    const btnCopyPattern = document.getElementById('btnCopyPattern');
    const btnClearPattern = document.getElementById('btnClearPattern');
    
    if (btnCopyPattern) btnCopyPattern.addEventListener('click', () => copyPattern());
    if (btnClearPattern) btnClearPattern.addEventListener('click', () => clearPattern());
    
    // Live Pads controls
    const padLoopBtn = document.getElementById('padLoopBtn');
    const padHoldBtn = document.getElementById('padHoldBtn');
    const padRecBtn = document.getElementById('padRecBtn');
    const padPlayStopBtn = document.getElementById('padPlayStopBtn');
    
    if (padLoopBtn) padLoopBtn.addEventListener('click', () => {
        padLoopBtn.classList.toggle('active');
        const enabled = padLoopBtn.classList.contains('active');
        sendCommand('/loop', { enabled: enabled ? 1 : 0 });
        console.log('Pad Loop:', enabled);
    });
    
    if (padHoldBtn) padHoldBtn.addEventListener('click', () => {
        padHoldBtn.classList.toggle('active');
        const enabled = padHoldBtn.classList.contains('active');
        sendCommand('/hold', { enabled: enabled ? 1 : 0 });
        console.log('Pad Hold:', enabled);
    });
    
    if (padRecBtn) padRecBtn.addEventListener('click', () => {
        padRecBtn.classList.toggle('active');
        const enabled = padRecBtn.classList.contains('active');
        sendCommand('/record', { enabled: enabled ? 1 : 0 });
        console.log('Pad Record:', enabled);
    });
    
    if (padPlayStopBtn) padPlayStopBtn.addEventListener('click', () => {
        padPlayStopBtn.classList.toggle('active');
        const enabled = padPlayStopBtn.classList.contains('active');
        if (enabled) {
            sendCommand('/play');
            const svg = padPlayStopBtn.querySelector('svg path');
            if (svg) svg.setAttribute('d', 'M6 6h12v12H6z'); // Stop icon
            padPlayStopBtn.querySelector('span').textContent = 'STOP';
        } else {
            sendCommand('/stop');
            const svg = padPlayStopBtn.querySelector('svg path');
            if (svg) svg.setAttribute('d', 'M8 5v14l11-7z'); // Play icon
            padPlayStopBtn.querySelector('span').textContent = 'PLAY';
        }
        console.log('Pad Play/Stop:', enabled);
    });
    
    // Kit selector en Live Pads
    const kitSelectorPads = document.getElementById('kitSelectorPads');
    if (kitSelectorPads) {
        kitSelectorPads.addEventListener('change', (e) => {
            const kitNum = parseInt(e.target.value);
            currentKit = kitNum;
            sendCommand('/kit', { kit: kitNum });
            
            // Sincronizar con el otro selector
            const kitSelector = document.getElementById('kitSelector');
            if (kitSelector) kitSelector.value = kitNum;
            
            console.log(`Kit changed to ${kitNum}`);
        });
    }
    
    // Theme selector
    document.querySelectorAll('.theme-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const theme = parseInt(btn.dataset.theme);
            changeTheme(theme);
        });
    });
    
    // Keyboard shortcuts globales
    document.addEventListener('keydown', (e) => {
        // Solo activar si no estamos escribiendo en un input
        if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA' || e.target.tagName === 'SELECT') {
            return;
        }
        
        const key = e.key.toLowerCase();
        
        // Teclas 1-8: Live Pads
        if (key >= '1' && key <= '8') {
            const track = parseInt(key) - 1;
            triggerSample(track);
            e.preventDefault();
        }
        
        // Barra espaciadora: Play/Stop
        else if (key === ' ' || e.code === 'Space') {
            togglePlayStop();
            e.preventDefault();
        }
        
        // Q/A: BPM
        else if (key === 'q') {
            tempo = Math.min(300, tempo + 5);
            updateBPMDisplay();
            sendCommand('/tempo', { bpm: tempo });
            e.preventDefault();
        }
        else if (key === 'a') {
            tempo = Math.max(40, tempo - 5);
            updateBPMDisplay();
            sendCommand('/tempo', { bpm: tempo });
            e.preventDefault();
        }
        
        // W/S: Volumen
        else if (key === 'w') {
            volume = Math.min(30, volume + 1);
            updateVolumeDisplay();
            sendCommand('/volume', { value: volume });
            e.preventDefault();
        }
        else if (key === 's') {
            volume = Math.max(0, volume - 1);
            updateVolumeDisplay();
            sendCommand('/volume', { value: volume });
            e.preventDefault();
        }
    });
    
    console.log('Event listeners attached');
}

// ============================================
// STEP CONTROL
// ============================================
function toggleStep(track, step) {
    const isActive = patterns[currentPattern][track][step];
    patterns[currentPattern][track][step] = !isActive;
    
    // Actualizar UI inmediatamente
    updateStepCell(track, step, !isActive);
    
    // Enviar al ESP32
    sendCommand('/step', {
        pattern: currentPattern,
        track: track,
        step: step,
        value: !isActive ? 1 : 0
    });
    
    console.log(`Step toggled: T${track} S${step} = ${!isActive}`);
}

function updateStepCell(track, step, active) {
    const cell = document.querySelector(`.step-cell[data-track="${track}"][data-step="${step}"]`);
    if (cell) {
        cell.classList.toggle('active', active);
    }
}

function updateGridFromPattern() {
    for (let track = 0; track < MAX_TRACKS; track++) {
        for (let step = 0; step < MAX_STEPS; step++) {
            const active = patterns[currentPattern][track][step];
            updateStepCell(track, step, active);
        }
    }
    console.log(`Grid synced with pattern ${currentPattern + 1}`);
}

// ============================================
// TRACK OPERATIONS
// ============================================
function clearTrack() {
    if (!confirm(`Clear track ${selectedTrack + 1} (${TRACK_NAMES[selectedTrack]})?`)) return;
    
    // Limpiar localmente
    for (let step = 0; step < MAX_STEPS; step++) {
        patterns[currentPattern][selectedTrack][step] = false;
        updateStepCell(selectedTrack, step, false);
    }
    
    // Enviar al ESP32
    sendCommand('/clear', { track: selectedTrack });
    
    console.log(`Track ${selectedTrack} cleared`);
}

function muteTrack() {
    // Toggle mute (implementación simple)
    sendCommand('/mute', { track: selectedTrack });
    
    const label = document.querySelector(`.track-label[data-track="${selectedTrack}"]`);
    if (label) {
        label.style.opacity = label.style.opacity === '0.4' ? '1' : '0.4';
    }
    
    console.log(`Track ${selectedTrack} mute toggled`);
}

function copyPattern() {
    const targetPattern = prompt(`Copy pattern ${currentPattern + 1} to pattern (1-${MAX_PATTERNS}):`, currentPattern + 2);
    if (!targetPattern) return;
    
    const target = parseInt(targetPattern) - 1;
    if (target < 0 || target >= MAX_PATTERNS) {
        alert('Invalid pattern number');
        return;
    }
    
    sendCommand('/copy', { from: currentPattern, to: target });
    alert(`Pattern ${currentPattern + 1} copied to ${target + 1}`);
}

function clearPattern() {
    if (!confirm(`Clear entire pattern ${currentPattern + 1}?`)) return;
    
    // Limpiar todos los tracks
    for (let track = 0; track < MAX_TRACKS; track++) {
        for (let step = 0; step < MAX_STEPS; step++) {
            patterns[currentPattern][track][step] = false;
            updateStepCell(track, step, false);
        }
        // Enviar clear al ESP32 por cada track
        sendCommand('/clear', { track: track });
    }
    
    console.log(`Pattern ${currentPattern + 1} cleared`);
}

function triggerSample(track) {
    // Enviar trigger al ESP32
    sendCommand('/trigger', { track: track });
    
    // Feedback visual
    const pad = document.querySelector(`.live-pad[data-track="${track}"]`);
    if (pad) {
        pad.classList.add('active');
        setTimeout(() => pad.classList.remove('active'), 200);
    }
    
    console.log(`Sample ${track + 1} triggered`);
}

// ============================================
// HELPER FUNCTIONS PARA DISPLAYS
// ============================================
function updateBPMDisplay() {
    // Actualizar header
    const bpmDisplayHeader = document.getElementById('bpmDisplay');
    if (bpmDisplayHeader) bpmDisplayHeader.textContent = tempo;
    
    // Actualizar settings
    const tempoSlider = document.getElementById('tempoSlider');
    const tempoValue = document.getElementById('tempoValue');
    if (tempoSlider) tempoSlider.value = tempo;
    if (tempoValue) tempoValue.textContent = tempo;
    
    // Actualizar footer
    const ftTempo = document.getElementById('ftTempo');
    if (ftTempo) ftTempo.textContent = tempo;
}

function updateVolumeDisplay() {
    // Actualizar header
    const volumeDisplayHeader = document.getElementById('volumeDisplayHeader');
    if (volumeDisplayHeader) volumeDisplayHeader.textContent = volume;
    
    // Actualizar settings
    const volumeSlider = document.getElementById('volumeSlider');
    const volumeValue = document.getElementById('volumeValue');
    if (volumeSlider) volumeSlider.value = volume;
    if (volumeValue) volumeValue.textContent = volume;
}

// ============================================
// PATTERN MANAGEMENT
// ============================================
function changePattern(patternNum) {
    if (patternNum === currentPattern) return;
    
    currentPattern = patternNum;
    
    // Actualizar botones
    document.querySelectorAll('.pattern-btn').forEach((btn, idx) => {
        btn.classList.toggle('active', idx === patternNum);
    });
    
    // Enviar al ESP32
    sendCommand('/pattern', { pattern: patternNum });
    
    // Cargar patrón
    fetchPatternData();
    
    // Actualizar footer
    const ftPattern = document.getElementById('ftPattern');
    if (ftPattern) ftPattern.textContent = patternNum + 1;
}

function changeTheme(themeNum) {
    if (themeNum === currentTheme) return;
    
    currentTheme = themeNum;
    
    // Actualizar botones
    document.querySelectorAll('.theme-btn').forEach((btn, idx) => {
        btn.classList.toggle('active', idx === themeNum);
    });
    
    // Aplicar tema al body
    document.body.className = ''; // Limpiar clases
    if (themeNum === 1) document.body.classList.add('theme-navy');
    else if (themeNum === 2) document.body.classList.add('theme-cyber');
    else if (themeNum === 3) document.body.classList.add('theme-emerald');
    // themeNum === 0 es RED808, no necesita clase adicional
    
    // Enviar al ESP32
    sendCommand('/theme', { theme: themeNum });
    
    console.log(`Theme changed to ${THEME_NAMES[themeNum]}`);
}

async function fetchPatternData() {
    try {
        const response = await fetch('/getpattern');
        const data = await response.json();
        
        if (data && data.data) {
            // Actualizar patrón local
            patterns[currentPattern] = data.data;
            updateGridFromPattern();
            console.log(`Pattern ${currentPattern + 1} loaded`);
        }
    } catch (error) {
        console.error('Error fetching pattern:', error);
    }
}

// ============================================
// COMUNICACIÓN CON ESP32
// ============================================
async function sendCommand(endpoint, data = {}) {
    try {
        const params = new URLSearchParams(data);
        const url = endpoint + (params.toString() ? '?' + params : '');
        const response = await fetch(url);
        
        if (!response.ok) {
            console.error('Command failed:', endpoint, response.status);
        } else {
            console.log('Command sent:', endpoint, data);
        }
    } catch (error) {
        console.error('Error sending command:', error);
        updateConnectionStatus(false);
    }
}

async function fetchStatus() {
    try {
        const response = await fetch('/status');
        const data = await response.json();
        
        console.log('Status received:', data); // Debug log
        
        updateUIFromESP32(data);
        updateConnectionStatus(true);
        
    } catch (error) {
        console.error('Error fetching status:', error);
        updateConnectionStatus(false);
    }
}

function updateUIFromESP32(data) {
    // BPM/Tempo
    if (data.bpm !== undefined) {
        tempo = data.bpm;
        const tempoDisplay = document.getElementById('tempoDisplay');
        const headerBpm = document.getElementById('headerBpm');
        if (tempoDisplay) tempoDisplay.textContent = data.bpm;
        if (headerBpm) headerBpm.textContent = data.bpm;
    }
    
    // Pattern
    if (data.pattern !== undefined && data.pattern !== currentPattern) {
        currentPattern = data.pattern;
        document.querySelectorAll('.pattern-btn').forEach((btn, idx) => {
            btn.classList.toggle('active', idx === data.pattern);
        });
        const headerPattern = document.getElementById('headerPattern');
        if (headerPattern) headerPattern.textContent = data.pattern + 1;
        
        // Recargar patrón
        fetchPatternData();
    }
    
    // Kit
    if (data.kit !== undefined) {
        currentKit = data.kit;
        const headerKit = document.getElementById('headerKit');
        if (headerKit) headerKit.textContent = data.kit + 1;
        
        const kitSelector = document.getElementById('kitSelector');
        if (kitSelector) kitSelector.value = data.kit;
    }
    
    // Playing state
    if (data.playing !== undefined) {
        isPlaying = data.playing;
        const btnPlay = document.getElementById('transportPlay');
        if (btnPlay) {
            btnPlay.classList.toggle('active', data.playing);
            const svg = btnPlay.querySelector('svg path');
            if (svg && data.playing) {
                svg.setAttribute('d', 'M6 4h4v16H6V4zm8 0h4v16h-4V4z'); // Pause icon
            } else if (svg) {
                svg.setAttribute('d', 'M8 5v14l11-7z'); // Play icon
            }
        }
    }
    
    // Current Step (playhead)
    if (data.step !== undefined) {
        currentStep = data.step;
        updatePlayingStep(data.step);
        
        const headerStep = document.getElementById('headerStep');
        if (headerStep) headerStep.textContent = data.step + 1;
    }
    
    // Track
    if (data.track !== undefined) {
        selectedTrack = data.track;
        const headerTrack = document.getElementById('headerTrack');
        if (headerTrack) headerTrack.textContent = data.track + 1;
    }
    
    // Volume
    if (data.volume !== undefined) {
        volume = data.volume;
        console.log('Volume received from ESP32:', data.volume);
        
        const volumeDisplayHeader = document.getElementById('volumeDisplayHeader');
        if (volumeDisplayHeader) volumeDisplayHeader.textContent = data.volume;
        
        const volumeSlider = document.getElementById('volumeSlider');
        const volumeValue = document.getElementById('volumeValue');
        if (volumeSlider) volumeSlider.value = data.volume;
        if (volumeValue) volumeValue.textContent = data.volume;
    } else {
        console.log('Volume not received in status data');
    }
    
    // Theme
    if (data.theme !== undefined && data.theme !== currentTheme) {
        changeTheme(data.theme);
    }
    
    // Connection OK
    updateConnectionStatus(true);
}

function updatePlayingStep(step) {
    // Highlight en grid - resaltar columna del step actual
    if (isPlaying) {
        document.querySelectorAll('.step-cell').forEach(cell => {
            const cellStep = parseInt(cell.dataset.step);
            cell.classList.toggle('playing', cellStep === step);
        });
        
        // Resaltar número de step
        document.querySelectorAll('.step-number').forEach((num, idx) => {
            num.classList.toggle('active', idx === step);
        });
    }
}

function updateSeg7Display(value) {
    const str = value.toString().padStart(3, '0');
    const digits = ['seg7_1', 'seg7_2', 'seg7_3'];
    digits.forEach((id, idx) => {
        const el = document.getElementById(id);
        if (el) el.textContent = str[idx];
    });
}

function updateInstrumentDisplay() {
    const instNum = document.getElementById('instNum');
    const instName = document.getElementById('instName');
    
    if (instNum) instNum.textContent = (selectedTrack + 1).toString().padStart(2, '0');
    if (instName) instName.textContent = TRACK_NAMES[selectedTrack];
}

function updateConnectionStatus(connected) {
    const led = document.getElementById('statusLed');
    if (led) {
        led.style.background = connected ? '#0f0' : '#f00';
        led.style.boxShadow = connected ? '0 0 20px #0f0' : '0 0 20px #f00';
    }
}

// ============================================
// SYNC LOOP
// ============================================
function startSyncLoop() {
    fetchStatus();
    updateInterval = setInterval(() => fetchStatus(), 300);
}

function stopSyncLoop() {
    if (updateInterval) {
        clearInterval(updateInterval);
        updateInterval = null;
    }
}

// Pause sync cuando no visible
document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
        stopSyncLoop();
    } else {
        startSyncLoop();
    }
});

window.addEventListener('beforeunload', () => stopSyncLoop());

// ============================================
// WIFI CONTROL
// ============================================
function initializeWiFiControls() {
    const wifiToggle = document.getElementById('wifiToggle');
    const wifiToggleText = document.getElementById('wifiToggleText');
    const wifiStatusBadge = document.getElementById('wifiStatusBadge');

    if (!wifiToggle) return; // No inicializar si no están los elementos
    
    wifiToggle.addEventListener('change', async () => {
        const enabled = wifiToggle.checked;
        if (wifiToggleText) wifiToggleText.textContent = enabled ? 'Enabling...' : 'Disabling...';
        
        try {
            const response = await fetch(`/wifi?enabled=${enabled ? '1' : '0'}`);
            const text = await response.text();
            console.log('WiFi toggle:', text);
            
            if (wifiToggleText) wifiToggleText.textContent = enabled ? 'Enabled' : 'Disabled';
            if (wifiStatusBadge) {
                wifiStatusBadge.textContent = enabled ? 'Active' : 'Inactive';
                wifiStatusBadge.className = 'status-badge ' + (enabled ? 'ok' : 'error');
            }
            
            if (!enabled) {
                // Mostrar advertencia de desconexión
                setTimeout(() => {
                    alert('WiFi will be disabled. You will be disconnected.');
                }, 100);
            }
        } catch (err) {
            console.error('Error toggling WiFi:', err);
            wifiToggle.checked = !enabled; // Revertir
            if (wifiToggleText) wifiToggleText.textContent = !enabled ? 'Enabled' : 'Disabled';
        }
    });
    
    // Obtener estado inicial del WiFi
    async function updateWiFiStatus() {
        try {
            const response = await fetch('/wifi');
            const data = await JSON.parse(await response.text());
            if (wifiToggle) wifiToggle.checked = data.enabled;
            if (wifiToggleText) wifiToggleText.textContent = data.enabled ? 'Enabled' : 'Disabled';
            if (wifiStatusBadge) {
                wifiStatusBadge.textContent = data.enabled ? 'Active' : 'Inactive';
                wifiStatusBadge.className = 'status-badge ' + (data.enabled ? 'ok' : 'error');
            }
            
            // Actualizar clientes conectados
            const clientsDisplay = document.getElementById('connectedClients');
            if (clientsDisplay && data.enabled) {
                clientsDisplay.textContent = data.clients || 0;
            }
        } catch (err) {
            console.error('Error fetching WiFi status:', err);
        }
    }
    
    // Actualizar estado cada 2 segundos
    updateWiFiStatus(); // Estado inicial
    setInterval(updateWiFiStatus, 2000);
}

