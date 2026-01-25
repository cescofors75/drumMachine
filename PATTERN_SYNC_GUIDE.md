# Guía de Sincronización de Patrones MASTER → SLAVE

## 🎯 Problema Resuelto

El SLAVE (XboxBLE Surface) no mostraba los patrones del MASTER visualmente. Ahora se ha implementado:

✅ **Auto-sincronización** al conectarse al MASTER  
✅ **Indicador visual** en pantalla TFT  
✅ **Botón SYNC** en la pantalla Sequencer  
✅ **Atajo de teclado** para forzar sincronización  
✅ **Debugging mejorado** en Serial Monitor  

---

## 📡 Cómo Funciona la Sincronización

### 1. Automática al Conectar WiFi

Cuando el SLAVE se conecta al MASTER:
```
1. Conecta al WiFi "RED808"
2. Envía comando "hello" al MASTER
3. Automáticamente solicita el patrón actual
4. Recibe respuesta "pattern_sync"
5. Actualiza la matriz local 16x16
6. Redibuja la pantalla
```

### 2. Automática al Entrar a SCREEN_SEQUENCER

Cada vez que entras a la pantalla del Sequencer:
```cpp
changeScreen(SCREEN_SEQUENCER);
// → Auto-solicita patrón actual del MASTER
```

### 3. Manual - Atajo de Teclado

**ENCODER + BACK** (mantener encoder presionado + pulsar BACK):
- Solo funciona en la pantalla SEQUENCER
- Solicita el patrón actual del MASTER
- Muestra notificación en pantalla

### 4. Visual - Botón SYNC en Pantalla

En la pantalla SEQUENCER (arriba a la derecha):
- **Botón amarillo "SYNC MASTER"** - UDP conectado y listo
- **Botón gris "SYNC NO CON"** - Sin conexión UDP

---

## 🔍 Debugging en Serial Monitor

### Al Solicitar Patrón

```
───────────────────────────────────────
► REQUESTING Pattern 1 from MASTER
   Master IP: 192.168.4.1:8888
   Waiting for response...
───────────────────────────────────────
```

### Al Recibir Patrón

```
═══════════════════════════════════════
► PATTERN SYNC RECEIVED: Pattern 1
═══════════════════════════════════════
► Total active steps: 64

=== PATTERN 1 RECEIVED === (16x16) ===
    1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16
T01 ■  ·  ·  ·  ■  ·  ·  ·  ■  ·  ·  ·  ■  ·  ·  · 
T02 ·  ·  ·  ·  ■  ·  ·  ·  ·  ·  ·  ·  ■  ·  ·  · 
T03 ■  ·  ■  ·  ■  ·  ■  ·  ■  ·  ■  ·  ■  ·  ■  · 
...
► Screen will be redrawn on next loop
✓ Pattern synchronized successfully!
═══════════════════════════════════════
```

### Notificación en Pantalla TFT

```
┌──────────────────────────────────────┐
│ SYNCED Pattern 1 from MASTER         │
└──────────────────────────────────────┘
```

---

## 🛠️ Comandos UDP Relacionados

### Solicitar Patrón (SLAVE → MASTER)

```json
{
  "cmd": "get_pattern",
  "pattern": 0
}
```

### Respuesta con Patrón (MASTER → SLAVE)

```json
{
  "cmd": "pattern_sync",
  "pattern": 0,
  "data": [
    [1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0],
    [0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0],
    [1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0],
    ...16 tracks total...
  ]
}
```

---

## 🎨 Indicadores Visuales

### En TFT (Pantalla Sequencer)

1. **Botón SYNC** (arriba derecha):
   - 🟨 Amarillo: Conectado y listo
   - ⬜ Gris: No hay conexión

2. **Notificación temporal** (parte inferior):
   - 🟡 "Requesting Pattern X..." (mientras espera)
   - 🟢 "SYNCED Pattern X from MASTER" (éxito)
   - 🔴 "ERROR: Not connected to MASTER" (fallo)

