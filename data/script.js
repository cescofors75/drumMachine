// RED808 Web Interface - Professional Drum Machine
// Sincronización bidireccional completa

const TRACK_NAMES = ['KICK', 'SNARE', 'CLHAT', 'OPHAT', 'CLAP', 'TOMLO', 'TOMHI', 'CYMBAL'];
const KIT_NAMES = ['808 CLASSIC', 'ELEKTRO', 'HOUSE'];
const MAX_STEPS = 16;
const MAX_TRACKS = 8;
const MAX_PATTERNS = 16;

let currentPattern = 0;
let currentKit = 0;
let currentStep = 0;
let selectedTrack = 0;
let tempo = 120;
let volume = 15;
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
    startSyncLoop();
    
    console.log('RED808 Professional Interface - Loaded');
    
    // Cargar patrón inicial del ESP32
    setTimeout(() => {
        fetchPatternData();
    }, 800);
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
    
    // 8 tracks x 16 steps
    for (let track = 0; track < MAX_TRACKS; track++) {
        for (let step = 0; step < MAX_STEPS; step++) {
            const cell = document.createElement('div');
            cell.className = `step-cell track-${track + 1}`;
            cell.dataset.track = track;
            cell.dataset.step = step;
            
            // Marcar beat divisions
            if (step % 4 === 0) cell.classList.add('beat-start');
            
            cell.addEventListener('click', () => toggleStep(track, step));
            
            grid.appendChild(cell);
        }
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
    const container = document.getElementById('livePadsGrid');
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

// ============================================
// EVENT LISTENERS
// ============================================
function attachEventListeners() {
    // Play/Stop/Record
    const btnPlay = document.getElementById('ctrlPlay');
    const btnStop = document.getElementById('ctrlStop');
    const btnRec = document.getElementById('ctrlRec');
    
    if (btnPlay) btnPlay.addEventListener('click', () => sendCommand('/play'));
    if (btnStop) btnStop.addEventListener('click', () => sendCommand('/stop'));
    if (btnRec) btnRec.addEventListener('click', () => alert('Record mode - Coming soon'));
    
    // Tempo
    const btnTempoPlus = document.getElementById('tempoPlus');
    const btnTempoMinus = document.getElementById('tempoMinus');
    
    if (btnTempoPlus) btnTempoPlus.addEventListener('click', () => sendCommand('/tempo', { delta: 5 }));
    if (btnTempoMinus) btnTempoMinus.addEventListener('click', () => sendCommand('/tempo', { delta: -5 }));
    
    // Volume
    const volumeSlider = document.getElementById('volumeSlider');
    const volValue = document.getElementById('volValue');
    
    if (volumeSlider) {
        volumeSlider.addEventListener('input', (e) => {
            const val = e.target.value;
            if (volValue) volValue.textContent = val;
        });
        
        volumeSlider.addEventListener('change', (e) => {
            const val = e.target.value;
            sendCommand('/volume', { value: val });
        });
    }
    
    // Kit selector
    document.querySelectorAll('.kit-select-btn').forEach((btn, idx) => {
        btn.addEventListener('click', () => {
            sendCommand('/kit', { kit: idx });
        });
    });
    
    // Track labels (selección de track)
    document.querySelectorAll('.track-label').forEach((label, idx) => {
        label.addEventListener('click', () => {
            selectedTrack = idx;
            document.querySelectorAll('.track-label').forEach(l => l.classList.remove('active'));
            label.classList.add('active');
            updateInstrumentDisplay();
        });
    });
    
    // Side buttons
    const btnClear = document.getElementById('btnClear');
    const btnMute = document.getElementById('btnMute');
    const btnCopy = document.getElementById('btnCopy');
    
    if (btnClear) btnClear.addEventListener('click', () => clearTrack());
    if (btnMute) btnMute.addEventListener('click', () => muteTrack());
    if (btnCopy) btnCopy.addEventListener('click', () => copyPattern());
    
    // LCD Tabs
    document.querySelectorAll('.lcd-tab').forEach(tab => {
        tab.addEventListener('click', () => {
            const view = tab.dataset.tab;
            switchView(view);
        });
    });
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
// VIEW SWITCHING
// ============================================
function switchView(view) {
    currentView = view;
    
    // Actualizar tabs
    document.querySelectorAll('.lcd-tab').forEach(tab => {
        tab.classList.toggle('active', tab.dataset.tab === view);
    });
    
    // Mostrar/ocultar secciones
    const sequencer = document.querySelector('.lcd-sequencer');
    
    if (view === 'seq') {
        if (sequencer) sequencer.style.display = 'grid';
        // TODO: Ocultar otras vistas
    } else if (view === 'pad') {
        if (sequencer) sequencer.style.display = 'none';
        // TODO: Mostrar Live Pads
        showLivePads();
    } else if (view === 'boot') {
        if (sequencer) sequencer.style.display = 'none';
        showBootInfo();
    }
}

function showLivePads() {
    const sequencer = document.querySelector('.lcd-sequencer');
    if (!sequencer) return;
    
    // Crear vista de Live Pads
    let padsHTML = '<div class="live-pads-grid">';
    for (let i = 0; i < MAX_TRACKS; i++) {
        padsHTML += `
            <div class="live-pad" data-track="${i}" ontouchstart="triggerSample(${i})" onmousedown="triggerSample(${i})">
                <div class="pad-num">${i + 1}</div>
                <div class="pad-name">${TRACK_NAMES[i]}</div>
            </div>
        `;
    }
    padsHTML += '</div>';
    
    sequencer.outerHTML = padsHTML;
}

function showBootInfo() {
    const sequencer = document.querySelector('.lcd-sequencer');
    if (!sequencer) return;
    
    sequencer.outerHTML = `
        <div class="boot-info">
            <h2>RED808 V5</h2>
            <p>WiFi: Connected</p>
            <p>IP: 192.168.4.1</p>
            <p>Firmware: v5.0</p>
            <p>Patterns: ${MAX_PATTERNS}</p>
            <p>Tracks: ${MAX_TRACKS}</p>
        </div>
    `;
}

function triggerSample(track) {
    sendCommand('/trigger', { track: track });
    
    // Feedback visual
    const pad = document.querySelector(`.live-pad[data-track="${track}"]`);
    if (pad) {
        pad.style.transform = 'scale(0.9)';
        setTimeout(() => { pad.style.transform = 'scale(1)'; }, 100);
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
        
        updateUIFromESP32(data);
        updateConnectionStatus(true);
        
    } catch (error) {
        console.error('Error fetching status:', error);
        updateConnectionStatus(false);
    }
}

function updateUIFromESP32(data) {
    // BPM
    if (data.bpm !== undefined) {
        tempo = data.bpm;
        const elements = ['lcdBpm', 'tempoNum'];
        elements.forEach(id => {
            const el = document.getElementById(id);
            if (el) el.textContent = data.bpm;
        });
    }
    
    // Pattern
    if (data.pattern !== undefined && data.pattern !== currentPattern) {
        currentPattern = data.pattern;
        document.querySelectorAll('.pattern-btn').forEach((btn, idx) => {
            btn.classList.toggle('active', idx === data.pattern);
        });
        const ftPattern = document.getElementById('ftPattern');
        if (ftPattern) ftPattern.textContent = data.pattern + 1;
        
        // Recargar patrón
        fetchPatternData();
    }
    
    // Kit
    if (data.kit !== undefined) {
        currentKit = data.kit;
        const lcdKit = document.getElementById('lcdKit');
        if (lcdKit) lcdKit.textContent = data.kit + 1;
        
        if (data.kitName) {
            const kitDisplay = document.getElementById('kitDisplay');
            if (kitDisplay) kitDisplay.textContent = data.kitName;
        }
        
        document.querySelectorAll('.kit-select-btn').forEach((btn, idx) => {
            btn.classList.toggle('active', idx === data.kit);
        });
    }
    
    // Playing
    if (data.playing !== undefined) {
        isPlaying = data.playing;
        const btnPlay = document.getElementById('ctrlPlay');
        if (btnPlay) {
            btnPlay.classList.toggle('active', data.playing);
            btnPlay.textContent = data.playing ? '⏸' : '▶';
        }
    }
    
    // Current Step
    if (data.step !== undefined) {
        currentStep = data.step;
        updatePlayingStep(data.step);
        
        const ftStep = document.getElementById('ftStep');
        if (ftStep) ftStep.textContent = data.step + 1;
        
        // Actualizar display de 7 segmentos
        updateSeg7Display(data.step + 1);
    }
    
    // Track
    if (data.track !== undefined) {
        selectedTrack = data.track;
        const ftTrack = document.getElementById('ftTrack');
        if (ftTrack) ftTrack.textContent = data.track + 1;
        
        updateInstrumentDisplay();
    }
    
    // Volume
    if (data.volume !== undefined) {
        volume = data.volume;
        const slider = document.getElementById('volumeSlider');
        const volValue = document.getElementById('volValue');
        if (slider) slider.value = data.volume;
        if (volValue) volValue.textContent = data.volume;
    }
}

function updatePlayingStep(step) {
    // Actualizar LEDs
    document.querySelectorAll('.step-led').forEach((led, idx) => {
        led.classList.toggle('active', idx === step);
    });
    
    // Highlight en grid
    if (isPlaying) {
        document.querySelectorAll('.lcd-step').forEach(cell => {
            const cellStep = parseInt(cell.dataset.step);
            if (cellStep === step) {
                cell.classList.add('playing');
                setTimeout(() => cell.classList.remove('playing'), 150);
            }
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
const wifiToggle = document.getElementById('wifiToggle');
const wifiToggleText = document.getElementById('wifiToggleText');
const wifiStatusBadge = document.getElementById('wifiStatusBadge');

if (wifiToggle) {
    wifiToggle.addEventListener('change', async () => {
        const enabled = wifiToggle.checked;
        wifiToggleText.textContent = enabled ? 'Enabling...' : 'Disabling...';
        
        try {
            const response = await fetch(`/wifi?enabled=${enabled ? '1' : '0'}`);
            const text = await response.text();
            console.log('WiFi toggle:', text);
            
            wifiToggleText.textContent = enabled ? 'Enabled' : 'Disabled';
            wifiStatusBadge.textContent = enabled ? 'Active' : 'Inactive';
            wifiStatusBadge.className = 'status-badge ' + (enabled ? 'ok' : 'error');
            
            if (!enabled) {
                // Mostrar advertencia de desconexión
                setTimeout(() => {
                    alert('WiFi will be disabled. You will be disconnected.');
                }, 100);
            }
        } catch (err) {
            console.error('Error toggling WiFi:', err);
            wifiToggle.checked = !enabled; // Revertir
            wifiToggleText.textContent = !enabled ? 'Enabled' : 'Disabled';
        }
    });
    
    // Obtener estado inicial del WiFi
    async function updateWiFiStatus() {
        try {
            const response = await fetch('/wifi');
            const data = await JSON.parse(await response.text());
            wifiToggle.checked = data.enabled;
            wifiToggleText.textContent = data.enabled ? 'Enabled' : 'Disabled';
            wifiStatusBadge.textContent = data.enabled ? 'Active' : 'Inactive';
            wifiStatusBadge.className = 'status-badge ' + (data.enabled ? 'ok' : 'error');
            
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
    setInterval(updateWiFiStatus, 2000);
    updateWiFiStatus();
}

