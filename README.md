
![JCBExpander Interface](Assets/screenshot.png)

Plugin expansor de audio desarrollado en gen~, exportado con [gen~ Plugin Export](https://github.com/Cycling74/gen-plugin-export) y finalizado con el framework C++ [JUCE](https://github.com/juce-framework/JUCE). Este plugin forma parte de un conjunto de herramientas didácticas que utilizo en la asignatura de Técnicas de Grabación y Masterización para Música Electroacústica del [MCE](https://katarinagurska.com/curso-of/master-de-composicion-electroacustica-mce/). Originalmente creado con JUCE 6 hace unos años, el proyecto ha evolucionado significativamente en su interfaz gráfica y funcionalidad gracias al desarrollo con Claude Code durante junio de 2025. Para más detalles técnicos, consulta [NOTAS.md](NOTAS.md).

## Instalación macOS
1. Descarga el archivo DMG desde la página de [Releases](https://github.com/cjitter/JCBExpander/releases)
2. Abre el DMG y ejecuta el instalador
3. El instalador colocará automáticamente los tipos de plugins seleccionados en las ubicaciones correctas del sistema:
   - VST3: `/Library/Audio/Plug-Ins/VST3/`
   - AU: `/Library/Audio/Plug-Ins/Components/`
   - AAX: `/Library/Application Support/Avid/Audio/Plug-Ins/`

*Nota: El DMG está firmado y notarizado para macOS.*

## Requisitos del sistema

- macOS 10.12 o posterior
- Procesador Intel o Apple Silicon
- DAW compatible con VST3, AU o AAX
- Probado en: Pro Tools, Reaper, Logic, Ableton Live y Bitwig

## Compilación desde código fuente

### Requisitos previos
- Git, [CMake](https://cmake.org) 3.20 o posterior, compilador Apple Clang compatible con C++20 (incluido en Xcode 13 o superior).
- [JUCE](https://github.com/juce-framework/JUCE) 8.0.8 (se descarga automáticamente via FetchContent)
- **AAX SDK**, solo requerido para compilar formato AAX - disponible desde cuenta de desarrollador Avid, etc.

### Instrucciones de compilación

1. Clona el repositorio:
```bash
git clone https://github.com/cjitter/JCBExpander.git
cd JCBExpander
```

2. **Configuración del proyecto**:
```bash
# Configurar proyecto Debug (VST3 y AU)
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug

# Configurar proyecto Release (VST3 y AU)
cmake -B build-release -DCMAKE_BUILD_TYPE=Release

# Configurar proyecto Debug/Release con AAX (requiere AAX SDK y Pro Tools Developer instalado)
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug -DJUCE_BUILD_AAX=ON
```

**Nota sobre generadores**: Por defecto, CMake usa Unix Makefiles en macOS. También puedes especificar otros generadores:
- **Ninja** (más rápido): `cmake -B build -G Ninja`
- **Xcode** (IDE nativo): `cmake -B build -G Xcode`

**Nota sobre AAX**: Si configuras con la opción AAX habilitada, cuando compiles el plugin se instalará automáticamente en `/Applications/Pro Tools Developer/Plug-Ins/` para testing con Pro Tools Developer.

3. **Compilación**:
```bash
# Compilar el proyecto configurado
cmake --build build-debug    # Para Debug
cmake --build build-release   # Para Release
```

## Características principales

- **Ajuste de entrada** (trim ±12 dB) para ambas cadenas de audio y sidechain.
- **Procesamiento sidechain** interno y externo con filtros paso alto y bajo, 2º orden (12/24 dB/oct).
- **Modo de detección** sharp con sliding RMS de ventana variable y control adicional de suavizado.
- **Ajuste interpolado independiente de reacción pico/RMS**.
- **Expansión con softknee** de segundo orden con transición gradual (1:1 a 6:1).
- **Controles estándar**: threshold (-60 a 0 dB), range (-100 a 0 dB), ratio (1:1 a 20:1), knee (1 a 10 dB), attack (0.1 a 250 ms), hold (0 a 250 ms), release (0.1 a 1000 ms).
- **Control de rango** (-100 a 0 dB) para limitar la expansión máxima.
- **Modo Delta para monitorización** de la señal diferencia (entrada vs salida).
- **Softclipping asimétrico**, aplicado tras expansión y makeup gain.
- **Control Dry/Wet (0 a 100%)** para mezcla de señal procesada.
- **Procesamiento mono, mono-estéreo y estéreo**, con detección de envolvente compartida.
- **Sistema lookahead** (0 a 10 ms) para procesamiento predictivo.
- **Visualización gráfica**: medidores de entrada, salida y reducción de ganancia.
- **Gestión de presets** (usuario y fábrica).
- **Monitorización**: solo sidechain y delta.
- **Bypass interno**, independiente del DAW.
- **Formatos disponibles**: VST3, AU y AAX.
- **Visualización del diagrama de bloques**: Acceso al diagrama completo del expansor con posibilidad de explorar cada bloque y copiar el código GenExpr para usar directamente en objetos Codebox o gen.codebox~ (Max 9).

![Diagrama de Bloques](Assets/screenshotDiagram.png)

## Recursos

### Bibliografía técnica
- [Graham Wakefield & Gregory Taylor - *Generating Sound and Organizing Time*](https://cycling74.com/books/go)
- [Will C. Pirkle - *Designing Audio Effect Plugins in C++*](https://www.willpirkle.com)
- [Giannoulis, Massberg, Reiss - *Dynamic Range Compressor Design*](https://eecs.qmul.ac.uk/~josh/documents/2012/GiannoulisMassbergReiss-dynamicrangecompression-JAES2012.pdf)
- [Matthijs Hollemans - *The Complete Beginner's Guide to Audio Plug-in Development*](https://www.theaudioprogrammer.com/books/beginners-plugin-book)

## Testing con pluginval

Este proyecto incluye tests automáticos con [pluginval](https://github.com/Tracktion/pluginval) para validar el plugin.

### Configuración

- **Detección automática**: Si tienes pluginval instalado, CMake lo detectará automáticamente
- **Descarga automática**: Si no está instalado, se descargará automáticamente en la carpeta `tools/`
- Para desactivar la descarga automática:
  ```bash
  cmake -B build -DPLUGINVAL_AUTO_DOWNLOAD=OFF
  ```

### Ejecutar los tests

```bash
# Entra al directorio de build (el mismo nombre que usaste después de -B al configurar)
# Por ejemplo: si usaste "cmake -B build", entonces:
cd build

# Ejecutar todos los tests de pluginval con salida detallada
# Usa -C Release o -C Debug según cómo compilaste el plugin
# Nota: Los tests completos pueden tardar varios minutos
ctest -C Release -R pluginval -V
```

Los tests validan los formatos VST3 y AU del plugin.

## Por hacer

- Migrar de Plugin Export a la exportación C++ de RNBO.
- Implementar oversampling con el módulo DSP de JUCE.
- Implementar modos dual y M/S.
- Implementar mapeo MIDI.
- Mejorar sistema de visualización entrada/salida e histograma de reducción de ganancia.
- Portar a SuperCollider los bloques de GenExpr, y/o crear UGen.
- Hacer el Max For Live device.

---

*© 2025 Juan Carlos Blancas – JCBExpander v0.9.1 beta*
