// RED808 Web Interface - Professional Drum Machine
// Sincronización bidireccional en tiempo real

const TRACK_NAMES = ['KICK', 'SNARE', 'CLAP', 'HIHAT', 'TOM', 'PERC', 'COWBL', 'CYMBAL'];
const KIT_NAMES = ['808 CLASSIC', 'ELEKTRO', 'HOUSE'];
const MAX_STEPS = 16;
const MAX_TRACKS = 8;
const MAX_PATTERNS = 16;

let currentPattern = 0;
let currentKit = 0;
let currentStep = 0;
let selectedTrack = 0;
let tempo = 120;
let isPlaying = false;
let patterns = [];
let updateInterval = null;
let syncEnabled = true;

// ============================================
// INICIALIZACIÓN
// ============================================
document.addEventListener('DOMContentLoaded', () => {
    initializePatterns();
    buildSequencerGrid();
    buildPatternButtons();
    attachEventListeners();
    startSyncLoop();
    
    console.log('RED808 Web Interface Loaded');
});

// Inicializar matriz de patterns vacía
function initializePatterns() {
    patterns = Array(MAX_PATTERNS).fill(null).map(() => 
        Array(MAX_TRACKS).fill(null).map(() => 
            Array(MAX_STEPS).fill(false)
        )
    );
}

// ============================================
// CONSTRUCCIÓN DE UI
// ============================================
function buildSequencerGrid() {
    const grid = document.getElementById('sequencerGrid');
    grid.innerHTML = '';
    
    for (let track = 0; track < MAX_TRACKS; track++) {
        for (let step = 0; step < MAX_STEPS; step++) {
            const cell = document.createElement('div');
            cell.className = 'step-cell';
            cell.dataset.track = track;
            cell.dataset.step = step;
            
            // Click para activar/desactivar step
            cell.addEventListener('click', () => toggleStep(track, step));
            
            grid.appendChild(cell);
        }
    }
}

function buildPatternButtons() {
    const grid = document.getElementById('patternGrid');
    grid.innerHTML = '';
    
    for (let i = 0; i < MAX_PATTERNS; i++) {
        const btn = document.createElement('button');
        btn.className = 'pattern-btn';
        btn.textContent = (i + 1).toString().padStart(2, '0');
        btn.dataset.pattern = i;
        
        btn.addEventListener('click', () => changePattern(i));
        
        if (i === currentPattern) {
            btn.classList.add('active');
        }
        
        grid.appendChild(btn);
    }
}

// ============================================
// EVENT LISTENERS
// ============================================
function attachEventListeners() {
    // Play/Stop
    document.getElementById('btnPlay').addEventListener('click', () => {
        sendCommand('/play');
    });
    
    document.getElementById('btnStop').addEventListener('click', () => {
        sendCommand('/stop');
    });
    
    // Tempo
    document.getElementById('btnTempoUp').addEventListener('click', () => {
        sendCommand('/tempo', { delta: 5 });
    });
    
    document.getElementById('btnTempoDown').addEventListener('click', () => {
        sendCommand('/tempo', { delta: -5 });
    });
    
    // Kit buttons
    document.querySelectorAll('.kit-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const kit = parseInt(btn.dataset.kit);
            sendCommand('/kit', { kit: kit });
        });
    });
    
    // Tab buttons (visual only)
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
        });
    });
}

// ============================================
// STEP CONTROL
// ============================================
function toggleStep(track, step) {
    const isActive = patterns[currentPattern][track][step];
    patterns[currentPattern][track][step] = !isActive;
    
    // Actualizar visualmente
    updateStepCell(track, step, !isActive);
    
    // Enviar al ESP32
    sendCommand('/step', {
        pattern: currentPattern,
        track: track,
        step: step,
        value: !isActive ? 1 : 0
    });
}

function updateStepCell(track, step, active) {
    const cell = document.querySelector(`.step-cell[data-track="${track}"][data-step="${step}"]`);
    if (cell) {
        if (active) {
            cell.classList.add('active');
        } else {
            cell.classList.remove('active');
        }
    }
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
    
    // Actualizar grid
    updateSequencerFromPattern();
    
    // Actualizar footer
    document.getElementById('footerPattern').textContent = patternNum + 1;
}

function updateSequencerFromPattern() {
    for (let track = 0; track < MAX_TRACKS; track++) {
        for (let step = 0; step < MAX_STEPS; step++) {
            const active = patterns[currentPattern][track][step];
            updateStepCell(track, step, active);
        }
    }
}

