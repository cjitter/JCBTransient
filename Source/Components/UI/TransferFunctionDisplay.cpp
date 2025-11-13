//==============================================================================
//
//  Copyright 2025 Juan Carlos Blancas
//  This file is part of JCBTransient and is licensed under the GNU General Public License v3.0 or later.
//
//==============================================================================

//==============================================================================
// INCLUDES
//==============================================================================
#include "TransferFunctionDisplay.h"
#include <juce_core/juce_core.h>
#include <cmath>
#include "../../Helpers/UTF8Helper.h"

//==============================================================================
// CONSTRUCTOR
//==============================================================================

TransferFunctionDisplay::TransferFunctionDisplay()
{
    setOpaque(false);  // Fondo translúcido para mejor integración visual
    
    // Inicializar buffers de waveform con valores por defecto (silencio)
    inputWaveformBuffer.resize(waveformBufferSize, -100.0f);       // Buffer entrada (-100dB = silencio)
    processedWaveformBuffer.resize(waveformBufferSize, -100.0f);   // Buffer salida procesada
    gainReductionBuffer.resize(waveformBufferSize, 0.0f);          // Buffer GR (0dB = sin reducción)
    attackGainBuffer.resize(waveformBufferSize, 0.0f);            // Buffer Attack gain (bipolar 0 = center)
    sustainGainBuffer.resize(waveformBufferSize, 0.0f);           // Buffer Sustain gain (bipolar 0 = center)
    
    // ELIMINADO: Inicialización de buffer DELTA - ya no se utiliza
    
    // Estado inicial: sin datos de audio disponibles
    hasWaveformData.store(false);
    isSilent.store(true, std::memory_order_relaxed);
}

//==============================================================================
// OVERRIDES DE COMPONENT
//==============================================================================

void TransferFunctionDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Fondo translúcido sutil
    g.setColour(DarkTheme::backgroundMedium.withAlpha(0.3f));
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Contorno sutil
    g.setColour(DarkTheme::borderHighlight.withAlpha(0.2f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), 4.0f, 1.0f);
    
    // Área de gráfico (con margen para labels)
    auto graphBounds = bounds.reduced(15.0f, 10.0f);
    
    // Dibujar elementos del gráfico
    drawGrid(g, graphBounds);
    // Mostrar contenido según el modo
    if (!soloSidechainActive && !bypassMode) {
        if (envelopeVisible) {
            // Mostrar waveforms (formas de onda de entrada y salida)
            drawWaveformAreas(g, graphBounds);
            
            // Mostrar histogramas Attack/Sustain superpuestos
            drawAttackSustainHistograms(g, graphBounds);
        }
    }
    
    // ELIMINADO: Solo mostrar envolventes y GR meter - sin transfer function
    // Los elementos de compresión (threshold, knee, transfer curve) han sido deshabilitados
    // según requerimientos del usuario para el transient shaper
    /*
    if (!bypassMode && !soloSidechainActive) {
        drawThresholdProjection(g, graphBounds);
        drawRangeProjection(g, graphBounds);
        drawKneeArea(g, graphBounds);
        drawTransferCurve(g, graphBounds);
    }
    */
}

void TransferFunctionDisplay::resized()
{
    // Actualizar curva cuando cambie el tamaño
    updateCurve();
}

//==============================================================================
// MÉTODOS DE ACTUALIZACIÓN DE PARÁMETROS
//==============================================================================

void TransferFunctionDisplay::setThreshold(float thresholdDb)
{
    if (threshold != thresholdDb)
    {
        threshold = thresholdDb;
        updateCurve();
    }
}

void TransferFunctionDisplay::setRatio(float newRatio)
{
    if (ratio != newRatio)
    {
        ratio = newRatio;
        updateCurve();
    }
}

void TransferFunctionDisplay::setKnee(float kneeDb)
{
    if (knee != kneeDb)
    {
        knee = kneeDb;
        updateCurve();
    }
}

void TransferFunctionDisplay::setRange(float rangeDb)
{
    if (range != rangeDb)
    {
        range = rangeDb;
        updateCurve();
    }
}

void TransferFunctionDisplay::updateCurve()
{
    repaint();
}

//==============================================================================
// MÉTODOS DE CONFIGURACIÓN Y ESTADO
//==============================================================================

void TransferFunctionDisplay::setEnvelopeVisible(bool visible)
{
    if (envelopeVisible != visible)
    {
        envelopeVisible = visible;
        repaint();
    }
}

void TransferFunctionDisplay::clearWaveformData()
{
    // Limpiar todos los buffers de forma de onda
    std::fill(inputWaveformBuffer.begin(), inputWaveformBuffer.end(), -100.0f);
    std::fill(processedWaveformBuffer.begin(), processedWaveformBuffer.end(), -100.0f);
    std::fill(gainReductionBuffer.begin(), gainReductionBuffer.end(), 0.0f);
    
    // ELIMINADO: Limpiar buffer DELTA - ya no se utiliza
    
    // Resetear estados de envolvente
    inputEnvelopeState.store(0.0f, std::memory_order_relaxed);
    processedEnvelopeState.store(0.0f, std::memory_order_relaxed);
    
    // Resetear índice de escritura y marcar que no hay datos
    waveformWriteIndex.store(0);
    hasWaveformData.store(false);
    
    // Forzar repintado
    repaint();
}

//==============================================================================
// OVERRIDES DE TOOLTIP CLIENT
//==============================================================================

juce::String TransferFunctionDisplay::getTooltip()
{
    // El tooltip se establece desde PluginEditor usando setHelpText()
    // Este método debe retornar el helpText establecido por el componente padre
    return getHelpText();
}

void TransferFunctionDisplay::setLogicStoppedState(bool stopped)
{
    bool wasStoppedBefore = isLogicStopped.load();
    isLogicStopped = stopped;
    
    // Si Logic acaba de parar o reanudar, limpiar los buffers de visualización
    if (!wasStoppedBefore && stopped)
    {
        // Logic acaba de parar - no hacer nada, mantener la última visualización
    }
    else if (wasStoppedBefore && !stopped)
    {
        // Logic acaba de reanudar - limpiar buffers para evitar mezclar datos antiguos
        clearWaveformData();
    }
}

//==============================================================================
// MÉTODOS PRIVADOS DE DIBUJO
//==============================================================================

void TransferFunctionDisplay::drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Determinar el rango de dB según el nivel de zoom
    float minDb, maxDb;
    switch (currentZoom)
    {
        case ZoomLevel::Normal:
            minDb = -100.0f;
            maxDb = 0.0f;
            break;
        case ZoomLevel::Zoomed:
            minDb = -50.0f;
            maxDb = 0.0f;
            break;
    }
    
    g.setColour(DarkTheme::textSecondary.withAlpha(0.1f));
    
    // Ajustar las líneas de la grilla según el zoom
    std::vector<float> dbValues;
    std::vector<float> secondaryDbValues;
    
    switch (currentZoom)
    {
        case ZoomLevel::Normal:
            dbValues = { 0.0f, -10.0f, -20.0f, -30.0f, -40.0f, -50.0f, -60.0f, -80.0f, -100.0f };
            secondaryDbValues = { -5.0f, -15.0f, -25.0f, -35.0f, -45.0f, -55.0f, -70.0f, -90.0f };
            break;
        case ZoomLevel::Zoomed:
            dbValues = { 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -30.0f, -36.0f, -42.0f, -50.0f };
            secondaryDbValues = { -3.0f, -9.0f, -15.0f, -21.0f, -27.0f, -33.0f, -39.0f, -47.0f };
            break;
    }
    
    // Grid vertical (Input dB)
    for (float db : dbValues)
    {
        float x = juce::jmap(db, minDb, maxDb, bounds.getX(), bounds.getRight());
        g.drawVerticalLine(int(x), bounds.getY(), bounds.getBottom());
    }
    
    // Grid horizontal (Output dB - coordenadas invertidas)
    for (float db : dbValues)
    {
        float y = juce::jmap(db, minDb, maxDb, bounds.getBottom(), bounds.getY());
        g.drawHorizontalLine(int(y), bounds.getX(), bounds.getRight());
    }
    
    // Líneas secundarias más tenues
    g.setColour(DarkTheme::textSecondary.withAlpha(0.05f));
    
    for (float db : secondaryDbValues)
    {
        float x = juce::jmap(db, minDb, maxDb, bounds.getX(), bounds.getRight());
        g.drawVerticalLine(int(x), bounds.getY(), bounds.getBottom());
        
        float y = juce::jmap(db, minDb, maxDb, bounds.getBottom(), bounds.getY());
        g.drawHorizontalLine(int(y), bounds.getX(), bounds.getRight());
    }
}

