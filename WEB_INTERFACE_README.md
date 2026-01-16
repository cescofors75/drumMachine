# 🌐 RED808 Web Interface - Instrucciones de Instalación

## 📦 **Archivos Creados**

### Estructura del Proyecto:
```
XboxBLE/
├── data/                    ← Archivos para LittleFS
│   ├── index.html          ← Interfaz principal
│   ├── style.css           ← Estilos profesionales
│   └── script.js           ← JavaScript de sincronización
├── src/
│   └── main.cpp            ← Código actualizado con servidor web
└── platformio.ini          ← Configuración de LittleFS
```

---

## 🚀 **Pasos para Subir al ESP32**

### **1. Compilar el Firmware**
```powershell
pio run
```

### **2. Subir el Filesystem (LittleFS) con los archivos HTML/CSS/JS**
```powershell
pio run --target uploadfs
```

### **3. Subir el Código al ESP32**
```powershell
pio run --target upload
```

### **4. Abrir Monitor Serial (opcional)**
```powershell
pio device monitor
```

---

## 📱 **Cómo Acceder a la Interfaz Web**

1. **Conectar a WiFi:**
   - SSID: `RED808`
   - Password: `12345678`

2. **Abrir navegador web:**
   - URL: **http://192.168.4.1**

3. **¡Listo!** Verás la interfaz profesional completa

---

## 🎛️ **Características de la Interfaz Web**

### ✅ **Control Bidireccional:**
- ▶️ **Play/Stop** desde el navegador
- 🎵 **Cambiar BPM** (+/- 5 BPM por click)
- 🔢 **Seleccionar Pattern** (1-16)
- 🥁 **Seleccionar Kit** (1-3)
- 🎹 **Click en Steps** para activar/desactivar (16x8 grid)

### 📊 **Visualización en Tiempo Real:**
- Display de 7 segmentos virtual
- Step actual parpadeante
- Estado PLAYING/STOPPED
- Nombre del instrumento actual
- Colores por track (KICK=rojo, SNARE=naranja, etc.)

### 🔄 **Sincronización:**
- Actualización automática cada 200ms
- Los cambios en el hardware se reflejan en la web
- Los cambios en la web se reflejan en el hardware

---

## 🎨 **Diseño Profesional**

La interfaz replica el aspecto de la RED808 física:
- ⚫ Fondo oscuro con LEDs brillantes
- 🔴 Tema rojo corporativo
- 📟 Display de 7 segmentos simulado
- 🎛️ Botones realistas con gradientes
- 📱 Responsive (funciona en móvil/tablet/PC)

---

## 🔧 **Troubleshooting**

### **Error: "LittleFS mount failed"**
- Ejecutar: `pio run --target uploadfs`
- Verificar que existan los archivos en `data/`

### **No puedo conectarme a RED808**
- Verificar que el ESP32 esté encendido
- Esperar 10-15 segundos después del reset
- IP por defecto: **192.168.4.1**

### **La página no carga**
- Verificar que se subió el filesystem (`uploadfs`)
- Abrir monitor serial para ver errores
- Probar en modo incógnito del navegador

### **Los cambios no se sincronizan**
- Verificar conexión WiFi estable
- Abrir DevTools (F12) → Console para ver errores JS
- El punto verde debe estar encendido (conexión activa)

---

## 📋 **Endpoints API Disponibles**

| Endpoint | Método | Parámetros | Descripción |
|----------|--------|------------|-------------|
| `/` | GET | - | Interfaz web completa |
| `/status` | GET | - | Estado actual (JSON) |
| `/play` | GET | - | Iniciar playback |
| `/stop` | GET | - | Detener playback |
| `/toggle` | GET | - | Play/Stop toggle |
| `/tempo` | GET | `delta` o `value` | Cambiar BPM |
| `/pattern` | GET | `pattern` (0-15) | Cambiar pattern |
| `/kit` | GET | `kit` (0-2) | Cambiar kit |
| `/step` | GET | `track`, `step`, `value` | Toggle step |
| `/getpattern` | GET | - | Obtener pattern completo |

### Ejemplos:
```
http://192.168.4.1/tempo?delta=10       → Aumentar 10 BPM
http://192.168.4.1/pattern?pattern=5    → Cambiar a Pattern 6
http://192.168.4.1/step?track=0&step=4&value=1  → Activar KICK en step 5
```

---

## 💡 **Optimización de Recursos**

Para desactivar el servidor web y ahorrar RAM:

En `main.cpp`, comentar la línea:
```cpp
// setupWebServer();  // Desactivar WiFi
```

---

## 📝 **Notas Importantes**

1. **Consumo de RAM:** El servidor web usa ~40KB de RAM
2. **WiFi Power:** Para máxima vida de batería, desactivar WiFi cuando no se use
3. **Actualización:** Los archivos `data/` pueden editarse y re-subirse sin recompilar
4. **Compatibilidad:** Funciona en Chrome, Firefox, Safari, Edge

---

## 🎉 **¡Listo para Usar!**

Ahora tienes una interfaz web profesional completamente funcional para controlar tu RED808 desde cualquier dispositivo.

**Desarrollado con ❤️ para RED808 V5**
