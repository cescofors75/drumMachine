# CHANGELOG - Corrección de Volumen y Sequencer

## Fecha: 25/01/2026

## 🔴 PROBLEMAS RESUELTOS

### 1. ❌ Doble Sequencer (CRÍTICO)
**Problema**: Al presionar PLAY en el SLAVE, se activaba un sequencer local que sonaba junto con el del MASTER, causando desfase horrible.

**Solución**: 
- ✅ Eliminado el sequencer de audio local del SLAVE
- ✅ El SLAVE ahora solo:
  - Envía comandos UDP al MASTER
  - Actualiza visualización (LEDs, TFT)
  - Sincroniza step actual vía UDP

```cpp
// ANTES (MALO - reproducía audio localmente)
void updateSequencer() {
    if (currentTime - lastStepTime >= stepInterval) {
        for (int track = 0; track < MAX_TRACKS; track++) {
            if (pattern.steps[track][currentStep]) {
                triggerDrum(track);  // ❌ Enviaba trigger al MASTER
            }
        }
        currentStep = (currentStep + 1) % MAX_STEPS;  // ❌ Avanzaba step localmente
    }
}

// AHORA (BUENO - solo visualización)
void updateSequencer() {
    // Solo actualiza LEDs basándose en currentStep sincronizado del MASTER
    updateStepLEDs();
}
```

---

### 2. 🎚️ Sistema de Volumen Dual

**Problema**: 
- MASTER tenía 2 volúmenes (Sequencer 0-100%, Live Pads 0-100%)
- SLAVE solo tenía 1 volumen (0-30)
- No había forma de controlar ambos desde el SLAVE

**Solución**:
- ✅ **Volúmenes separados**: Sequencer Volume y Live Pads Volume
- ✅ **Rango unificado**: 0-100% en todos los dispositivos
- ✅ **Botón Toggle** (pin 14): Alterna entre controlar SEQ o PAD
- ✅ **Indicador visual**: Color verde en TFT y TM1638 muestra modo activo
- ✅ **Comandos UDP correctos**: `setSequencerVolume` / `setLiveVolume`

#### Pin 14 - Volume Toggle Button
```cpp
#define VOLUME_TOGGLE_BTN 14  // Nueva definición

// Modo de volumen
enum VolumeMode {
    VOL_SEQUENCER,    // Controla volumen del sequencer
    VOL_LIVE_PADS     // Controla volumen de live pads
};

// Variables
int sequencerVolume = 50;
int livePadsVolume = 50;
VolumeMode volumeMode = VOL_SEQUENCER;
```

#### Funcionamiento
1. **Potenciómetro** (pin 35): Ajusta el volumen (0-100%)
2. **Botón pin 14**: Alterna entre modo SEQ ↔ PAD
3. **Display TM1638**: Muestra "SEQ 75%" o "PAD 80%"
4. **Header TFT**: 
   - `SEQ:75%` (verde si activo, gris si no)
   - `PAD:80%` (verde si activo, gris si no)

---

## 📡 COMANDOS UDP MODIFICADOS

### Envío de Volumen (SLAVE → MASTER)

```json
// Volumen del Sequencer
{"cmd":"setSequencerVolume","value":75}

// Volumen de Live Pads
{"cmd":"setLiveVolume","value":80}
```

### Sincronización de Volumen (MASTER → SLAVE)

```json
// Sync volumen sequencer
{"cmd":"volume_seq_sync","value":75}

// Sync volumen live pads
{"cmd":"volume_live_sync","value":80}
```

---

## 🎨 INTERFAZ ACTUALIZADA

### TFT Header (Superior)
```
┌──────────────────────────────────────────────┐
│ R808  120 BPM  SEQ:75%  PAD:80%  P1  BD      │
│                 ^^^^      ^^^^                │
│               (verde)   (gris)                │
└──────────────────────────────────────────────┘
```
- Verde = Modo activo (controlándose ahora)
- Gris = Modo inactivo

### TM1638 Display
```
┌─────────┬─────────┐
│ SEQ 75% │         │  ← Modo Sequencer activo
└─────────┴─────────┘

┌─────────┬─────────┐
│ PAD 80% │         │  ← Modo Live Pads activo
└─────────┴─────────┘
```

### Serial Monitor
```
► Volume Mode: SEQUENCER
► Sequencer Volume: 75%

[Presionar botón pin 14]

► Volume Mode: LIVE PADS
► Live Pads Volume: 80%
```

---

## 🔧 PINES UTILIZADOS

| Pin | Función | Tipo | Notas |
|-----|---------|------|-------|
| 35 | Potenciómetro Volumen | Analog Input | 0-4095 → 0-100% |
| 14 | Toggle SEQ/PAD | Digital Input (Pull-up) | Nuevo botón |
| 34 | 3 Botones Analógicos | Analog Input | PLAY/STOP, MUTE, BACK |

---

## 📝 FLUJO DE CONTROL