void TransferFunctionDisplay::drawAxes(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour(DarkTheme::textSecondary.withAlpha(0.4f));
    
    // Eje X (entrada) - parte inferior
    g.drawHorizontalLine(int(bounds.getBottom()), bounds.getX(), bounds.getRight());
    
    // Eje Y (salida) - parte izquierda
    g.drawVerticalLine(int(bounds.getX()), bounds.getY(), bounds.getBottom());
    
    // Labels de ejes (pequeños)
    g.setColour(DarkTheme::textPrimary.withAlpha(0.6f));
    g.setFont(7.0f);
    
    // Label entrada (abajo centro)
    g.drawText("Input (dB)", bounds.getCentreX() - 25, bounds.getBottom() + 2, 50, 8,
               juce::Justification::centred);
    
    // Label salida (izquierda centro, vertical)
    g.drawText("Output", bounds.getX() - 12, bounds.getCentreY() - 15, 12, 30,
               juce::Justification::centred);
}

void TransferFunctionDisplay::drawThresholdProjection(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Determinar el rango de dB según el nivel de zoom
    float minDb, maxDb;
    switch (currentZoom)
    {
        case ZoomLevel::Normal:
            minDb = -100.0f;
            maxDb = 0.0f;
            break;
        case ZoomLevel::Zoomed:
            minDb = -50.0f;
            maxDb = 0.0f;
            break;
    }
    
    // Solo dibujar si el threshold está dentro del rango visible
    if (threshold < minDb || threshold > maxDb) return;
    
    // Calcular el mismo offset vertical que se usa en calculateOutput
    float thresholdOffset = 0.0f;
    if (knee > 0.0f && ratio > 1.0f)
    {
        thresholdOffset = knee * 0.15f * (1.0f - 1.0f / ratio);
    }
    
    // Aplicar el offset al threshold efectivo para la proyección
    float effectiveThreshold = threshold - thresholdOffset;
    
    g.setColour(DarkTheme::accent.withAlpha(0.6f));
    
    // Coordenadas del threshold efectivo
    float thresholdX = juce::jmap(threshold, minDb, maxDb, bounds.getX(), bounds.getRight());
    float thresholdY = juce::jmap(effectiveThreshold, minDb, maxDb, bounds.getBottom(), bounds.getY());
    
    // Línea punteada vertical desde el threshold hasta abajo (proyección en eje X)
    juce::Path verticalDash;
    for (float y = thresholdY; y < bounds.getBottom(); y += 4.0f) {
        verticalDash.addRectangle(thresholdX - 0.5f, y, 1.0f, 2.0f);
    }
    g.fillPath(verticalDash);
    
    // Línea punteada horizontal desde el threshold hasta la izquierda (proyección en eje Y)
    juce::Path horizontalDash;
    for (float x = bounds.getX(); x < thresholdX; x += 4.0f) {
        horizontalDash.addRectangle(x, thresholdY - 0.5f, 2.0f, 1.0f);
    }
    g.fillPath(horizontalDash);
    
    // Punto en la intersección del threshold
    g.setColour(DarkTheme::accent);
    g.fillEllipse(thresholdX - 2, thresholdY - 2, 4, 4);
}

void TransferFunctionDisplay::drawRangeProjection(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Determinar el rango de dB según el nivel de zoom
    float minDb, maxDb;
    switch (currentZoom)
    {
        case ZoomLevel::Normal:
            minDb = -100.0f;
            maxDb = 0.0f;
            break;
        case ZoomLevel::Zoomed:
            minDb = -50.0f;
            maxDb = 0.0f;
            break;
    }
    
    // Línea paralela a 1:1 desplazada por el valor RANGE
    g.setColour(juce::Colours::white.withAlpha(0.8f));  // Blanco para RANGE
    
    juce::Path rangePath;
    
    // Dos puntos para dibujar la línea paralela: y = x + range
    // Punto inicial: esquina inferior izquierda + range
    float startInputDb = minDb;
    float startOutputDb = startInputDb + range;
    
    // Punto final: esquina superior derecha + range  
    float endInputDb = maxDb;
    float endOutputDb = endInputDb + range;
    
    // Clipping completo para mantener la línea dentro del área visible
    // Para una línea y = x + range, necesitamos verificar intersecciones con todos los bordes
    
    // Verificar si la línea es completamente invisible
    float lineStartOutput = minDb + range;  // y cuando x = minDb
    float lineEndOutput = maxDb + range;    // y cuando x = maxDb
    
    // Si toda la línea está fuera del rango visible, no dibujar
    if ((lineStartOutput > maxDb && lineEndOutput > maxDb) || 
        (lineStartOutput < minDb && lineEndOutput < minDb))
    {
        return; // Línea completamente fuera del área visible
    }
    
    // Calcular puntos de intersección con los bordes del área visible
    float clippedStartInputDb = startInputDb;
    float clippedStartOutputDb = startOutputDb;
    float clippedEndInputDb = endInputDb;
    float clippedEndOutputDb = endOutputDb;
    
    // Clipping en borde superior (maxDb)
    if (startOutputDb > maxDb) {
        clippedStartInputDb = maxDb - range;  // Resolver: clippedStartInputDb + range = maxDb
        clippedStartOutputDb = maxDb;
    }
    if (endOutputDb > maxDb) {
        clippedEndInputDb = maxDb - range;
        clippedEndOutputDb = maxDb;
    }
    
    // Clipping en borde inferior (minDb)
    if (startOutputDb < minDb) {
        clippedStartInputDb = minDb - range;  // Resolver: clippedStartInputDb + range = minDb
        clippedStartOutputDb = minDb;
    }
    if (endOutputDb < minDb) {
        clippedEndInputDb = minDb - range;
        clippedEndOutputDb = minDb;
    }
    
    // Asegurar que los puntos de entrada también estén en el rango visible
    clippedStartInputDb = juce::jlimit(minDb, maxDb, clippedStartInputDb);
    clippedEndInputDb = juce::jlimit(minDb, maxDb, clippedEndInputDb);
    
    // Recalcular outputs después del clipping de inputs
    clippedStartOutputDb = clippedStartInputDb + range;
    clippedEndOutputDb = clippedEndInputDb + range;
    
    // Convertir a coordenadas de píxeles
    auto clippedStartPoint = dbToPixel(clippedStartInputDb, clippedStartOutputDb, bounds);
    auto clippedEndPoint = dbToPixel(clippedEndInputDb, clippedEndOutputDb, bounds);
    
    // Solo dibujar si tenemos una línea válida
    if (clippedStartInputDb != clippedEndInputDb || clippedStartOutputDb != clippedEndOutputDb)
    {
        rangePath.startNewSubPath(clippedStartPoint);
        rangePath.lineTo(clippedEndPoint);
        
        // Crear patrón discontinuo: 4 píxeles línea, 3 píxeles espacio
        float dashLengths[] = {4.0f, 3.0f};
        juce::PathStrokeType dashedStroke(1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded);
        dashedStroke.createDashedStroke(rangePath, rangePath, dashLengths, 2);
        g.fillPath(rangePath);
    }
}