// ============================================
// COMUNICACIÓN CON ESP32
// ============================================
async function sendCommand(endpoint, data = {}) {
    try {
        const url = endpoint + (Object.keys(data).length > 0 ? '?' + new URLSearchParams(data) : '');
        const response = await fetch(url, { method: 'GET' });
        
        if (!response.ok) {
            console.error('Command failed:', endpoint);
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
        
        // Actualizar UI con datos del ESP32
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
        document.getElementById('bpmDisplay').textContent = data.bpm;
        document.getElementById('tempoValue').textContent = data.bpm;
    }
    
    // Pattern
    if (data.pattern !== undefined && data.pattern !== currentPattern) {
        currentPattern = data.pattern;
        document.querySelectorAll('.pattern-btn').forEach((btn, idx) => {
            btn.classList.toggle('active', idx === data.pattern);
        });
        document.getElementById('footerPattern').textContent = data.pattern + 1;
    }
    
    // Kit
    if (data.kit !== undefined) {
        currentKit = data.kit;
        document.getElementById('kitNumDisplay').textContent = data.kit + 1;
        
        if (data.kitName) {
            document.getElementById('kitName').textContent = data.kitName;
        }
        
        // Actualizar botones de kit
        document.querySelectorAll('.kit-btn').forEach((btn, idx) => {
            btn.classList.toggle('active', idx === data.kit);
        });
    }
    
    // Playing state
    if (data.playing !== undefined) {
        isPlaying = data.playing;
        const btnPlay = document.getElementById('btnPlay');
        btnPlay.classList.toggle('active', data.playing);
        btnPlay.textContent = data.playing ? '⏸' : '▶';
    }
    
    // Current Step
    if (data.step !== undefined) {
        updatePlayingStep(data.step);
        currentStep = data.step;
        document.getElementById('footerStep').textContent = data.step + 1;
        
        // Actualizar display de 7 segmentos
        const stepDisplay = (data.step + 1).toString().padStart(3, '0');
        const digits = document.querySelectorAll('.seven-seg-large .digit');
        digits[0].textContent = stepDisplay[0];
        digits[1].textContent = stepDisplay[1];
        digits[2].textContent = stepDisplay[2];
    }
    
    // Track
    if (data.track !== undefined) {
        selectedTrack = data.track;
        document.getElementById('footerTrack').textContent = data.track + 1;
        document.getElementById('trackNum').textContent = (data.track + 1).toString().padStart(2, '0');
        document.getElementById('instrumentName').textContent = TRACK_NAMES[data.track] || 'TRACK';
    }
    
    // Pattern data (si está disponible)
    if (data.patternData) {
        patterns[currentPattern] = data.patternData;
        updateSequencerFromPattern();
    }
    
    // Tiempo transcurrido
    if (data.time !== undefined) {
        document.getElementById('timeDisplay').textContent = formatTime(data.time);
    }
}

function updatePlayingStep(step) {
    // Remover highlight anterior
    document.querySelectorAll('.step-cell.playing').forEach(cell => {
        cell.classList.remove('playing');
    });
    
    // Agregar highlight al step actual
    if (isPlaying) {
        document.querySelectorAll(`.step-cell[data-step="${step}"]`).forEach(cell => {
            cell.classList.add('playing');
        });
    }
}

function updateConnectionStatus(connected) {
    const status = document.getElementById('status');
    if (connected) {
        status.style.background = '#0f0';
        status.style.boxShadow = '0 0 15px #0f0';
    } else {
        status.style.background = '#f00';
        status.style.boxShadow = '0 0 15px #f00';
    }
}

// ============================================
// SYNC LOOP - Actualización automática
// ============================================
function startSyncLoop() {
    // Fetch inicial
    fetchStatus();
    
    // Actualizar cada 200ms para sincronización fluida
    updateInterval = setInterval(() => {
        if (syncEnabled) {
            fetchStatus();
        }
    }, 200);
}

function stopSyncLoop() {
    if (updateInterval) {
        clearInterval(updateInterval);
        updateInterval = null;
    }
}

// ============================================
// UTILITIES
// ============================================
function formatTime(seconds) {
    const mins = Math.floor(seconds / 60);
    const secs = Math.floor(seconds % 60);
    const decisecs = Math.floor((seconds % 1) * 10);
    return `${mins}:${secs.toString().padStart(2, '0')}:${decisecs}`;
}

// Pause sync cuando la página no está visible (ahorro de recursos)
document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
        syncEnabled = false;
    } else {
        syncEnabled = true;
        fetchStatus(); // Actualizar inmediatamente al volver
    }
});

// Cleanup
window.addEventListener('beforeunload', () => {
    stopSyncLoop();
});