### Escenario 1: Ajustar Volumen del Sequencer
```
1. Usuario gira potenciómetro
2. SLAVE lee ADC (0-4095)
3. Mapea a 0-100%
4. Si volumeMode == VOL_SEQUENCER:
   - Envía UDP: {"cmd":"setSequencerVolume","value":75}
   - Actualiza TM1638: "SEQ 75%"
   - Actualiza header TFT
5. MASTER recibe y ajusta su sequencer
```

### Escenario 2: Cambiar a Control de Live Pads
```
1. Usuario presiona botón pin 14
2. volumeMode cambia: VOL_SEQUENCER → VOL_LIVE_PADS
3. TM1638 muestra: "PAD VOL"
4. Ahora el potenciómetro controla Live Pads
5. Al girar: {"cmd":"setLiveVolume","value":80}
```

### Escenario 3: Presionar PLAY
```
ANTES (MALO):
1. Usuario presiona PLAY
2. isPlaying = true
3. updateSequencer() reproduce audio local ❌
4. MASTER también reproduce ❌
5. Resultado: Doble sonido con desfase 💀

AHORA (BUENO):
1. Usuario presiona PLAY
2. Envía UDP: {"cmd":"start"}
3. isPlaying = true (solo para UI)
4. updateSequencer() solo actualiza LEDs ✅
5. MASTER reproduce audio ✅
6. Resultado: Un solo sonido limpio 🎵
```

---

## ⚙️ CONFIGURACIÓN REQUERIDA

### En el MASTER (DrumMachine)
Debe responder a:
- `setSequencerVolume` (valor 0-100)
- `setLiveVolume` (valor 0-100)
- `start` / `stop`
- `trigger` (para live pads)
- `get_pattern` (para sincronización)

Debe broadcast:
- `volume_seq_sync` (cuando cambia volumen sequencer)
- `volume_live_sync` (cuando cambia volumen live pads)
- `step_sync` (step actual del sequencer)
- `tempo_sync` (cuando cambia BPM)

### En el SLAVE (XboxBLE Surface)
- Pin 14: Conectar botón pulsador (GND cuando presionado)
- Pin 35: Potenciómetro 10kΩ (0-3.3V)
- WiFi: Conectado a "RED808" (192.168.4.1)

---

## 🧪 TESTING

### Test 1: Volumen Sequencer
```
1. Encender SLAVE
2. Modo por defecto: VOL_SEQUENCER
3. Girar potenciómetro
4. Verificar en Serial: "► Sequencer Volume: X%"
5. Verificar en TM1638: "SEQ X%"
6. Verificar que MASTER cambia volumen
```

### Test 2: Volumen Live Pads
```
1. Presionar botón pin 14
2. Verificar Serial: "► Volume Mode: LIVE PADS"
3. Girar potenciómetro
4. Verificar: "► Live Pads Volume: X%"
5. Verificar TM1638: "PAD X%"
6. Tocar pad → verificar volumen diferente al sequencer
```

### Test 3: No Doble Sequencer
```
1. Presionar PLAY en SLAVE
2. Verificar que solo suena UNA vez (del MASTER)
3. NO debe haber desfase ni eco
4. LEDs TM1638 deben sincronizar con MASTER
```

---

## 📊 ESTADÍSTICAS

- **Código eliminado**: ~15 líneas (sequencer local)
- **Código agregado**: ~80 líneas (dual volume)
- **Pines nuevos**: 1 (pin 14)
- **Comandos UDP nuevos**: 4 (setSequencerVolume, setLiveVolume, volume_seq_sync, volume_live_sync)
- **Rango volumen**: 0-30 → 0-100% (+233% precisión)

---

## 🐛 BUGS CONOCIDOS

- ✅ Doble sequencer: RESUELTO
- ✅ Volumen 0-30: RESUELTO (ahora 0-100%)
- ✅ Un solo volumen: RESUELTO (ahora dual SEQ/PAD)

---

## 📚 ARCHIVOS MODIFICADOS

1. `src/main.cpp`
   - Línea ~55: Definición `VOLUME_TOGGLE_BTN`
   - Línea ~61: `MAX_VOLUME 100`
   - Línea ~297-305: Variables dual volume
   - Línea ~1346: Setup pin 14
   - Línea ~1540: updateSequencer() simplificado
   - Línea ~2030: handleVolume() con toggle
   - Línea ~2124: showVolumeOnTM1638() dual mode
   - Línea ~3115: drawHeader() con 2 volúmenes
   - Línea ~710: Recepción volume_sync

---

## 🎯 PRÓXIMOS PASOS (Opcional)

1. Agregar indicador LED en pin 14 (modo visual)
2. Guardar modo de volumen en EEPROM
3. Agregar preset de volúmenes (ej: "LOUD", "QUIET")
4. Sincronización automática de volúmenes al conectar

---

**¡SISTEMA CORREGIDO Y FUNCIONAL!** 🎉