void TransferFunctionDisplay::drawKneeArea(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    if (knee <= 0.0f) return; // No dibujar si knee es 0 (hard knee)
    
    // Determinar el rango de dB según el nivel de zoom
    float minDb, maxDb;
    switch (currentZoom)
    {
        case ZoomLevel::Normal:
            minDb = -100.0f;
            maxDb = 0.0f;
            break;
        case ZoomLevel::Zoomed:
            minDb = -50.0f;
            maxDb = 0.0f;
            break;
    }
    
    // Calcular el mismo offset vertical que se usa en calculateOutput
    float thresholdOffset = 0.0f;
    if (knee > 0.0f && ratio > 1.0f)
    {
        thresholdOffset = knee * 0.15f * (1.0f - 1.0f / ratio);
    }
    
    // Aplicar el offset al threshold efectivo
    float effectiveThreshold = threshold - thresholdOffset;
    
    // Dibujar el área de transición suave del knee - como una sombra muy sutil
    g.setColour(DarkTheme::textSecondary.withAlpha(0.03f));
    
    // Calcular los puntos de inicio y fin del knee usando el threshold efectivo
    float kneeStart = effectiveThreshold - knee;
    float kneeEnd = effectiveThreshold + knee;
    
    // Solo dibujar la parte visible del knee
    kneeStart = juce::jmax(kneeStart, minDb);
    kneeEnd = juce::jmin(kneeEnd, maxDb);
    
    if (kneeStart >= maxDb || kneeEnd <= minDb) return; // Knee completamente fuera del rango visible
    
    // Convertir a coordenadas de píxel (usar threshold original para X, efectivo para Y)
    float kneeStartX = juce::jmap(threshold - knee, minDb, maxDb, bounds.getX(), bounds.getRight());
    float kneeEndX = juce::jmap(threshold + knee, minDb, maxDb, bounds.getX(), bounds.getRight());
    
    // Dibujar área sombreada vertical para indicar la zona de transición
    juce::Rectangle<float> kneeRect(kneeStartX, bounds.getY(), kneeEndX - kneeStartX, bounds.getHeight());
    g.fillRect(kneeRect);
    
    // Dibujar líneas verticales punteadas en los extremos del knee - aún más sutiles
    g.setColour(DarkTheme::textSecondary.withAlpha(0.05f));
    
    // Línea inicio del knee (solo si está visible)
    if (threshold - knee > minDb)
    {
        juce::Path startDash;
        for (float y = bounds.getY(); y < bounds.getBottom(); y += 4.0f) {
            startDash.addRectangle(kneeStartX - 0.5f, y, 1.0f, 2.0f);
        }
        g.fillPath(startDash);
    }
    
    // Línea fin del knee (solo si está visible)
    if (threshold + knee < maxDb)
    {
        juce::Path endDash;
        for (float y = bounds.getY(); y < bounds.getBottom(); y += 4.0f) {
            endDash.addRectangle(kneeEndX - 0.5f, y, 1.0f, 2.0f);
        }
        g.fillPath(endDash);
    }
}

void TransferFunctionDisplay::drawTransferCurve(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Determinar el rango de dB según el nivel de zoom
    float minDb, maxDb;
    switch (currentZoom)
    {
        case ZoomLevel::Normal:
            minDb = -100.0f;
            maxDb = 0.0f;
            break;
        case ZoomLevel::Zoomed:
            minDb = -50.0f;
            maxDb = 0.0f;
            break;
    }
    
    juce::Path curvePath;
    bool firstPoint = true;
    
    // Generar puntos de la curva dentro del rango visible
    for (float inputDb = minDb; inputDb <= maxDb; inputDb += 0.5f)
    {
        float outputDb = calculateOutput(inputDb);
        auto point = dbToPixel(inputDb, outputDb, bounds);
        
        if (firstPoint)
        {
            curvePath.startNewSubPath(point);
            firstPoint = false;
        }
        else
        {
            curvePath.lineTo(point);
        }
    }
    
    // Solo dibujar la curva blanca si NO estamos en modos especiales
    // (BYPASS, SOLO SC ocultan la curva para mejor lectura de textos)
    if (!bypassMode && !soloSidechainActive)
    {
        // Dibujar la curva en blanco
        g.setColour(juce::Colours::white);
        g.strokePath(curvePath, juce::PathStrokeType(2.0f));
    }
}

float TransferFunctionDisplay::calculateKneeOutput(float inputDb, float threshold, float knee, float ratio)
{
    // Función auxiliar para calcular output solo en la zona del knee
    if (knee <= 0.0f || inputDb < threshold - knee || inputDb > threshold + knee)
    {
        // Fuera de la zona del knee - no debería llamarse aquí
        return inputDb;
    }
    
    // Posición en el knee normalizada de 0 a 1
    float kneePos = (inputDb - (threshold - knee)) / (2.0f * knee);
    
    // Interpolación suave usando función coseno
    float blend = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * kneePos));
    
    // Calcular outputs para ambos extremos
    float noCompressionOutput = inputDb;
    float fullCompressionOutput = threshold + (inputDb - threshold) / ratio;
    
    // Interpolar entre sin compresión y compresión completa
    return blend * fullCompressionOutput + (1.0f - blend) * noCompressionOutput;
}

//==============================================================================
// FUNCIONES MATEMÁTICAS DEL EXPANSOR
//==============================================================================

float TransferFunctionDisplay::calculateOutput(float inputDb)
{
    inputDb = juce::jlimit(-100.0f, 0.0f, inputDb);

    float output;

    if (knee <= 0.0f)
    {
        // Hard knee - EXPANDER LOGIC
        if (inputDb <= threshold)
            output = threshold + (inputDb - threshold) * ratio;  // Atenuar señales bajo threshold
        else
            output = inputDb;  // Señales sobre threshold pasan sin cambio
    }
    else
    {
        // Soft knee - EXPANDER LOGIC
        float kneeHalf = knee * 0.5f;
        float kneeStart = threshold - kneeHalf;
        float kneeEnd = threshold + kneeHalf;

        if (inputDb <= kneeStart)
        {
            // Línea expandida (atenuada)
            output = threshold + (inputDb - threshold) * ratio;
        }
        else if (inputDb >= kneeEnd)
        {
            // Línea 1:1 (sin expansión)
            output = inputDb;
        }
        else
        {
            // Dentro del knee: interpolación hermite cúbica
            float t = (inputDb - kneeStart) / knee; // normalizar 0-1

            // Puntos de conexión exactos para EXPANDER
            float y0 = threshold + (kneeStart - threshold) * ratio;  // punto inicial (línea expandida)
            float y1 = kneeEnd;  // punto final (línea 1:1)

            // Derivadas en los extremos (pendientes de las líneas)
            float m0 = ratio;  // pendiente línea expandida
            float m1 = 1.0f;   // pendiente línea 1:1

            // Interpolación Hermite que respeta puntos y derivadas
            float t2 = t * t;
            float t3 = t2 * t;

            float h00 = 2*t3 - 3*t2 + 1;
            float h10 = t3 - 2*t2 + t;
            float h01 = -2*t3 + 3*t2;
            float h11 = t3 - t2;

            output = h00 * y0 + h10 * knee * m0 + h01 * y1 + h11 * knee * m1;
        }
    }

    // Aplicar RANGE como línea paralela móvil a 1:1
    // Línea paralela: output_range = input + range
    float rangeLineOutput = inputDb + range;
    
    // La función se "corta" por la línea del RANGE si va por debajo
    if (output < rangeLineOutput)
    {
        output = rangeLineOutput;
    }
    
    return output;
}