3. **Grid actualizado**:
   - Los steps se actualizan con los colores del instrumento
   - Si hay steps activos, se muestran como cuadrados llenos (■)
   - Steps vacíos se muestran oscuros (·)

### En Serial Monitor

```
► ENCODER+BACK = REQUESTING PATTERN SYNC!
```

---

## 📋 Checklist de Verificación

Para confirmar que la sincronización funciona:

- [ ] El SLAVE se conecta al WiFi "RED808"
- [ ] Aparece "✓ WiFi connected!" en Serial
- [ ] Se ejecuta "Auto-requesting pattern from MASTER..."
- [ ] Aparece "PATTERN SYNC RECEIVED" en Serial
- [ ] Se imprime la matriz 16x16 con ■ y ·
- [ ] La pantalla TFT muestra los steps actualizados
- [ ] Los LEDs TM1638 reflejan el patrón del track seleccionado

---

## ⚠️ Solución de Problemas

### No se recibe el patrón

1. **Verificar conexión WiFi**:
   ```
   ► WiFi status: WL_CONNECTED
   ► IP Address: 192.168.4.X
   ```

2. **Verificar que el MASTER está activo**:
   - IP: 192.168.4.1
   - Puerto UDP: 8888
   - SSID: RED808

3. **Forzar sincronización manual**:
   - Presionar **ENCODER + BACK** en pantalla SEQUENCER
   - O entrar/salir de SCREEN_SEQUENCER

4. **Ver debug UDP**:
   ```
   [UDP] Received 2048 bytes from 192.168.4.1:8888
   [UDP] Data: {"cmd":"pattern_sync",...}
   ```

### El patrón se recibe pero no se ve en pantalla

1. **Verificar que estás en SCREEN_SEQUENCER**
2. **Forzar redibujado**:
   ```cpp
   needsFullRedraw = true;
   ```
3. **Verificar que el patrón tiene steps activos**:
   ```
   ► Total active steps: 64  ← Debe ser > 0
   ```

---

## 🎹 Uso Típico

### Escenario 1: Inicio desde Cero

```
1. Encender MASTER (DrumMachine)
2. Encender SLAVE (XboxBLE Surface)
3. SLAVE se conecta automáticamente
4. Patrón se sincroniza en <1 segundo
5. Ver patrón en pantalla y TM1638
```

### Escenario 2: Cambio de Patrón en MASTER

```
1. MASTER cambia al Pattern 2
2. SLAVE no se actualiza automáticamente*
3. En SLAVE: presionar ENCODER+BACK
4. Patrón 2 se sincroniza
```

*Nota: Para sincronización automática en cada cambio de patrón, el MASTER debe broadcast el evento.

### Escenario 3: Debugging

```
1. Abrir Serial Monitor (115200 baud)
2. Verificar "WiFi connected!"
3. Ir a SCREEN_SEQUENCER
4. Presionar ENCODER+BACK
5. Ver debug completo en Serial
```

---

## 📝 Notas Técnicas

- **Buffer UDP**: 2048 bytes (suficiente para matriz 16x16)
- **Timeout**: 500ms (ajustable)
- **Auto-retry**: No implementado (solicitar manualmente)
- **Formato JSON**: Compacto sin espacios
- **Latencia típica**: 50-100ms
- **Max patrones**: 16 (0-15)

---

## 🔗 Referencias

- [UDP_CONTROL_EXAMPLES copy.md](UDP_CONTROL_EXAMPLES%20copy.md) - Documentación completa del protocolo UDP
- [src/main.cpp](src/main.cpp) - Líneas 666-705 (requestPatternFromMaster y receivePatternSync)
- [src/main.cpp](src/main.cpp) - Líneas 609-650 (handler de pattern_sync)

---

**Última actualización**: 25/01/2026
**Versión**: RED808 V6 SLAVE
