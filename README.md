# Proyecto ESP32 con PlatformIO

Proyecto para **ESP32 Dev Module** usando el framework Arduino con PlatformIO.

## Requisitos

- [Visual Studio Code](https://code.visualstudio.com/)
- [Extensión PlatformIO IDE](https://platformio.org/install/ide?install=vscode)

## Estructura del Proyecto

```
XboxBLE/
├── .github/
│   └── copilot-instructions.md
├── include/           # Archivos de cabecera (.h)
├── lib/              # Librerías privadas
├── src/              # Código fuente
│   └── main.cpp      # Archivo principal
├── test/             # Tests unitarios
├── platformio.ini    # Configuración del proyecto
└── README.md         # Este archivo
```

## Configuración

El archivo `platformio.ini` está configurado para:
- **Plataforma**: Espressif32
- **Placa**: ESP32 Dev Module
- **Framework**: Arduino
- **Velocidad del monitor serial**: 115200

## Compilar el Proyecto

### Desde VS Code:
1. Abre la paleta de comandos (Ctrl+Shift+P)
2. Busca "PlatformIO: Build"
3. O usa el ícono de ✓ en la barra inferior

### Desde terminal:
```bash
platformio run
```

## Subir el Código al ESP32

### Desde VS Code:
1. Conecta tu ESP32 al puerto USB
2. Abre la paleta de comandos (Ctrl+Shift+P)
3. Busca "PlatformIO: Upload"
4. O usa el ícono de → en la barra inferior

### Desde terminal:
```bash
platformio run --target upload
```

## Monitor Serial

Para ver la salida del ESP32:

### Desde VS Code:
- Usa el ícono de 🔌 en la barra inferior

### Desde terminal:
```bash
platformio device monitor
```

## Código de Ejemplo

El proyecto incluye un código básico "Hello World" que:
- Inicializa la comunicación serial a 115200 baudios
- Hace parpadear el LED integrado cada segundo
- Imprime mensajes en el monitor serial

## Solución de Problemas

### El ESP32 no se detecta
- Verifica que el cable USB transmita datos (no solo carga)
- Instala los drivers USB-Serial (CP210x o CH340)
- Verifica el puerto COM en el administrador de dispositivos

### Error de compilación
- Limpia el proyecto: `platformio run --target clean`
- Elimina la carpeta `.pio` y vuelve a compilar

## Recursos Adicionales

- [Documentación PlatformIO](https://docs.platformio.org/)
- [Referencia ESP32 Arduino](https://docs.espressif.com/projects/arduino-esp32/)
- [Ejemplos ESP32](https://github.com/espressif/arduino-esp32/tree/master/libraries)