juce::Point<float> TransferFunctionDisplay::dbToPixel(float inputDb, float outputDb, juce::Rectangle<float> bounds)
{
    // Determinar el rango de dB según el nivel de zoom
    float minDb, maxDb;
    switch (currentZoom)
    {
        case ZoomLevel::Normal:
            minDb = -100.0f;
            maxDb = 0.0f;
            break;
        case ZoomLevel::Zoomed:
            minDb = -50.0f;
            maxDb = 0.0f;
            break;
    }
    
    // X: Input va de izquierda (minDb) a derecha (maxDb)
    float x = juce::jmap(inputDb, minDb, maxDb, bounds.getX(), bounds.getRight());
    
    // Y: Output va de abajo (minDb) a arriba (maxDb) - coordenadas invertidas
    float y = juce::jmap(outputDb, minDb, maxDb, bounds.getBottom(), bounds.getY());
    
    return {x, y};
}

juce::Point<float> TransferFunctionDisplay::pixelToDb(juce::Point<float> pixel, juce::Rectangle<float> bounds)
{
    // Determinar el rango de dB según el nivel de zoom
    float minDb, maxDb;
    switch (currentZoom)
    {
        case ZoomLevel::Normal:
            minDb = -100.0f;
            maxDb = 0.0f;
            break;
        case ZoomLevel::Zoomed:
            minDb = -50.0f;
            maxDb = 0.0f;
            break;
    }
    
    // Convertir coordenadas de píxel a dB
    float inputDb = juce::jmap(pixel.x, bounds.getX(), bounds.getRight(), minDb, maxDb);
    float outputDb = juce::jmap(pixel.y, bounds.getBottom(), bounds.getY(), minDb, maxDb);
    
    return {inputDb, outputDb};
}

//==============================================================================
// MÉTODOS DE DATOS DE WAVEFORM (THREAD-SAFE)
//==============================================================================

void TransferFunctionDisplay::updateWaveformData(const float* inputSamples, const float* processedSamples, int numSamples)
{
    if (numSamples <= 0) return;
    
    int writeIndex = waveformWriteIndex.load();
    
    // Verificar si hay señal o silencio
    float maxSignalLevel = 0.0f;
    for (int i = 0; i < numSamples; i += 64)  // Chequeo rápido cada 64 muestras
    {
        maxSignalLevel = juce::jmax(maxSignalLevel, std::abs(inputSamples[i]));
        if (maxSignalLevel > silenceLevel) break;
    }
    
    // Actualizar contador de silencio
    if (maxSignalLevel < silenceLevel)
    {
        int currentSilenceCounter = silenceCounter.load(std::memory_order_relaxed);
        silenceCounter.store(currentSilenceCounter + 1, std::memory_order_relaxed);
        
        if (currentSilenceCounter > silenceThreshold)
        {
            isSilent.store(true, std::memory_order_relaxed);
            // Comenzar fade out progresivo
            float currentFade = fadeOutFactor.load(std::memory_order_relaxed);
            fadeOutFactor.store(currentFade * 0.95f, std::memory_order_relaxed);  // Desvanecimiento gradual
            
            // NO limpiar el buffer inmediatamente, solo aplicar fade
            float currentInputEnv = inputEnvelopeState.load(std::memory_order_relaxed);
            float currentProcessedEnv = processedEnvelopeState.load(std::memory_order_relaxed);
            inputEnvelopeState.store(currentInputEnv * 0.95f, std::memory_order_relaxed);
            processedEnvelopeState.store(currentProcessedEnv * 0.95f, std::memory_order_relaxed);
            
            // Solo limpiar cuando el fade es muy bajo
            if (currentFade < 0.05f)
            {
                fadeOutFactor.store(0.0f, std::memory_order_relaxed);
                // Mantener los datos pero con fade completo
            }
        }
    }
    else
    {
        silenceCounter.store(0, std::memory_order_relaxed);
        isSilent.store(false, std::memory_order_relaxed);
        fadeOutFactor.store(1.0f, std::memory_order_relaxed);  // Restaurar opacidad completa
        hasWaveformData.store(true);
    }
    
    // NUEVO: Solo capturar salida procesada en modo bipolar (sin abs())
    // NOTA: Según comentarios, inputSamples[i] es realmente la salida procesada
    float processedSample = inputSamples[0];  // Usar primera muestra como representativa
    
    // Para múltiples muestras, usar la de mayor magnitud (pero mantener signo)
    for (int i = 1; i < numSamples; ++i)
    {
        if (std::abs(inputSamples[i]) > std::abs(processedSample)) {
            processedSample = inputSamples[i];
        }
    }
    
    // Si estamos en silencio, aplicar fade progresivo
    if (isSilent.load(std::memory_order_relaxed))
    {
        float currentFade = fadeOutFactor.load(std::memory_order_relaxed);
        processedSample *= currentFade;
    }
    
    // Detectar cambios rápidos en la señal usando magnitud
    float prevLevel = previousInputLevel.load(std::memory_order_relaxed);
    float currentMagnitude = std::abs(processedSample);
    float inputChange = std::abs(currentMagnitude - prevLevel);
    float currentDetector = changeDetector.load(std::memory_order_relaxed);
    changeDetector.store(currentDetector * 0.9f + inputChange * 0.1f, std::memory_order_relaxed);
    previousInputLevel.store(currentMagnitude, std::memory_order_relaxed);
    
    // Cambiar a modo rápido si detectamos transientes
    useFastMode.store(currentDetector > 0.1f, std::memory_order_relaxed);
    
    // NUEVO: Almacenar valor bipolar directamente (sin conversión a dB)
    // Clampear al rango [-1.0, 1.0] para evitar desbordamientos visuales
    float clampedProcessedSample = juce::jlimit(-1.0f, 1.0f, processedSample);
    
    // Solo almacenar la señal de salida procesada (entrada no se necesita más)
    processedWaveformBuffer[writeIndex] = clampedProcessedSample;
    
    // Limpiar entrada para evitar confusion visual (opcional)
    inputWaveformBuffer[writeIndex] = 0.0f;
        
    // Avanzar índice de escritura para próxima muestra
    waveformWriteIndex.store((writeIndex + 1) % waveformBufferSize);
    
    hasWaveformData.store(true);
}

// NUEVO: Método unificado para sincronizar forma de onda con histogramas Attack/Sustain
void TransferFunctionDisplay::updateWaveformData(const float* inputSamples, const float* processedSamples, const float* attackSamples, const float* sustainSamples, int numSamples)
{
    if (numSamples <= 0) return;
    
    int writeIndex = waveformWriteIndex.load();
    
    // NUEVO: Verificar si hay señal o silencio (igual que en el otro método)
    float maxSignalLevel = 0.0f;
    for (int i = 0; i < numSamples; i += 64)  // Chequeo rápido cada 64 muestras
    {
        maxSignalLevel = juce::jmax(maxSignalLevel, std::abs(processedSamples[i]));
        if (maxSignalLevel > silenceLevel) break;
    }
    
    // NUEVO: Actualizar contador de silencio
    if (maxSignalLevel < silenceLevel)
    {
        int currentSilenceCounter = silenceCounter.load(std::memory_order_relaxed);
        silenceCounter.store(currentSilenceCounter + 1, std::memory_order_relaxed);
        
        if (currentSilenceCounter > silenceThreshold)
        {
            isSilent.store(true, std::memory_order_relaxed);
            // Comenzar fade out progresivo
            float currentFade = fadeOutFactor.load(std::memory_order_relaxed);
            fadeOutFactor.store(currentFade * 0.95f, std::memory_order_relaxed);  // Desvanecimiento gradual
            
            // Solo limpiar cuando el fade es muy bajo
            if (currentFade < 0.05f)
            {
                fadeOutFactor.store(0.0f, std::memory_order_relaxed);
                // Mantener los datos pero con fade completo
            }
        }
    }
    else
    {
        silenceCounter.store(0, std::memory_order_relaxed);
        isSilent.store(false, std::memory_order_relaxed);
        fadeOutFactor.store(1.0f, std::memory_order_relaxed);  // Restaurar opacidad completa
        hasWaveformData.store(true);
    }
    
    // Procesar todas las muestras con el mismo timestamp
    for (int i = 0; i < numSamples; ++i)
    {
        // WAVEFORM DATA - Conversión bipolar directa sin dB para sincronización
        float clampedProcessed = juce::jlimit(-1.0f, 1.0f, processedSamples[i]);
        processedWaveformBuffer[writeIndex] = clampedProcessed;
        inputWaveformBuffer[writeIndex] = 0.0f; // Input deshabilitado como antes
        
        // ATTACK/SUSTAIN DATA - Misma conversión y timestamp
        if (attackSamples && sustainSamples) {
            float clampedAttack = juce::jlimit(-1.0f, 1.0f, attackSamples[i]);
            float clampedSustain = juce::jlimit(-1.0f, 1.0f, sustainSamples[i]);
            
            attackGainBuffer[writeIndex] = clampedAttack;
            sustainGainBuffer[writeIndex] = clampedSustain;
        }
        
        // Avanzar al siguiente índice
        writeIndex = (writeIndex + 1) % waveformBufferSize;
    }
    
    // Actualizar índice de escritura una sola vez al final
    waveformWriteIndex.store(writeIndex);
    // NOTA: hasWaveformData se actualiza arriba en la lógica de detección de silencio
}

void TransferFunctionDisplay::updateWaveformDataWithGR(const float* inputSamples, const float* processedSamples, const float* gainReductionSamples, int numSamples)
{
    if (numSamples <= 0) return;
    
    int writeIndex = waveformWriteIndex.load();
    
    // Procesar todas las muestras disponibles
    for (int i = 0; i < numSamples; i++)
    {
        // Encontrar el valor máximo en las muestras (detector de picos)
        float maxInput = std::abs(inputSamples[i]);
        float maxProcessed = std::abs(processedSamples[i]);
        
        // Actualizar el estado del envelope follower
        // Similar al método anterior pero usando la GR real de Gen~
        
        // Detectar cambios rápidos en la señal
        float inputChange = std::abs(maxInput - previousInputLevel);
        changeDetector = changeDetector * 0.9f + inputChange * 0.1f;
        previousInputLevel = maxInput;
        
        // Cambiar a modo rápido si detectamos transientes
        useFastMode = changeDetector > 0.1f;
        
        // Parámetros independientes para diferentes tipos de visualización
        //float envelopeReleaseTime = 0.3f;   // Release rápido para envolventes (comportamiento original)
        
        // Factor de suavizado por defecto para gain reduction (valor medio)
        // grSmoothingFactor = 0.7f;  // Equivalente a ~100ms de release - REMOVIDO (no usado)
        
        // TEMPORAL DEBUG - BYPASS ENVELOPE FILTERING
        /*
        // Filtro de envolvente original (COMENTADO TEMPORALMENTE)
        {
            // Para entrada - seguimiento directo sin ruido artificial
            float clampedInput = juce::jmin(maxInput, 0.99f); // Evitar que supere 0 dBFS
            float currentInputEnv = inputEnvelopeState.load(std::memory_order_relaxed);
            
            if (clampedInput > currentInputEnv * 1.1f)  // Umbral más bajo para más respuesta
            {
                // Attack instantáneo
                inputEnvelopeState.store(clampedInput, std::memory_order_relaxed);
            }
            else
            {
                // Release muy abrupto para mantener detalles
                float newValue = currentInputEnv * envelopeReleaseTime + clampedInput * (1.0f - envelopeReleaseTime);
                
                // Corte abrupto en silencio
                if (newValue < 0.001f) newValue = 0.0f;
                inputEnvelopeState.store(newValue, std::memory_order_relaxed);
            }
            
            // Para salida procesada - igual de directo sin ruido
            float currentProcessedEnv = processedEnvelopeState.load(std::memory_order_relaxed);
            
            if (maxProcessed > currentProcessedEnv * 1.1f)
            {
                // Attack instantáneo
                processedEnvelopeState.store(maxProcessed, std::memory_order_relaxed);
            }
            else
            {
                // Mezclar con la señal actual para más detalle
                float newValue = currentProcessedEnv * envelopeReleaseTime + maxProcessed * (1.0f - envelopeReleaseTime);
                
                // Corte abrupto
                if (newValue < 0.001f) newValue = 0.0f;
                processedEnvelopeState.store(newValue, std::memory_order_relaxed);

            }
            
            // Re-leer los valores finales para convertir a dB
            float finalInputEnv = inputEnvelopeState.load(std::memory_order_relaxed);
            float finalProcessedEnv = processedEnvelopeState.load(std::memory_order_relaxed);
            
            // Convertir a dB sin ruido artificial
            float inputDb = finalInputEnv > 0.0001f ?
                20.0f * std::log10(finalInputEnv) : -100.0f;
            float processedDb = finalProcessedEnv > 0.0001f ?
                20.0f * std::log10(finalProcessedEnv) : -100.0f;
            
            // Limitar al rango del gráfico (-100 dB a 0 dB)
            inputDb = juce::jlimit(-100.0f, 0.0f, inputDb);
            processedDb = juce::jlimit(-100.0f, 0.0f, processedDb);
            
            inputWaveformBuffer[writeIndex] = inputDb;
            processedWaveformBuffer[writeIndex] = processedDb;
        }
        */
        
        // DIRECT CONVERSION - NO FILTERING (DEBUG)
        {
            float inputDb = maxInput > 0.0001f ?
                20.0f * std::log10(maxInput) : -100.0f;
            float processedDb = maxProcessed > 0.0001f ?
                20.0f * std::log10(maxProcessed) : -100.0f;
            
            // SIEMPRE escribir los datos originales primero
            inputWaveformBuffer[writeIndex] = inputDb;
            processedWaveformBuffer[writeIndex] = processedDb;
            
            // NUEVO: Usar datos reales de gain reduction de Gen~
            if (gainReductionSamples != nullptr)
            {
                // Convertir gain reduction real de Gen~ a dB
                float grLinear = gainReductionSamples[i];
                float grDb = grLinear > 0.0001f ? 20.0f * std::log10(grLinear) : -60.0f;
                
                // Para expansor: valores negativos indican expansión (atenuación)
                // Los datos de Gen~ ya vienen en el formato correcto
                gainReductionBuffer[writeIndex] = grDb;
            }
            else
            {
                // Fallback: calcular gain reduction basándose en diferencia de envolventes
                // Para expansor: cuando processed < input hay expansión (valor positivo)
                float calculatedGR = inputDb - processedDb;
                gainReductionBuffer[writeIndex] = calculatedGR;
            }
        }
        
        // Solo escribir UN valor por llamada (no múltiples)
        writeIndex = (writeIndex + 1) % waveformBufferSize;
    }
    
    waveformWriteIndex.store(writeIndex);
    hasWaveformData.store(true);
}

void TransferFunctionDisplay::drawWaveformAreas(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    if (!hasWaveformData.load()) return;
    
    // Si Logic está parado, no dibujar las envolventes
    if (isLogicStopped.load()) return;
    
    const int readIndex = waveformWriteIndex.load();
    const float currentFadeOutFactor = fadeOutFactor.load(std::memory_order_relaxed);
    
    // NO expandir bounds - mantener dentro del área del gráfico
    // bounds = bounds.expanded(60.0f, 10.0f);  // COMENTADO - causaba el bloque azul
    
    // ELIMINADO: Variables no utilizadas con nueva visualización pico-valle
    // float minDb, maxDb - ya no necesario para mapeo dB
    
    // Crear paths para áreas rellenas
    juce::Path inputAreaPath;
    juce::Path processedAreaPath;   // Salida procesada

    // RESTAURADO: Colectar puntos para visualización (lógica temporal original)
    std::vector<juce::Point<float>> processedPoints;    // Puntos principales de la waveform
    
    // Mostrar los últimos 'displayPoints' valores del buffer circular con lógica temporal original
    for (int i = 0; i < displayPoints; ++i)
    {
        // RESTAURADO: Lógica temporal original que funcionaba
        int samplesBack = displayPoints - i + 5;  // Offset mínimo para sincronía audio-visual
        int bufferIndex = (readIndex - samplesBack + waveformBufferSize) % waveformBufferSize;
        
        // Leer valor bipolar principal
        float processedSample = processedWaveformBuffer[bufferIndex];

        // NUEVO: Detección de picos micro-local (analizar 3 muestras adyacentes)
        float localMax = processedSample;
        float localMin = processedSample;

        // Analizar muestra anterior y siguiente para detectar picos locales
        for (int offset = -1; offset <= 1; ++offset)
        {
            if (offset == 0) continue; // Ya tenemos la muestra central
            
            int adjacentIndex = (bufferIndex + offset + waveformBufferSize) % waveformBufferSize;
            float adjacentSample = processedWaveformBuffer[adjacentIndex];
            
            if (adjacentSample > localMax) localMax = adjacentSample;
            if (adjacentSample < localMin) localMin = adjacentSample;
        }
        
        // Decidir qué valor usar: si hay diferencia significativa, usar el extremo más prominente
        float finalSample = processedSample; // Por defecto usar muestra central
        float peakDifference = std::abs(localMax - processedSample);
        float valleyDifference = std::abs(localMin - processedSample);

        if (peakDifference > 0.05f || valleyDifference > 0.05f) // Umbral de significancia
        {
            // Usar el extremo más prominente
            finalSample = (peakDifference > valleyDifference) ? localMax : localMin;
        }
        
        // Calcular posición X - usar todo el ancho disponible
        float normalizedTime = float(i) / float(displayPoints - 1);
        float x = bounds.getX() + normalizedTime * bounds.getWidth();
        
        // Mapeo bipolar - 0 en el centro, con amplificación visual en zoom x2
        float topOffset = 5.0f; // Margen desde bordes
        float visualRange = (currentZoom == ZoomLevel::Zoomed) ? 0.5f : 1.0f; // x2 amplificación en zoom
        float y = juce::jmap(finalSample, -visualRange, visualRange, bounds.getBottom() - topOffset, bounds.getY() + topOffset);

        // Limitar valores a los bounds para evitar artefactos
        y = juce::jlimit(bounds.getY() + topOffset, bounds.getBottom() - topOffset, y);

        processedPoints.push_back({x, y});
    }

    // NUEVO: Dibujar waveform mejorada con detección de picos micro-local
    if (!processedPoints.empty())
    {
        // COLORES NEUTROS: Gradiente gris-blanco para mejor visibilidad
        const juce::Colour lightGrey = DarkTheme::textPrimary;        // Blanco
        const juce::Colour mediumGrey = DarkTheme::textSecondary;     // Gris claro

        // 1. Crear área rellena sutil desde línea central
        const float centerY = bounds.getCentreY();
        juce::Path waveformArea;
        waveformArea.startNewSubPath(processedPoints[0].x, centerY);
        waveformArea.lineTo(processedPoints[0]);
        
        // Conectar todos los puntos
        for (size_t i = 1; i < processedPoints.size(); ++i)
        {
            waveformArea.lineTo(processedPoints[i]);
        }
        
        // Cerrar área volviendo al centro
        waveformArea.lineTo(processedPoints.back().x, centerY);
        waveformArea.closeSubPath();
        
        // Dibujar área rellena muy sutil (reducida aún más para mayor difuminado)
        g.setColour(lightGrey.withAlpha(0.05f * currentFadeOutFactor));
        g.fillPath(waveformArea);

        // 2. Dibujar línea continua con efecto difuminado (multicapa)
        juce::Path waveformLine;
        waveformLine.startNewSubPath(processedPoints[0]);
        for (size_t i = 1; i < processedPoints.size(); ++i)
        {
            waveformLine.lineTo(processedPoints[i]);
        }

        // Capa base: más gruesa y muy difusa (efecto blur manual aumentado)
        g.setColour(lightGrey.withAlpha(0.12f * currentFadeOutFactor));
        g.strokePath(waveformLine, juce::PathStrokeType(4.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));

        // Capa superior: línea principal aún más sutil (reducida de 0.3f a 0.22f)
        g.setColour(lightGrey.withAlpha(0.22f * currentFadeOutFactor));
        g.strokePath(waveformLine, juce::PathStrokeType(1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));

        // 3. Dibujar línea de referencia central (cero)
        g.setColour(mediumGrey.withAlpha(0.3f * currentFadeOutFactor));
        g.drawHorizontalLine(static_cast<int>(centerY), bounds.getX(), bounds.getRight());
    }
    
}

//==============================================================================
// Eventos de mouse

void TransferFunctionDisplay::mouseEnter(const juce::MouseEvent&)
{
    // Tooltip ya está configurado en el constructor
}

void TransferFunctionDisplay::mouseExit(const juce::MouseEvent&)
{
    // Resetear cursor
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void TransferFunctionDisplay::mouseDown(const juce::MouseEvent& e)
{
    // DESHABILITADO: No permitir interacción con transfer function
    // Solo mostrar envolventes - sin capacidad de modificar parámetros
    /*
    auto bounds = getLocalBounds().toFloat().reduced(15.0f, 10.0f);
    auto mousePos = e.position;
    
    // Detectar qué elemento se está arrastrando
    currentDragMode = detectDragMode(mousePos, bounds);
    lastMousePos = mousePos;
    
    // Guardar valor inicial para el drag
    switch (currentDragMode) {
        case DragMode::Threshold:
            dragStartValue = threshold;
            break;
        case DragMode::Knee:
            dragStartValue = knee;
            break;
        case DragMode::Ratio:
            dragStartValue = ratio;
            break;
        case DragMode::Range:
            dragStartValue = range;
            break;
        default:
            break;
    }
    */
}

void TransferFunctionDisplay::mouseDrag(const juce::MouseEvent& e)
{
    // DESHABILITADO: No permitir drag de parámetros - solo mostrar envolventes
    /*
    if (currentDragMode == DragMode::None) return;
    
    auto bounds = getLocalBounds().toFloat().reduced(15.0f, 10.0f);
    auto mousePos = e.position;
    auto deltaPixels = mousePos - lastMousePos;
    
    switch (currentDragMode) {
        case DragMode::Threshold: {
            // Arrastrar horizontalmente para cambiar threshold
            float deltaDb = juce::jmap(deltaPixels.x, 0.0f, bounds.getWidth(), 0.0f, 60.0f);
            float newThreshold = juce::jlimit(-60.0f, 0.0f, threshold + deltaDb);
            setThreshold(newThreshold);
            if (onThresholdChange) onThresholdChange(newThreshold);
            break;
        }
        case DragMode::Knee: {
            // Arrastrar horizontalmente para cambiar knee
            float deltaDb = juce::jmap(deltaPixels.x, 0.0f, bounds.getWidth(), 0.0f, 30.0f);
            float newKnee = juce::jlimit(0.0f, 20.0f, knee + deltaDb);
            setKnee(newKnee);
            if (onKneeChange) onKneeChange(newKnee);
            break;
        }
        case DragMode::Ratio: {
            // Arrastrar verticalmente para cambiar ratio
            float deltaRatio = juce::jmap(deltaPixels.y, 0.0f, bounds.getHeight(), 0.0f, 20.0f);
            float newRatio = juce::jlimit(1.0f, 20.0f, ratio + deltaRatio);
            setRatio(newRatio);
            if (onRatioChange) onRatioChange(newRatio);
            break;
        }
        case DragMode::Range: {
            // Arrastrar horizontalmente para cambiar range
            float deltaDb = juce::jmap(deltaPixels.x, 0.0f, bounds.getWidth(), 0.0f, 60.0f);
            float newRange = juce::jlimit(-60.0f, 0.0f, range + deltaDb);
            setRange(newRange);
            if (onRangeChange) onRangeChange(newRange);
            break;
        }
        default:
            break;
    }
    
    lastMousePos = mousePos;
    */
}

void TransferFunctionDisplay::mouseUp(const juce::MouseEvent&)
{
    // DESHABILITADO: No hay drag mode para resetear
    /*
    currentDragMode = DragMode::None;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    */
}

//==============================================================================
// Métodos de detección

TransferFunctionDisplay::DragMode TransferFunctionDisplay::detectDragMode(juce::Point<float> mousePos, juce::Rectangle<float> bounds)
{
    // Prioridad: Threshold > Range > Knee > Ratio
    if (isNearThresholdLine(mousePos, bounds)) {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        return DragMode::Threshold;
    }
    if (isNearRangeLine(mousePos, bounds)) {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        return DragMode::Range;
    }
    if (isNearKneeArea(mousePos, bounds)) {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        return DragMode::Knee;
    }
    if (isNearTransferCurve(mousePos, bounds)) {
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        return DragMode::Ratio;
    }
    
    setMouseCursor(juce::MouseCursor::NormalCursor);
    return DragMode::None;
}

bool TransferFunctionDisplay::isNearThresholdLine(juce::Point<float> mousePos, juce::Rectangle<float> bounds)
{
    // Determinar el rango de dB según el nivel de zoom
    float minDb, maxDb;
    switch (currentZoom)
    {
        case ZoomLevel::Normal:
            minDb = -100.0f;
            maxDb = 0.0f;
            break;
        case ZoomLevel::Zoomed:
            minDb = -50.0f;
            maxDb = 0.0f;
            break;
    }
    
    // Solo verificar si el threshold está dentro del rango visible
    if (threshold < minDb || threshold > maxDb) return false;
    
    float thresholdX = juce::jmap(threshold, minDb, maxDb, bounds.getX(), bounds.getRight());
    return std::abs(mousePos.x - thresholdX) < 8.0f; // Tolerancia de 8 píxeles
}

bool TransferFunctionDisplay::isNearRangeLine(juce::Point<float> mousePos, juce::Rectangle<float> bounds)
{
    // Calcular si el mouse está cerca de la línea diagonal del range
    auto dbPos = pixelToDb(mousePos, bounds);
    float rangeLineOutput = dbPos.x + range;
    float distanceToRangeLine = std::abs(dbPos.y - rangeLineOutput);
    return distanceToRangeLine < 3.0f; // Tolerancia de 3 dB
}

bool TransferFunctionDisplay::isNearKneeArea(juce::Point<float> mousePos, juce::Rectangle<float> bounds)
{
    if (knee <= 0.0f) return false; // No hay área de knee si es hard knee
    
    // Verificar si el mouse está en la zona del knee
    auto dbPos = pixelToDb(mousePos, bounds);
    float kneeStart = threshold - knee;
    float kneeEnd = threshold + knee;
    
    // Verificar si está dentro del rango horizontal del knee
    return dbPos.x >= kneeStart && dbPos.x <= kneeEnd;
}

bool TransferFunctionDisplay::isNearTransferCurve(juce::Point<float> mousePos, juce::Rectangle<float> bounds)
{
    // Calcular si el mouse está cerca de la curva de transferencia
    auto dbPos = pixelToDb(mousePos, bounds);
    float expectedOutput = calculateOutput(dbPos.x);
    float distanceToCurve = std::abs(dbPos.y - expectedOutput);
    return distanceToCurve < 4.0f; // Tolerancia de 4 dB
}

void TransferFunctionDisplay::drawGainReductionHistory(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    if (!hasWaveformData.load()) return;
    
    // Si Logic está parado, no dibujar el histograma
    if (isLogicStopped.load()) return;
    
    const int readIndex = waveformWriteIndex.load();
    const float currentFadeOutFactor = fadeOutFactor.load(std::memory_order_relaxed);
    
    // Determinar el rango de gain reduction según el nivel de zoom
    // NUEVO: Rangos ajustados para expansor (valores típicos más pequeños que compresor)
    float maxGainReduction;
    bool useNonLinearScale = false;
    switch (currentZoom)
    {
        case ZoomLevel::Normal:
            maxGainReduction = 12.0f;  // Expansor: rango típico más pequeño (0-12dB)
            break;
        case ZoomLevel::Zoomed:
            maxGainReduction = 6.0f;   // Expansor: zoom para rangos pequeños (0-6dB)
            break;
    }
    
    // Crear path para la línea de reducción de ganancia
    juce::Path grPath;
    std::vector<juce::Point<float>> grPoints;
    
    // Recolectar puntos de reducción de ganancia
    for (int i = 0; i < displayPoints; ++i)
    {
        int samplesBack = displayPoints - i + 5;
        int bufferIndex = (readIndex - samplesBack + waveformBufferSize) % waveformBufferSize;
        
        float grDb = gainReductionBuffer[bufferIndex];
        
        // Posición X - usar todo el ancho
        float normalizedTime = float(i) / float(displayPoints - 1);
        float x = bounds.getX() + normalizedTime * bounds.getWidth();
        
        // Posición Y - mapear GR al rango vertical según el zoom
        // Para expansor: manejar valores positivos (amplificación) y negativos (atenuación)
        float normalizedGR;
        float y;
        
        if (grDb >= 0.0f) {
            // Valores positivos (amplificación) - mapear a la mitad superior
            normalizedGR = juce::jlimit(0.0f, 1.0f, grDb / maxGainReduction);
            y = bounds.getY() + (0.5f - normalizedGR * 0.5f) * bounds.getHeight();
        } else {
            // Valores negativos (atenuación) - mapear a la mitad inferior
            normalizedGR = juce::jlimit(0.0f, 1.0f, std::abs(grDb) / maxGainReduction);
            
            if (useNonLinearScale)
            {
                // Aplicar escala no lineal para expandir visualmente el rango 0 a -6 dB
                float expandedGR = std::pow(normalizedGR, 0.75f);  
                y = bounds.getY() + (0.5f + expandedGR * 0.5f) * bounds.getHeight();
            }
            else
            {
                // Escala lineal normal
                y = bounds.getY() + (0.5f + normalizedGR * 0.5f) * bounds.getHeight();
            }
        }
        
        grPoints.push_back({x, y});
    }
    
    // Crear path suavizado
    if (!grPoints.empty())
    {
        // Modo normal: usar colores morados originales
        juce::ColourGradient grGradient(
            DarkTheme::accentSecondary.withAlpha(0.5f * currentFadeOutFactor), bounds.getX(), bounds.getY(),
            DarkTheme::accentSecondary.darker(0.3f).withAlpha(0.3f * currentFadeOutFactor), bounds.getRight(), bounds.getY(),
            false
        );
            
            // Dibujar área bajo la curva - empezar sin offset
            grPath.startNewSubPath(grPoints[0].x, bounds.getY());
            grPath.lineTo(grPoints[0]);
            
            // Suavizado con spline
            for (size_t i = 1; i < grPoints.size(); ++i)
            {
                if (i < grPoints.size() - 1)
                {
                    auto p0 = (i > 1) ? grPoints[i - 2] : grPoints[i - 1];
                    auto p1 = grPoints[i - 1];
                    auto p2 = grPoints[i];
                    auto p3 = (i < grPoints.size() - 1) ? grPoints[i + 1] : grPoints[i];
                    
                    for (float t = 0.0f; t < 1.0f; t += 0.2f)
                    {
                        float t2 = t * t;
                        float t3 = t2 * t;
                        
                        float x = 0.5f * ((2.0f * p1.x) +
                                         (-p0.x + p2.x) * t +
                                         (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                                         (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
                        
                        float y = 0.5f * ((2.0f * p1.y) +
                                         (-p0.y + p2.y) * t +
                                         (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
                                         (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
                        
                        grPath.lineTo(x, y);
                    }
                }
                else
                {
                    grPath.lineTo(grPoints[i]);
                }
            }
        
        grPath.lineTo(grPoints.back().x, bounds.getY());
        grPath.closeSubPath();
        
        // Modo normal: No dibujar área rellena para gain reduction
        // g.setGradientFill(grGradient);
        // g.fillPath(grPath);
        
        // Solo dibujar la línea morada con currentFadeOutFactor
        juce::Path grLine;
        grLine.startNewSubPath(grPoints[0]);
        for (size_t i = 1; i < grPoints.size(); ++i)
        {
            grLine.lineTo(grPoints[i]);
        }
        g.setColour(DarkTheme::accentSecondary.withAlpha(0.9f * currentFadeOutFactor));
        g.strokePath(grLine, juce::PathStrokeType(2.0f)); // Línea un poco más gruesa para mejor visibilidad
    }
    
    // Dibujar línea central de referencia (0 dB - sin ganancia ni atenuación)
    float centerY = bounds.getY() + bounds.getHeight() * 0.5f;
    g.setColour(DarkTheme::textSecondary.withAlpha(0.3f * currentFadeOutFactor));
    g.drawHorizontalLine(static_cast<int>(centerY), bounds.getX(), bounds.getRight());
}

// ELIMINADO: drawDeltaGainReduction - ya no se utiliza el modo DELTA

// ELIMINADO: Métodos de histograma DELTA específico - ya no se utilizan

//==============================================================================
// NUEVOS MÉTODOS PARA ATTACK/SUSTAIN HISTOGRAMS
//==============================================================================


void TransferFunctionDisplay::drawAttackSustainHistograms(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    if (!hasWaveformData.load() || isSilent.load()) return;
    
    const float currentFadeOutFactor = fadeOutFactor.load();
    if (currentFadeOutFactor <= 0.0f) return;
    
    // Configuración de dibujo
    const float topOffset = 20.0f;
    const float baseLine = bounds.getCentreY(); // Línea base en el centro para bipolar
    
    const int currentIndex = waveformWriteIndex.load();
    const float widthStep = bounds.getWidth() / static_cast<float>(displayPoints);
    
    // HISTOGRAMA ATTACK (Naranja - accentWarm)
    {
        juce::Path attackHistogram;
        bool firstPoint = true;
        
        for (int i = 0; i < displayPoints; ++i) {
            // SINCRONIZADO: Usar misma fórmula que drawWaveformAreas
            int samplesBack = displayPoints - i + 5;  // Offset mínimo para sincronía audio-visual
            int readIndex = (currentIndex - samplesBack + waveformBufferSize) % waveformBufferSize;
            const float attackSample = attackGainBuffer[readIndex];
            
            // Mapeo bipolar con amplificación visual en zoom x2
            float visualRange = (currentZoom == ZoomLevel::Zoomed) ? 0.5f : 1.0f; // x2 amplificación en zoom
            const float yPos = juce::jmap(attackSample, -visualRange, visualRange, 
                                        bounds.getBottom() - topOffset, 
                                        bounds.getY() + topOffset);
            const float xPos = bounds.getX() + i * widthStep;
            
            if (firstPoint) {
                attackHistogram.startNewSubPath(xPos, yPos);
                firstPoint = false;
            } else {
                attackHistogram.lineTo(xPos, yPos);
            }
        }
        
        // Dibujar histograma Attack con transparencia (NARANJA)
        g.setColour(DarkTheme::accentWarm.withAlpha(0.6f * currentFadeOutFactor));
        g.strokePath(attackHistogram, juce::PathStrokeType(1.5f));
    }
    
    // HISTOGRAMA SUSTAIN (Azul - accent)
    {
        juce::Path sustainHistogram;
        bool firstPoint = true;
        
        for (int i = 0; i < displayPoints; ++i) {
            // SINCRONIZADO: Usar misma fórmula que drawWaveformAreas
            int samplesBack = displayPoints - i + 5;  // Offset mínimo para sincronía audio-visual
            int readIndex = (currentIndex - samplesBack + waveformBufferSize) % waveformBufferSize;
            const float sustainSample = sustainGainBuffer[readIndex];
            
            // Mapeo bipolar con amplificación visual en zoom x2
            float visualRange = (currentZoom == ZoomLevel::Zoomed) ? 0.5f : 1.0f; // x2 amplificación en zoom
            const float yPos = juce::jmap(sustainSample, -visualRange, visualRange, 
                                        bounds.getBottom() - topOffset, 
                                        bounds.getY() + topOffset);
            const float xPos = bounds.getX() + i * widthStep;
            
            if (firstPoint) {
                sustainHistogram.startNewSubPath(xPos, yPos);
                firstPoint = false;
            } else {
                sustainHistogram.lineTo(xPos, yPos);
            }
        }
        
        // Dibujar histograma Sustain con transparencia
        g.setColour(DarkTheme::accent.withAlpha(0.6f * currentFadeOutFactor));
        g.strokePath(sustainHistogram, juce::PathStrokeType(1.5f));
    }
    
    // Dibujar línea central de referencia (0 = sin ganancia ni atenuación)
    g.setColour(DarkTheme::textSecondary.withAlpha(0.3f * currentFadeOutFactor));
    g.drawHorizontalLine(static_cast<int>(baseLine), bounds.getX(), bounds.getRight());
}
