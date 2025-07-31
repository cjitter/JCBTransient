//==============================================================================
//
//  Copyright 2025 Juan Carlos Blancas
//  This file is part of JCBTransient and is licensed under the GNU General Public License v3.0 or later.
//
//==============================================================================

//==============================================================================
// INCLUDES
//==============================================================================
// Módulos JUCE
#include <juce_graphics/juce_graphics.h>

// Librerías estándar C++
#include <mutex>
#include <algorithm>

// Archivos del proyecto
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Components/Windows/CreditsWindow.h"
#include "Components/Windows/CodeWindow.h"
#include "Helpers/UTF8Helper.h"
#include "BinaryData.h"


//==============================================================================
// CONSTRUCTOR Y DESTRUCTOR
//==============================================================================
JCBTransientAudioProcessorEditor::JCBTransientAudioProcessorEditor (JCBTransientAudioProcessor& p, juce::UndoManager& um)
    : AudioProcessorEditor (&p), 
      processor (p), 
      undoManager (um),
      inputMeterL([&p](){ return p.getRmsInputValue(0); }),
      inputMeterR([&p](){ return p.getRmsInputValue(1); }),
      outputMeterL([&p](){ return p.getRmsOutputValue(0); }),
      outputMeterR([&p](){ return p.getRmsOutputValue(1); }),
      scMeterL([&p](){ return p.getSCValue(0); }), // SC meter L
      scMeterR([&p](){ return p.getSCValue(1); }) // SC meter R
{
    // Configurar todos los componentes
    setupBackground();
    setupKnobs();
    setupButtons();  // Configurar botones FLIP y LINK
    setupMeters();
    setupSidechainControls();
    setupPresetArea();
    setupUtilityButtons();
    setupParameterButtons();
    
    // Agregar display principal
    addAndMakeVisible(transferDisplay);
    
    // Agregar título y versión - mismo estilo que ExpansorGate
    auto titleFont = juce::Font(juce::FontOptions(22.0f));
    titleLink.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    titleLink.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    titleLink.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary.withAlpha(0.7f));
    titleLink.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    
    // Detectar si estamos en Logic Pro para mostrar solo la versión
    juce::String titleText;
    
    // Verificar si el host es Logic Pro
    juce::PluginHostType hostInfo;
    if (hostInfo.isLogic()) {
        titleText = "v0.9.1 beta";  // Solo versión para Logic Pro
    } else {
        titleText = "JCBTransient v0.9.1 beta";  // Nombre completo para otros DAWs
    }
    
    titleLink.setButtonText(titleText);
    
    // NO agregar tooltip individual - solo usar la ventana general de tooltips
    // El tooltip se actualiza dinámicamente en updateTooltips() usando getTooltipText("title")
    titleLink.setTooltip("");
    
    // Configurar el click para mostrar créditos
    titleLink.onClick = [this]() {
        showCredits();
    };
    
    addAndMakeVisible(titleLink);
    
    
    // Agregar tooltip
    addAndMakeVisible(tooltipComponent);
    tooltipComponent.setAlwaysOnTop(true);
    tooltipComponent.showTip(JUCE_UTF8("Pasa el mouse sobre los controles\npara ver ayuda detallada"));
    
    
    // Establecer tamaño inicial con restricciones
    auto savedSize = processor.getSavedSize();
    int initialWidth = savedSize.x > 0 ? savedSize.x : DEFAULT_WIDTH;
    int initialHeight = savedSize.y > 0 ? savedSize.y : DEFAULT_HEIGHT;
    
    setSize(initialWidth, initialHeight);
    setResizable(true, true);
    
    // Inicializar el estado del tamaño basado en las dimensiones iniciales
    if (initialWidth == DEFAULT_WIDTH && initialHeight == DEFAULT_HEIGHT) {
        currentSizeState = GuiSizeState::Current;
    } else if (initialWidth == MIN_WIDTH && initialHeight == MIN_HEIGHT) {
        currentSizeState = GuiSizeState::Minimum;
    } else if (initialWidth == MAX_WIDTH && initialHeight == MAX_HEIGHT) {
        currentSizeState = GuiSizeState::Maximum;
    } else {
        // Para tamaños personalizados, usar estado Current por defecto
        currentSizeState = GuiSizeState::Current;
        lastCustomSize = {initialWidth, initialHeight};
    }

    // Establecer límites de redimensionado - valores exactos de JCBExpansorGate
    setResizeLimits(MIN_WIDTH, MIN_HEIGHT, MAX_WIDTH, MAX_HEIGHT);
    getConstrainer()->setFixedAspectRatio(ASPECT_RATIO);
    
    // Inicializar cache de contenido de código para thread safety
    initializeCodeContentCache();
    
    // Iniciar timer para updates - 90Hz para mayor consistencia visual
    startTimerHz(TIMER_HZ);
    
    // Timer para resetear indicadores de clip cada cierto tiempo
    clipResetCounter = 0;
    
    // Inicializar transfer display con valores actuales
    float thresholdDB = leftTopKnobs.attackSlider.getValue();
    transferDisplay.setThreshold(thresholdDB);
    transferDisplay.setRatio(leftTopKnobs.sustainSlider.getValue());
    transferDisplay.setKnee(leftTopKnobs.sensSlider.getValue());
    // ELIMINADO: setRange - ahora dmodeButton maneja h_DELTAMODE directamente
    
    // Conectar callbacks del transfer display para actualizar knobs
    transferDisplay.onThresholdChange = [this](float newThreshold) {
        leftTopKnobs.attackSlider.setValue(newThreshold, juce::sendNotification);
        handleParameterChange();  // Marcar preset como modificado
    };
    transferDisplay.onRatioChange = [this](float newRatio) {
        leftTopKnobs.sustainSlider.setValue(newRatio, juce::sendNotification);
        handleParameterChange();  // Marcar preset como modificado
    };
    transferDisplay.onKneeChange = [this](float newKnee) {
        leftTopKnobs.sensSlider.setValue(newKnee, juce::sendNotification);
        handleParameterChange();  // Marcar preset como modificado
    };
    // ELIMINADO: onRangeChange - dmodeButton no usa callbacks de transferDisplay
    
    // Updates iniciales
    updateTransferDisplay();
    
    // Crear y registrar parameter listener para updates de automatización
    transferFunctionListener = std::make_unique<TransferFunctionParameterListener>(this);
    processor.apvts.addParameterListener("b_THD", transferFunctionListener.get());
    processor.apvts.addParameterListener("c_RATIO", transferFunctionListener.get());
    processor.apvts.addParameterListener("q_KNEE", transferFunctionListener.get());
    
    // Configurar estado inicial del idioma
    if (processor.getTooltipLanguageEnglish()) {
        currentLanguage = TooltipLanguage::English;
        utilityButtons.tooltipLangButton.setButtonText("eng");
        utilityButtons.tooltipLangButton.setColour(juce::TextButton::textColourOffId, DarkTheme::accentSecondary);
    } else {
        currentLanguage = TooltipLanguage::Spanish;
        utilityButtons.tooltipLangButton.setButtonText("esp");
        utilityButtons.tooltipLangButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
    }



    // Actualizar todos los tooltips basado en idioma inicial
    updateAllTooltips();
    
    // Configurar AUTO RELEASE button
    
    // ELIMINADO: Configuración especial inicial para DELTA - ahora funciona como botón normal
    parameterButtons.deltaButton.setButtonText("SOLO DELTA");  // Texto actualizado
    
    updateButtonStates();
    updateFilterButtonText();  // Establecer texto inicial de botones de filtro
    updateDmodeButtonText();   // Establecer texto inicial del botón DMODE
    
    // Establecer estado inicial de SOLO SIDECHAIN en transfer display
    transferDisplay.setSoloSidechain(sidechainControls.soloScButton.getToggleState());
    
    // Configurar estado inicial del botón run graphics basado en el processor
    bool initialEnvelopeState = processor.getEnvelopeVisualEnabled();
    // El estado del processor indica si las envolventes están visibles
    // Si las envolventes están visibles, graphics debe estar OFF (false)
    bool graphicsButtonState = !initialEnvelopeState;
    transferDisplay.setEnvelopeVisible(initialEnvelopeState);
    utilityButtons.runGraphicsButton.setToggleState(graphicsButtonState, juce::dontSendNotification);
    utilityButtons.runGraphicsButton.setButtonText("graphics");
    
    // Actualizar valores de sliders desde APVTS para evitar problemas al cargar sesión
    // Usar MessageManager::callAsync para ejecución thread-safe sin delay
    juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<JCBTransientAudioProcessorEditor>(this)]() {
        if (safeThis) {
            safeThis->updateSliderValues();
        }
    });
    
    // Configurar estado inicial del tooltip toggle y language button
    bool initialTooltipState = processor.getTooltipEnabled();
    tooltipsEnabled = initialTooltipState;
    tooltipComponent.setTooltipsEnabled(tooltipsEnabled);
    utilityButtons.tooltipToggleButton.setToggleState(tooltipsEnabled, juce::dontSendNotification);
    
    // Habilitar/deshabilitar botón de idioma según estado de tooltips
    utilityButtons.tooltipLangButton.setEnabled(tooltipsEnabled);
    utilityButtons.tooltipLangButton.setAlpha(tooltipsEnabled ? 1.0f : 0.25f);
    
    // Aplicar estados iniciales de todos los modos en el orden correcto
    updateButtonStates();
    
    
    // Inicializar texto del botón AR basado en el estado actual del parámetro
    
}

JCBTransientAudioProcessorEditor::~JCBTransientAudioProcessorEditor()
{
    // CRÍTICO: Detener timer PRIMERO para prevenir crashes durante destrucción
    stopTimer();
    
    // Eliminar parameter listeners
    if (transferFunctionListener)
    {
        processor.apvts.removeParameterListener("b_THD", transferFunctionListener.get());
        processor.apvts.removeParameterListener("c_RATIO", transferFunctionListener.get());
        processor.apvts.removeParameterListener("q_KNEE", transferFunctionListener.get());
    }
    
    // Remover listener custom de DMODE
    processor.apvts.removeParameterListener("h_DELTAMODE", this);
    
    setLookAndFeel(nullptr);
}

//==============================================================================
// OVERRIDES DE JUCE  
//==============================================================================
void JCBTransientAudioProcessorEditor::paint (juce::Graphics& g)
{
    // El background se maneja por el componente backgroundImage
}

void JCBTransientAudioProcessorEditor::paintOverChildren (juce::Graphics& g)
{
    // Usar las mismas coordenadas que la transfer function display
    float x = 460.0f * 700.0f / 1247.0f;  // ≈ 258
    float y = 73.0f * 200.0f / 353.0f + 4.0f;    // ≈ 41 + 4 píxeles
    float w = 335.0f * 700.0f / 1247.0f;  // ≈ 188
    float h = 205.0f * 200.0f / 353.0f - 5.0f;   // ≈ 116 - 5 píxeles
    auto transferBounds = getScaledBounds(x, y, w, h).toFloat();
    
    // Texto BYPASS
    if (isBypassed && bypassTextVisible) {
        g.setColour(juce::Colours::transparentWhite.withAlpha(0.85f));
        g.setFont(juce::Font(juce::FontOptions(transferBounds.getHeight() * 0.4f))
                  .withStyle(juce::Font::bold));
        g.drawText("BYPASS", transferBounds, juce::Justification::centred);
    }
    
    // ELIMINADO: Texto DELTA - ya no se muestra overlay de texto
    
    // Texto SOLO SIDECHAIN con info de filtros (cuando está activo)
    if (sidechainControls.soloScButton.getToggleState()) {
        g.setColour(juce::Colours::transparentWhite.withAlpha(0.85f));
        
        // Mostrar diferente texto según EXT KEY y estado de filtros
        juce::String soloText;
        bool filtersActive = sidechainControls.scButton.getToggleState();
        
        if (sidechainControls.keyButton.getToggleState()) {
            soloText = "SOLO EXT SC";
        } else {
            soloText = "SOLO INT SC";
        }
        
        // Texto principal en tamaño más grande
        g.setFont(juce::Font(juce::FontOptions(transferBounds.getHeight() * 0.25f))
                  .withStyle(juce::Font::bold));
        
        // Dibujar texto principal en la parte superior
        auto upperBounds = transferBounds;
        upperBounds = upperBounds.removeFromTop(transferBounds.getHeight() * 0.6f);
        g.drawText(soloText, upperBounds, juce::Justification::centred);
        
        // Texto de filtros en tamaño más pequeño
        g.setFont(juce::Font(juce::FontOptions(transferBounds.getHeight() * 0.15f)));
        g.setColour(juce::Colours::transparentWhite.withAlpha(0.7f));
        
        juce::String filterText = filtersActive ? "FILTERS ON" : "FILTERS OFF";
        auto lowerBounds = transferBounds;
        lowerBounds = lowerBounds.removeFromBottom(transferBounds.getHeight() * 0.4f);
        g.drawText(filterText, lowerBounds, juce::Justification::centred);
    }
}

void JCBTransientAudioProcessorEditor::resized()
{
    // Guardar tamaño de ventana
    processor.setSavedSize({getWidth(), getHeight()});
    
    // Si el usuario redimensiona manualmente (no es uno de los tamaños predefinidos),
    // guardar como tamaño personalizado para poder volver a él
    int currentWidth = getWidth();
    int currentHeight = getHeight();

    if (currentWidth != DEFAULT_WIDTH && currentWidth != MIN_WIDTH && currentWidth != MAX_WIDTH &&
        currentHeight != DEFAULT_HEIGHT && currentHeight != MIN_HEIGHT && currentHeight != MAX_HEIGHT) {
        // Es un tamaño personalizado - guardarlo
        lastCustomSize = {currentWidth, currentHeight};
    }
    
    // El background llena toda el área
    backgroundImage.setBounds(getLocalBounds());
    
    // === METERS CON SLIDERS DE TRIM/MAKEUP ===
    // Meters de entrada (lado izquierdo)
    inputMeterL.setBounds(getScaledBounds(2, 42, 12, 117));
    inputMeterR.setBounds(getScaledBounds(12, 42, 12, 117));
    
    // Meters de sidechain - ahora directamente adyacentes a los meters de entrada (sin gap)
    scMeterL.setBounds(getScaledBounds(24, 42, 12, 117));
    scMeterR.setBounds(getScaledBounds(34, 42, 12, 117));
    
    // Sliders de trim superpuestos a los meters
    trimSlider.setBounds(getScaledBounds(2, 40, 22, 130));  // Altura expandida para TextBox integrado
    
    // Sliders de trim de sidechain superpuestos a los medidores de sidechain
    scTrimSlider.setBounds(getScaledBounds(24, 40, 22, 130));  // Altura expandida para TextBox integrado
    
    // Medidor GR (centro-derecha) - más delgado y alto, posición final ajustada
    
    // Medidores de salida (lado derecho)
    outputMeterL.setBounds(getScaledBounds(677, 42, 12, 117));
    outputMeterR.setBounds(getScaledBounds(687, 42, 12, 117));
    
    // Sliders de makeup superpuestos a los medidores de salida
    makeupSlider.setBounds(getScaledBounds(677, 40, 22, 130));  // Altura expandida para TextBox integrado
    
    
    // === TRANSFER FUNCTION DISPLAY (CENTER) ===
    float x = 460.0f * 700.0f / 1247.0f;
    float y = 73.0f * 200.0f / 353.0f + 4.0f;
    float w = 335.0f * 700.0f / 1247.0f;
    float h = 205.0f * 200.0f / 353.0f - 5.0f;
    transferDisplay.setBounds(getScaledBounds(x, y, w, h));
    // === LEFT SIDE KNOBS === (Between SC meters and transfer function)
    // Top row - TRAN, SUST, SENS (RANGE movido al lado derecho)
    leftTopKnobs.sensSlider.setBounds(getScaledBounds(50, 48, 53, 53));
    leftTopKnobs.attackSlider.setBounds(getScaledBounds(100, 48, 53, 53));
    leftTopKnobs.sustainSlider.setBounds(getScaledBounds(150, 48, 53, 53));
    
    // Botón FLIP - ubicado entre SUST y SENS verticalmente
    leftTopKnobs.flipButton.setBounds(getScaledBounds(205, 67, 30, 20));  // Centrado verticalmente

    // Bottom row - D/W, LA, CLIP (AGAIN está en fila superior)
    leftBottomKnobs.drywetSlider.setBounds(getScaledBounds(74, 100, 53, 53));
    leftBottomKnobs.lookaheadSlider.setBounds(getScaledBounds(124, 100, 53, 53));
    leftBottomKnobs.clipSlider.setBounds(getScaledBounds(174, 100, 53, 53));

    // === RIGHT SIDE CONTROLS ===
    // Top row - RANGE, REACT, SMOOTH knobs
    rightTopControls.dmodeButton.setBounds(getScaledBounds(530, 67, 40, 20));
    rightTopControls.smoothSlider.setBounds(getScaledBounds(575, 48, 53, 53));

    // Bottom row - Attack, Release, Hold
    rightBottomKnobs.atkSlider.setBounds(getScaledBounds(468, 100, 53, 53));
    rightBottomKnobs.holdSlider.setBounds(getScaledBounds(532, 100, 53, 53));
    rightBottomKnobs.relSlider.setBounds(getScaledBounds(605, 100, 53, 53));

    // === SIDECHAIN CONTROLS (TOP CENTER) ===
    // HPF and LPF knobs swapped with their order buttons
    // Botón orden HPF ahora a la izquierda, perilla HPF se mueve a la derecha
    sidechainControls.hpfOrderButton.setBounds(getScaledBounds(265, 14, 24, 12));  // Was HPF knob position, moved up for icon space
    sidechainControls.hpfSlider.setBounds(getScaledBounds(290, 5, 36, 36));        // Era posición del botón de orden, expandido
    
    // Botón orden LPF ahora a la derecha, perilla LPF se mueve a la izquierda  
    sidechainControls.lpfSlider.setBounds(getScaledBounds(378, 5, 36, 36));        // Era posición del botón de orden, expandido
    sidechainControls.lpfOrderButton.setBounds(getScaledBounds(415, 14, 24, 12));  // Was LPF knob position, moved up for icon space
    
    // Filter icons below the order buttons
    hpfIcon.setBounds(getScaledBounds(265, 28, 24, 12));  // Debajo del botón de orden HPF
    lpfIcon.setBounds(getScaledBounds(415, 28, 24, 12));  // Debajo del botón de orden LPF
    
    
    // Botones SC, KEY y SOLO en el medio - mismo ancho, centrados en 355 (movidos a la derecha)
    // Movido arriba 1 píxel
    const int buttonWidth = 50;
    const int centerX = 353;
    // Posicionamiento del botón SC
    sidechainControls.scButton.setBounds(getScaledBounds(centerX - buttonWidth/2, 5, buttonWidth, 12));
    sidechainControls.keyButton.setBounds(getScaledBounds(centerX - buttonWidth/2, 17, buttonWidth, 12));
    sidechainControls.soloScButton.setBounds(getScaledBounds(centerX - buttonWidth/2, 29, buttonWidth, 12));
    
    // === PRESET AREA (TOP LEFT) ===
    presetArea.saveButton.setBounds(getScaledBounds(15, 15, 20, 12));  // Alineado con undo
    presetArea.saveAsButton.setBounds(getScaledBounds(37, 15, 25, 12));
    presetArea.deleteButton.setBounds(getScaledBounds(64, 15, 25, 12));
    presetArea.backButton.setBounds(getScaledBounds(91, 15, 18, 12));
    presetArea.nextButton.setBounds(getScaledBounds(112, 15, 18, 12));
    presetArea.presetMenu.setBounds(getScaledBounds(133, 15, 65, 12));
    
    // Botones A/B junto al menú de preset
    topButtons.abStateButton.setBounds(getScaledBounds(202, 15, 18, 12));
    topButtons.abCopyButton.setBounds(getScaledBounds(222, 15, 22, 12));
    
    // === BOTONES DE UTILIDAD (INFERIOR IZQUIERDA) ===
    utilityButtons.undoButton.setBounds(getScaledBounds(15, 179, 22, 12));
    utilityButtons.redoButton.setBounds(getScaledBounds(37, 179, 22, 12));
    utilityButtons.resetGuiButton.setBounds(getScaledBounds(61, 179, 30, 12));
    utilityButtons.runGraphicsButton.setBounds(getScaledBounds(93, 179, 30, 12));
    utilityButtons.zoomButton.setBounds(getScaledBounds(125, 179, 30, 12));
    // Mover botones de tooltip y lenguaje donde estaba el diagram
    utilityButtons.tooltipToggleButton.setBounds(getScaledBounds(157, 179, 30, 12));
    utilityButtons.tooltipLangButton.setBounds(getScaledBounds(189, 179, 22, 12));
    
    // Botones DELTA, DIAGRAM y BYPASS - DIAGRAM alineado con botones de sidechain arriba
    const int centerButtonsY = 163;
    const int diagramCenterX = 355; // Mismo centerX que botones de sidechain
    const int buttonSpacing = 45;   // Espaciado entre DELTA-DIAGRAM y DIAGRAM-BYPASS
    
    // DIAGRAM centrado en mismo X que botones de sidechain (FILTERS, EXT KEY, SOLO SC)
    centerButtons.diagramButton.setBounds(getScaledBounds(diagramCenterX - 45, centerButtonsY, 40, 12));
    // DELTA movido a la izquierda del slider RANGE - botón rectangular más alto que ancho
    parameterButtons.deltaButton.setBounds(getScaledBounds(472, 67, 52, 20));
    // BYPASS a la derecha de DIAGRAM  
    parameterButtons.bypassButton.setBounds(getScaledBounds(diagramCenterX + buttonSpacing - 45, centerButtonsY, 40, 12));
    
    // Botones TODO movidos abajo a la derecha, centrados en el rectángulo
    // Calcular posición central para el grupo de botones TODO
    const int todoStartX = 503;    // Movido a la derecha para mejor centrado en el rectángulo
    const int todoY = 179;       // Mismo Y que botones de utilidad
    utilityButtons.hqButton.setBounds(getScaledBounds(todoStartX, todoY, 18, 12));
    utilityButtons.dualMonoButton.setBounds(getScaledBounds(todoStartX + 20, todoY, 23, 12));
    utilityButtons.stereoLinkedButton.setBounds(getScaledBounds(todoStartX + 45, todoY, 23, 12));
    utilityButtons.msButton.setBounds(getScaledBounds(todoStartX + 70, todoY, 18, 12));
    // Botón MIDI posicionado a la derecha del botón M/S
    utilityButtons.midiLearnButton.setBounds(getScaledBounds(todoStartX + 90, todoY, 23, 12));
    
    // === TÍTULO Y VERSIÓN (CENTRO INFERIOR) ===
    // Ancho similar a botones DIAGRAM + GEN DSP combinados
    titleLink.setBounds(getScaledBounds(305, 179, 95, 15));
    
    // Tooltip en esquina superior derecha - ajustado al rectángulo visible
    tooltipComponent.setBounds(getScaledBounds(450, 0, 228, 42));
    
    // Actualizar factor de escala del tooltip basado en el tamaño de la ventana
    float scaleFactor = (float)getWidth() / (float)DEFAULT_WIDTH;
    tooltipComponent.setScaleFactor(scaleFactor);
    
    // Redimensionar el overlay del diagrama si está visible
    if (diagramOverlay != nullptr)
    {
        diagramOverlay->setBounds(getLocalBounds());
        diagramOverlay->invalidateClickableAreasCache(); // Invalidar cache para regenerar con nuevas coordenadas escaladas
    }
    
    // Redimensionar el overlay de créditos si está visible
    if (creditsOverlay != nullptr)
    {
        creditsOverlay->setBounds(getLocalBounds());
    }
    
    // Redimensionar y reposicionar la ventana de código si está visible
    if (codeWindow != nullptr && codeWindow->isVisible())
    {
        // Recalcular tamaño responsivo
        int windowWidth = static_cast<int>(getWidth() * 0.35f);
        int windowHeight = static_cast<int>(getHeight() * 0.50f);
        
        // Limitar tamaños
        windowWidth = juce::jlimit(350, 600, windowWidth);
        windowHeight = juce::jlimit(250, 450, windowHeight);
        
        // Recentrar la ventana
        int x = getWidth() / 2 - windowWidth / 2;
        int y = getHeight() / 2 - windowHeight / 2;
        
        codeWindow->setBounds(x, y, windowWidth, windowHeight);
    }
}

void JCBTransientAudioProcessorEditor::timerCallback()
{
    // Sistema universal de decay para todos los DAWs
    applyMeterDecayIfNeeded();
    
    updateMeters();
    
    // Actualizar estado visual de botones undo/redo
    bool canUndo = undoManager.canUndo();
    bool canRedo = undoManager.canRedo();
    
    // Actualizar apariencia del botón undo
    utilityButtons.undoButton.setEnabled(canUndo);
    utilityButtons.undoButton.setAlpha(canUndo ? 1.0f : 0.3f);
    utilityButtons.undoButton.setColour(juce::TextButton::textColourOffId, 
                                       canUndo ? DarkTheme::textPrimary : DarkTheme::textSecondary);
    
    // Actualizar apariencia del botón redo
    utilityButtons.redoButton.setEnabled(canRedo);
    utilityButtons.redoButton.setAlpha(canRedo ? 1.0f : 0.3f);
    utilityButtons.redoButton.setColour(juce::TextButton::textColourOffId, 
                                       canRedo ? DarkTheme::textPrimary : DarkTheme::textSecondary);
    
    // Actualizar texto y tooltip del botón A/B copy dinámicamente
    bool isSpanish = !processor.getTooltipLanguageEnglish();
    if (processor.getIsStateA()) {
        topButtons.abCopyButton.setButtonText("A-B");
        topButtons.abCopyButton.setTooltip(isSpanish ? "Copiar A a B" : "Copy A to B");
    } else {
        topButtons.abCopyButton.setButtonText("B-A");
        topButtons.abCopyButton.setTooltip(isSpanish ? "Copiar B a A" : "Copy B to A");
    }
    
    // Resetear indicadores de clip cada 3 segundos (270 frames a 90Hz)
    clipResetCounter++;
    if (clipResetCounter >= 270) {
        processor.resetClipIndicators();
        clipResetCounter = 0;
    }
    
    
    // Pasar el estado de Logic parado al display
    // Determinar si el procesamiento está inactivo
    // Usar sistema híbrido optimizado (timestamp + playhead + audio tail)
    bool isProcessingInactive = !processor.isPlaybackActive();
    
    // Sistema original comentado en processBlock:
    // bool isProcessingInactive = processor.getIsLogicStopped();
    transferDisplay.setLogicStoppedState(isProcessingInactive);
    
    // Actualizar datos de waveform y obtener gain reduction para el medidor
    if (!isBypassed && !isProcessingInactive)
    {
        // Obtener datos de waveform si las envolventes están visibles
        if (transferDisplay.isEnvelopeVisible())
        {
            std::vector<float> inputSamples, processedSamples;
            processor.getWaveformData(inputSamples, processedSamples);
            
            if (!inputSamples.empty() && !processedSamples.empty())
            {
                // Actualizar estado EXT KEY y nivel de sidechain
                bool keyEnabled = sidechainControls.keyButton.getToggleState();
                transferDisplay.setExtKeyActive(keyEnabled);
                
                // Obtener nivel promedio de sidechain desde los medidores
                float scLevel = (processor.getSCValue(0) + processor.getSCValue(1)) * 0.5f;
                transferDisplay.setSidechainLevel(scLevel);
                
                // Obtener datos de Attack/Sustain gains
                std::vector<float> attackSamples, sustainSamples;
                processor.getAttackSustainData(attackSamples, sustainSamples);
                
                // LLAMADA UNIFICADA: Enviar todos los datos sincronizados
                if (!attackSamples.empty() && !sustainSamples.empty()) {
                    transferDisplay.updateWaveformData(&inputSamples[0], &processedSamples[0], &attackSamples[0], &sustainSamples[0], 1);
                } else {
                    // Fallback si no hay datos Attack/Sustain
                    transferDisplay.updateWaveformData(&inputSamples[0], &processedSamples[0], 1);
                }
                
                // Forzar repintado para mostrar envolvente actualizada
                transferDisplay.repaint();
            }
        }
    }
    else
    {
        // Forzar repaint del transfer display cuando Logic está parado
        // para que las envolventes desaparezcan
        if (isProcessingInactive)
        {
            transferDisplay.repaint();
        }
    }
}

void JCBTransientAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    // Simplificado - sin manejo automático de cambios de parámetros para undo/redo
    
    if (button == &utilityButtons.undoButton)
    {
        undoManager.undo();
    }
    else if (button == &utilityButtons.redoButton)
    {
        undoManager.redo();
    }
    else if (button == &utilityButtons.resetGuiButton)
    {
        // Resetear tamaño de GUI, no parámetros
        resetGuiSize();
    }
    else if (button == &parameterButtons.bypassButton)
    {
        if (parameterButtons.bypassButton.getToggleState()) {
            // BYPASS solo desactiva SOLO SC y DIAGRAM - DELTA ahora es independiente
            sidechainControls.soloScButton.setToggleState(false, juce::sendNotification);
            if (centerButtons.diagramButton.getToggleState()) {
                centerButtons.diagramButton.setToggleState(false, juce::dontSendNotification);
                hideDiagram(); // Cerrar DIAGRAM si está abierto
            }
        }
        updateButtonStates();
        
        // Actualizar transfer display para ocultar/mostrar envolventes basado en estado de bypass
        bool bypassActive = parameterButtons.bypassButton.getToggleState();
        transferDisplay.setBypassMode(bypassActive);
        
        // Actualizar output meters para usar gradient de entrada cuando bypass está activo
        outputMeterL.setBypassMode(bypassActive);
        outputMeterR.setBypassMode(bypassActive);
    }
    else if (button == &sidechainControls.scButton)
    {
        // Actualizar texto del botón y visibilidad del filtro
        updateButtonStates();
    }
    else if (button == &sidechainControls.keyButton)
    {
        // Actualizar visibilidad del medidor de sidechain cuando KEY es activado
        updateButtonStates();
        
        // Actualizar estado EXT KEY en transfer display
        transferDisplay.setExtKeyActive(sidechainControls.keyButton.getToggleState());
    }
    else if (button == &sidechainControls.soloScButton)
    {
        if (sidechainControls.soloScButton.getToggleState()) {
            // SOLO SC solo desactiva BYPASS y DIAGRAM - DELTA ahora es independiente
            parameterButtons.bypassButton.setToggleState(false, juce::sendNotification);
            if (centerButtons.diagramButton.getToggleState()) {
                centerButtons.diagramButton.setToggleState(false, juce::dontSendNotification);
                hideDiagram(); // Cerrar DIAGRAM si está abierto
            }
        }
        
        updateButtonStates();
        
        // Actualizar transfer function display para estado SOLO SC
        bool soloActive = sidechainControls.soloScButton.getToggleState();
        transferDisplay.setSoloSidechain(soloActive);
    }
    else if (button == &utilityButtons.runGraphicsButton)
    {
        bool newState = utilityButtons.runGraphicsButton.getToggleState();
        // Invertir la lógica: cuando graphics está ON, ocultar visualizaciones
        transferDisplay.setEnvelopeVisible(!newState);
        processor.setEnvelopeVisualEnabled(!newState);
        // Cuando graphics está activo, ya no hay grMeter que ocultar
        // Mantener el texto siempre como "graphics"
    }
    // Botones de gestión de presets
    else if (button == &presetArea.saveButton)
    {
        // Desactivar botones momentáneos antes de guardar - DELTA ya se guarda en presets
        parameterButtons.bypassButton.setToggleState(false, juce::sendNotification);
        sidechainControls.soloScButton.setToggleState(false, juce::sendNotification);
        savePresetFile();
    }
    else if (button == &presetArea.saveAsButton)
    {
        // Desactivar botones momentáneos antes de guardar - DELTA ya se guarda en presets
        parameterButtons.bypassButton.setToggleState(false, juce::sendNotification);
        sidechainControls.soloScButton.setToggleState(false, juce::sendNotification);
        saveAsPresetFile();
    }
    else if (button == &presetArea.deleteButton)
    {
        // Desactivar botones momentáneos antes de mostrar el diálogo - DELTA ahora se guarda en presets
        parameterButtons.bypassButton.setToggleState(false, juce::sendNotification);
        sidechainControls.soloScButton.setToggleState(false, juce::sendNotification);
        deletePresetFile();
    }
    else if (button == &presetArea.backButton)
    {
        // Desactivar botones momentáneos antes de cambiar preset - DELTA ahora se guarda en presets
        parameterButtons.bypassButton.setToggleState(false, juce::sendNotification);
        sidechainControls.soloScButton.setToggleState(false, juce::sendNotification);
        selectPreviousPreset();
    }
    else if (button == &presetArea.nextButton)
    {
        // Desactivar botones momentáneos antes de cambiar preset - DELTA ahora se guarda en presets
        parameterButtons.bypassButton.setToggleState(false, juce::sendNotification);
        sidechainControls.soloScButton.setToggleState(false, juce::sendNotification);
        selectNextPreset();
    }
    else if (button == &utilityButtons.tooltipToggleButton)
    {
        // Alternar visibilidad del tooltip
        tooltipsEnabled = utilityButtons.tooltipToggleButton.getToggleState();
        tooltipComponent.setTooltipsEnabled(tooltipsEnabled);
        
        // Guardar estado del tooltip al processor
        processor.setTooltipEnabled(tooltipsEnabled);
        
        // Habilitar/deshabilitar botón de idioma basado en estado del tooltip
        utilityButtons.tooltipLangButton.setEnabled(tooltipsEnabled);
        utilityButtons.tooltipLangButton.setAlpha(tooltipsEnabled ? 1.0f : 0.25f);
    }
    else if (button == &utilityButtons.tooltipLangButton)
    {
        // Alternar idioma
        currentLanguage = (currentLanguage == TooltipLanguage::Spanish) ? 
                         TooltipLanguage::English : TooltipLanguage::Spanish;
        
        // Actualizar texto del botón
        utilityButtons.tooltipLangButton.setButtonText(
            currentLanguage == TooltipLanguage::Spanish ? "esp" : "eng");
        
        // Actualizar color del botón basado en el idioma
        if (currentLanguage == TooltipLanguage::English) {
            // Color azul para inglés
            utilityButtons.tooltipLangButton.setColour(juce::TextButton::textColourOffId, DarkTheme::accentSecondary);
        } else {
            // Color por defecto para español
            utilityButtons.tooltipLangButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
        }
        
        // Guardar preferencia de idioma
        processor.setTooltipLanguageEnglish(currentLanguage == TooltipLanguage::English);
        
        // Actualizar todos los tooltips con el nuevo idioma
        updateAllTooltips();
        
        // Actualizar textos de botones TODO con el nuevo idioma
        updateTodoButtonTexts();
    }
    else if (button == &utilityButtons.hqButton)
    {
        // TODO: Implementar oversampling HQ
    }
    else if (button == &utilityButtons.dualMonoButton)
    {
        // Manejar exclusividad de botones de modo estéreo
        if (utilityButtons.dualMonoButton.getToggleState())
        {
            // utilityButtons.stereoLinkedButton.setToggleState(false, juce::dontSendNotification); // LINK siempre está activo
            utilityButtons.msButton.setToggleState(false, juce::dontSendNotification);
        }
    }
    /* Botón LINK siempre está ON y deshabilitado - no necesita manejador
    else if (button == &utilityButtons.stereoLinkedButton)
    {
        // Manejar exclusividad de botones de modo estéreo
        if (utilityButtons.stereoLinkedButton.getToggleState())
        {
            utilityButtons.dualMonoButton.setToggleState(false, juce::dontSendNotification);
            utilityButtons.msButton.setToggleState(false, juce::dontSendNotification);
        }
    }
    */
    else if (button == &utilityButtons.msButton)
    {
        // Manejar exclusividad de botones de modo estéreo
        if (utilityButtons.msButton.getToggleState())
        {
            utilityButtons.dualMonoButton.setToggleState(false, juce::dontSendNotification);
            // utilityButtons.stereoLinkedButton.setToggleState(false, juce::dontSendNotification); // LINK siempre está activo
        }
    }
    else if (button == &topButtons.abStateButton)
    {
        // Alternar estado A/B
        processor.toggleAB();
        
        // Actualizar texto del botón para mostrar estado actual
        topButtons.abStateButton.setButtonText(processor.getIsStateA() ? "A" : "B");
        
        // Retroalimentación visual con colores más evidentes
        if (processor.getIsStateA()) {
            // A = Púrpura/Violeta
            topButtons.abStateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff9c27b0));  // Purple
            topButtons.abStateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        } else {
            // B = Azul
            topButtons.abStateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2196f3));  // Blue
            topButtons.abStateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        }
        
        repaint();
    }
    else if (button == &topButtons.abCopyButton)
    {
        // Copiar estado actual al otro
        if (processor.getIsStateA()) {
            processor.copyAtoB();
            // Retroalimentación visual - destello púrpura a azul
            topButtons.abCopyButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2196f3));  // Destello azul
            topButtons.abCopyButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            // Usar SafePointer para prevenir crashes si el editor se destruye antes de que se dispare el timer
            juce::Component::SafePointer<JCBTransientAudioProcessorEditor> safeThis(this);
            juce::Timer::callAfterDelay(300, [safeThis]() {
                if (safeThis) {
                    safeThis->topButtons.abCopyButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
                    safeThis->topButtons.abCopyButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
                }
            });
        } else {
            processor.copyBtoA();
            // Retroalimentación visual - destello azul a púrpura
            topButtons.abCopyButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff9c27b0));  // Destello púrpura
            topButtons.abCopyButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            // Usar SafePointer para prevenir crashes si el editor se destruye antes de que se dispare el timer
            juce::Component::SafePointer<JCBTransientAudioProcessorEditor> safeThis(this);
            juce::Timer::callAfterDelay(300, [safeThis]() {
                if (safeThis) {
                    safeThis->topButtons.abCopyButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
                    safeThis->topButtons.abCopyButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
                }
            });
        }
    }
    else if (button == &utilityButtons.midiLearnButton)
    {
        // TODO: Implementar modo MIDI learn
    }
    else if (button == &utilityButtons.zoomButton)
    {
        // Ciclar entre dos niveles de zoom
        auto currentZoom = transferDisplay.getZoomLevel();
        TransferFunctionDisplay::ZoomLevel newZoom;
        
        switch (currentZoom)
        {
            case TransferFunctionDisplay::ZoomLevel::Normal:
                // Normal -> Ampliado
                newZoom = TransferFunctionDisplay::ZoomLevel::Zoomed;
                utilityButtons.zoomButton.setButtonText("zoom x2");
                utilityButtons.zoomButton.setToggleState(true, juce::dontSendNotification);
                // Ya no hay grMeter para sincronizar
                break;
                
            case TransferFunctionDisplay::ZoomLevel::Zoomed:
                // Ampliado -> Normal
                newZoom = TransferFunctionDisplay::ZoomLevel::Normal;
                utilityButtons.zoomButton.setButtonText("zoom");
                utilityButtons.zoomButton.setToggleState(false, juce::dontSendNotification);
                // Ya no hay grMeter para sincronizar
                break;
                
            default:
                // Por defecto a Normal si algo sale mal
                newZoom = TransferFunctionDisplay::ZoomLevel::Normal;
                utilityButtons.zoomButton.setButtonText("zoom");
                utilityButtons.zoomButton.setToggleState(false, juce::dontSendNotification);
                // Ya no hay grMeter para sincronizar
                break;
        }
        
        transferDisplay.setZoomLevel(newZoom);
    }
    else if (button == &centerButtons.diagramButton)
    {
        // Anti-rebote para evitar clicks múltiples
        juce::uint32 currentTime = juce::Time::getMillisecondCounter();
        if (currentTime - lastDiagramButtonTime < DIAGRAM_BUTTON_DEBOUNCE_MS)
        {
            return;  // Ignorar clicks muy rápidos
        }
        lastDiagramButtonTime = currentTime;
        
        // Alternar diagrama basado en visibilidad actual
        bool diagramWillBeActivated = (diagramOverlay == nullptr || !diagramOverlay->isVisible());
        
        // Comportamiento exclusivo: solo desactivar otros cuando DIAGRAM se va a activar
        if (diagramWillBeActivated) {
            // DIAGRAM solo desactiva BYPASS y SOLO SC - DELTA ahora es independiente
            parameterButtons.bypassButton.setToggleState(false, juce::sendNotification);
            sidechainControls.soloScButton.setToggleState(false, juce::sendNotification);
        }
        
        // Ejecutar el alternado
        if (diagramOverlay != nullptr && diagramOverlay->isVisible())
        {
            hideDiagram();
        }
        else
        {
            showDiagram();
        }
    }
    // Manejador del botón CODE removido - funcionalidad manejada por botón DIAGRAM
    else if (button == &parameterButtons.deltaButton)
    {
        // DELTA funciona como botón normal - igual que FILTERS
        updateButtonStates();
    }
    else if (button == &rightTopControls.dmodeButton)
    {
        // Custom handling para DMODE: mapear toggle state (OFF/ON) a parámetro (0/2)
        if (auto* dmodeParam = processor.apvts.getParameter("h_DELTAMODE")) {
            bool isToggled = rightTopControls.dmodeButton.getToggleState();
            float newValue = isToggled ? 2.0f : 0.0f;  // 0=TRANS, 2=SUST
            dmodeParam->setValueNotifyingHost(dmodeParam->convertTo0to1(newValue));
            updateDmodeButtonText();
            handleParameterChange();
        }
    }
    else if (button == &leftTopKnobs.flipButton)
    {
        // FLIP: Intercambiar valores de TRANS y SUST
        auto* attackParam = processor.apvts.getParameter("b_ATTACK_GAIN");
        auto* sustainParam = processor.apvts.getParameter("c_SUSTAIN_GAIN");
        
        if (attackParam && sustainParam) {
            // Obtener valores actuales
            float currentAttack = attackParam->getValue();
            float currentSustain = sustainParam->getValue();
            
            // Intercambiar valores con gesture para que host decida si acepta los cambios
            attackParam->beginChangeGesture();
            sustainParam->beginChangeGesture();
            attackParam->setValue(currentSustain);  // Sin notificar host - gesture maneja automatización
            sustainParam->setValue(currentAttack);
            attackParam->endChangeGesture();
            sustainParam->endChangeGesture();
            
            // Registrar acción undo compound para ambos parámetros
            undoManager.beginNewTransaction("Flip TRANS/SUST");
            UndoableParameterHelper::registerChange(&undoManager, *attackParam, currentAttack, currentSustain, 0.0001f);
            UndoableParameterHelper::registerChange(&undoManager, *sustainParam, currentSustain, currentAttack, 0.0001f);
            
            // Feedback visual: destello negro temporal
            leftTopKnobs.flipButton.setColour(juce::TextButton::buttonColourId, juce::Colours::black.withAlpha(0.8f));
            leftTopKnobs.flipButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            
            // Usar SafePointer para prevenir crashes si el editor se destruye antes del timer
            juce::Component::SafePointer<JCBTransientAudioProcessorEditor> safeThis(this);
            juce::Timer::callAfterDelay(200, [safeThis]() {
                if (safeThis) {
                    safeThis->leftTopKnobs.flipButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
                    safeThis->leftTopKnobs.flipButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary);
                }
            });
            
            handleParameterChange();
        }
    }
}

void JCBTransientAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    // Manejar cambios en h_DELTAMODE para sincronizar botón
    if (parameterID == "h_DELTAMODE") {
        if (auto* dmodeParam = processor.apvts.getParameter("h_DELTAMODE")) {
            float denormalizedValue = dmodeParam->convertFrom0to1(newValue);
            bool shouldBeToggled = denormalizedValue >= 1.5f;  // 2.0 = SUST (ON), 0.0 = TRANS (OFF)
            
            // Actualizar botón sin enviar notificación (evitar loop)
            rightTopControls.dmodeButton.setToggleState(shouldBeToggled, juce::dontSendNotification);
            updateDmodeButtonText();
        }
    }
}

//==============================================================================
// MANEJO DE PARÁMETROS Y EVENTOS
//==============================================================================
//==============================================================================
// GESTIÓN DE ESTADO DE PARÁMETROS
//==============================================================================
void JCBTransientAudioProcessorEditor::handleParameterChange()
{
    // No hacer nada si estamos cargando un preset
    if (isLoadingPreset) {
        return;
    }
    
    if (presetArea.presetMenu.getSelectedId() > 0) {
        // Un preset está seleccionado - deseleccionar y mostrar con asterisco
        juce::String currentPresetName = presetArea.presetMenu.getText();
        
        presetArea.presetMenu.setSelectedId(0);
        processor.setLastPreset(0);
        
        // Mostrar el nombre del preset con asterisco en cursiva
        auto modifiedText = currentPresetName + "*";
        presetArea.presetMenu.setTextWhenNothingSelected(modifiedText);
        presetArea.presetMenu.setTextItalic(true);
        
        // Guardar el estado visual en el processor
        processor.setPresetDisplayText(modifiedText);
        processor.setPresetTextItalic(true);
        
    } else {
        // No hay preset seleccionado - verificar si es DEFAULT
        juce::String currentText = presetArea.presetMenu.getTextWhenNothingSelected();
        if (currentText == "DEFAULT" || currentText.isEmpty()) {
            // DEFAULT nunca debe mostrarse como modificado
            // En su lugar, no mostrar nada
            presetArea.presetMenu.setTextWhenNothingSelected("");
            presetArea.presetMenu.setTextItalic(false);
            
            // Guardar el estado visual en el processor
            processor.setPresetDisplayText("");
            processor.setPresetTextItalic(false);
        }
        // Si ya tiene un asterisco, no hacer nada
    }
    
    // Actualizar texto del botón EXT KEY dinámicamente basándose en el estado del parámetro "r_KEY"
    if (auto* keyParam = processor.apvts.getRawParameterValue("r_KEY")) {
        bool isExtKeyActive = keyParam->load() > 0.5f;
        if (isExtKeyActive) {
            sidechainControls.keyButton.setButtonText("SC EXT");
        } else {
            sidechainControls.keyButton.setButtonText("SC INT");
        }
        // Nota: El tooltip se actualiza automáticamente via getTooltipText("extkey") en updateAllTooltips()
    }
}



//==============================================================================
// MÉTODOS DE SETUP Y CONFIGURACIÓN
//==============================================================================
void JCBTransientAudioProcessorEditor::setupKnobs()
{
    // Establecer undo manager y editor para todos los sliders primero
    
    // === LEFT TOP KNOBS ===
    // TRAN (Attack Gain)
    leftTopKnobs.attackSlider.setName("trans");  // Para mostrar TRANS dentro del knob
    leftTopKnobs.attackSlider.setComponentID("tran");
    leftTopKnobs.attackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    leftTopKnobs.attackSlider.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 70, 16);
    leftTopKnobs.attackSlider.setLookAndFeel(&sliderLAFBig);
    leftTopKnobs.attackSlider.setTextBoxIsEditable(true);
    leftTopKnobs.attackSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    // Configurar color naranja claro para TRANS (más visible que histograma Attack)
    leftTopKnobs.attackSlider.setColour(juce::Slider::rotarySliderOutlineColourId, DarkTheme::accentWarmLight);
    leftTopKnobs.attackSlider.setDoubleClickReturnValue(true, 0.0);
    leftTopKnobs.attackSlider.setPopupDisplayEnabled(false, false, this);
    leftTopKnobs.attackSlider.setNumDecimalPlacesToDisplay(1);
    leftTopKnobs.attackSlider.setTextValueSuffix(" dB");
    // Configurar rango para Attack Gain: -18 a +18 dB, centrado en 0
    leftTopKnobs.attackSlider.setRange(-18.0, 18.0, 0.1);
    leftTopKnobs.attackSlider.setSkewFactorFromMidPoint(0.0);  // 0dB en el centro
    addAndMakeVisible(leftTopKnobs.attackSlider);
    if (auto* param = processor.apvts.getParameter("b_ATTACK_GAIN"))
    {
        leftTopKnobs.attackAttachment = std::make_unique<CustomSliderAttachment>(
            *param, leftTopKnobs.attackSlider, &undoManager);
        leftTopKnobs.attackAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }
    leftTopKnobs.attackSlider.setTooltip(JUCE_UTF8("ATTACK GAIN: ganancia para transientes de ataque entre -18 y +18 dB.\nValores positivos realzan ataques, negativos los atenúan.\nValor por defecto: 0 dB"));
    
    // SUST (Sustain Gain)
    leftTopKnobs.sustainSlider.setName("sust");  // Para mostrar SUST dentro del knob
    leftTopKnobs.sustainSlider.setComponentID("sust");
    leftTopKnobs.sustainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    leftTopKnobs.sustainSlider.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 70, 16);
    leftTopKnobs.sustainSlider.setLookAndFeel(&sliderLAFBig);
    leftTopKnobs.sustainSlider.setTextBoxIsEditable(true);  // Ahora editable
    leftTopKnobs.sustainSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    // Configurar color azul para SUST (igual que histograma Sustain)
    leftTopKnobs.sustainSlider.setColour(juce::Slider::rotarySliderOutlineColourId, DarkTheme::accent);
    leftTopKnobs.sustainSlider.setDoubleClickReturnValue(true, 0.0);
    leftTopKnobs.sustainSlider.setPopupDisplayEnabled(false, false, this);
    leftTopKnobs.sustainSlider.setNumDecimalPlacesToDisplay(1);
    leftTopKnobs.sustainSlider.setTextValueSuffix(" dB");
    // Configurar rango para Sustain Gain: -18 a +18 dB, centrado en 0
    leftTopKnobs.sustainSlider.setRange(-18.0, 18.0, 0.1);
    leftTopKnobs.sustainSlider.setSkewFactorFromMidPoint(0.0);  // 0dB en el centro
    addAndMakeVisible(leftTopKnobs.sustainSlider);
    if (auto* param = processor.apvts.getParameter("c_SUSTAIN_GAIN"))
    {
        leftTopKnobs.sustainAttachment = std::make_unique<CustomSliderAttachment>(
            *param, leftTopKnobs.sustainSlider, &undoManager);
        leftTopKnobs.sustainAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }
    leftTopKnobs.sustainSlider.setTooltip(JUCE_UTF8("SUSTAIN GAIN: ganancia para transientes de sustain entre -18 y +18 dB.\nValores positivos realzan sustains, negativos los atenúan.\nValor por defecto: 0 dB"));

    // SENS (Sensitivity)
    leftTopKnobs.sensSlider.setName("sens");  // Para mostrar SENS dentro del knob
    leftTopKnobs.sensSlider.setComponentID("sens");
    leftTopKnobs.sensSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    leftTopKnobs.sensSlider.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 70, 16);
    leftTopKnobs.sensSlider.setLookAndFeel(&sliderLAFBig);
    leftTopKnobs.sensSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    // Configurar color amarillo para SENS (distintivo)
    leftTopKnobs.sensSlider.setColour(juce::Slider::rotarySliderOutlineColourId, DarkTheme::accentYellow);
    leftTopKnobs.sensSlider.setTextBoxIsEditable(true);
    leftTopKnobs.sensSlider.setDoubleClickReturnValue(true, 0.5);
    leftTopKnobs.sensSlider.setPopupDisplayEnabled(false, false, this);
    leftTopKnobs.sensSlider.setNumDecimalPlacesToDisplay(0);  // Sin decimales para mostrar porcentajes
    leftTopKnobs.sensSlider.textFromValueFunction = [](double value) { return juce::String(juce::roundToInt(value * 100)) + "%"; };
    // Configurar rango para Sensitivity: 0 a 1, centrado en 0.5
    leftTopKnobs.sensSlider.setRange(0.0, 1.0, 0.01);
    leftTopKnobs.sensSlider.setSkewFactorFromMidPoint(0.5);  // 0.5 en el centro
    addAndMakeVisible(leftTopKnobs.sensSlider);
    if (auto* param = processor.apvts.getParameter("q_SENSITIVITY"))
    {
        leftTopKnobs.sensAttachment = std::make_unique<CustomSliderAttachment>(
            *param, leftTopKnobs.sensSlider, &undoManager);
        leftTopKnobs.sensAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }
    leftTopKnobs.sensSlider.setTooltip(JUCE_UTF8("SENSITIVITY: sensibilidad de detección entre 0 y 1.\nControla la sensibilidad del algoritmo de detección de transientes.\nValor por defecto: 0.5"));

    // === PERILLAS INFERIORES IZQUIERDAS ===
    // D/W (Seco/Húmedo)
    leftBottomKnobs.drywetSlider.setComponentID("drywet");
    leftBottomKnobs.drywetSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    leftBottomKnobs.drywetSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
    leftBottomKnobs.drywetSlider.setLookAndFeel(&sliderLAFBig);
    leftBottomKnobs.drywetSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    // Configurar color blanco para D/W
    leftBottomKnobs.drywetSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white);
    leftBottomKnobs.drywetSlider.setDoubleClickReturnValue(true, 1.0);
    leftBottomKnobs.drywetSlider.setPopupDisplayEnabled(false, false, this);
    // Visualización personalizada de valor para porcentaje
    leftBottomKnobs.drywetSlider.textFromValueFunction = [](double value) {
        return juce::String(static_cast<int>(value * 101)) + " %";
    };
    addAndMakeVisible(leftBottomKnobs.drywetSlider);
    if (auto* param = processor.apvts.getParameter("o_DRYWET"))
    {
        leftBottomKnobs.drywetAttachment = std::make_unique<CustomSliderAttachment>(
            *param, leftBottomKnobs.drywetSlider, &undoManager);
        leftBottomKnobs.drywetAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }
    leftBottomKnobs.drywetSlider.setTooltip(JUCE_UTF8("D/W: mezcla entre señal seca y procesada.\n0% = solo señal original, 100% = solo señal comprimida.\nValor por defecto: 100% (wet)"));
    
    // LA (Anticipación)
    leftBottomKnobs.lookaheadSlider.setComponentID("lookahead");
    leftBottomKnobs.lookaheadSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    leftBottomKnobs.lookaheadSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
    leftBottomKnobs.lookaheadSlider.setLookAndFeel(&sliderLAFBig);
    leftBottomKnobs.lookaheadSlider.setTextBoxIsEditable(true);
    leftBottomKnobs.lookaheadSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    // Configurar color blanco para LA
    leftBottomKnobs.lookaheadSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white);
    leftBottomKnobs.lookaheadSlider.setTextValueSuffix(" ms");
    leftBottomKnobs.lookaheadSlider.setRange(0.0, 10.0, 0.1);
    leftBottomKnobs.lookaheadSlider.setValue(0.0);
    leftBottomKnobs.lookaheadSlider.setDoubleClickReturnValue(true, 0.0);
    leftBottomKnobs.lookaheadSlider.setPopupDisplayEnabled(false, false, this);
    leftBottomKnobs.lookaheadSlider.setNumDecimalPlacesToDisplay(1);
    addAndMakeVisible(leftBottomKnobs.lookaheadSlider);
    if (auto* param = processor.apvts.getParameter("n_LOOKAHEAD"))
    {
        leftBottomKnobs.lookaheadAttachment = std::make_unique<CustomSliderAttachment>(
            *param, leftBottomKnobs.lookaheadSlider, &undoManager);
        leftBottomKnobs.lookaheadAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }
    leftBottomKnobs.lookaheadSlider.setTooltip(JUCE_UTF8("LOOKAHEAD: anticipación entre 0 y 10 ms.\nEvita distorsión en transitorios rápidos.\nValor por defecto: 0 ms"));
    
    // CLIP (vinculado a u_SOFTCLIP)
    leftBottomKnobs.clipSlider.setComponentID("clipper");
    leftBottomKnobs.clipSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    leftBottomKnobs.clipSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
    leftBottomKnobs.clipSlider.setLookAndFeel(&sliderLAFBig);
    leftBottomKnobs.clipSlider.setTextBoxIsEditable(true);
    leftBottomKnobs.clipSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    // Configurar color blanco para CLIP
    leftBottomKnobs.clipSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white);

    leftBottomKnobs.clipSlider.setValue(0.0);
    leftBottomKnobs.clipSlider.setDoubleClickReturnValue(true, 0.0);
    leftBottomKnobs.clipSlider.setPopupDisplayEnabled(false, false, this);
    // Formato de texto personalizado para CLIP: OFF a 100%
    leftBottomKnobs.clipSlider.textFromValueFunction = [](double value) {
        if (value < 0.01)
            return juce::String("OFF");
        else if (value > 0.99)
            return juce::String("100%");
        else
            return juce::String(static_cast<int>(value * 100)) + "%";
    };
    addAndMakeVisible(leftBottomKnobs.clipSlider);
    if (auto* param = processor.apvts.getParameter("u_SOFTCLIP"))
    {
        leftBottomKnobs.clipAttachment = std::make_unique<CustomSliderAttachment>(
            *param, leftBottomKnobs.clipSlider, &undoManager);
        leftBottomKnobs.clipAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }
    leftBottomKnobs.clipSlider.setTooltip(JUCE_UTF8("CLIP: soft clipping para limitar picos.\n0 = sin clipping, 1 = máximo clipping.\nValor por defecto: 0 (off)"));
    
    
    // === PERILLAS INFERIORES DERECHAS ===
    // ATK - copiado exactamente de ExpansorGate
    rightBottomKnobs.atkSlider.setComponentID("attack");
    rightBottomKnobs.atkSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    rightBottomKnobs.atkSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
    rightBottomKnobs.atkSlider.setLookAndFeel(&sliderLAFBig);
    rightBottomKnobs.atkSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    rightBottomKnobs.atkSlider.setDoubleClickReturnValue(true, 1.0);  // Valor por defecto 1ms
    rightBottomKnobs.atkSlider.setPopupDisplayEnabled(false, false, this);
    rightBottomKnobs.atkSlider.setTextBoxIsEditable(true);
    // Custom text formatting con decimales progresivos
    rightBottomKnobs.atkSlider.textFromValueFunction = [](double value) {
        if (value < 1.0)
            return juce::String(value, 3) + " ms";  // 3 decimales: 0.001
        else if (value < 10.0)
            return juce::String(value, 2) + " ms";  // 2 decimales: 1.50
        else if (value < 100.0)
            return juce::String(value, 1) + " ms";  // 1 decimal: 10.5
        else
            return juce::String(value, 1) + " ms";  // 1 decimal: 100.5
    };
    // Configurar rango con compensación ligera para mejor balance visual
    rightBottomKnobs.atkSlider.setRange(0.1, 250.0, 0.01);
    rightBottomKnobs.atkSlider.setSkewFactorFromMidPoint(20.0);  // Compensación ligera para balance visual
    addAndMakeVisible(rightBottomKnobs.atkSlider);
    if (auto* param = processor.apvts.getParameter("d_ATK"))
    {
        rightBottomKnobs.atkAttachment = std::make_unique<CustomSliderAttachment>(
            *param, rightBottomKnobs.atkSlider, &undoManager);
        rightBottomKnobs.atkAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }
    rightBottomKnobs.atkSlider.setTooltip(JUCE_UTF8("ATK: tiempo de ataque entre 0.1 y 250 ms.\nControla la respuesta ante aumentos de nivel.\nValor por defecto: 5 ms"));
    
    // REL - modificado con rango mínimo de 1ms y formato de decimales progresivo
    rightBottomKnobs.relSlider.setComponentID("release");
    rightBottomKnobs.relSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    rightBottomKnobs.relSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
    rightBottomKnobs.relSlider.setLookAndFeel(&sliderLAFBig);
    rightBottomKnobs.relSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    rightBottomKnobs.relSlider.setRange(0.1, 1000.0, 0.01);  // Coherente con parámetro
    rightBottomKnobs.relSlider.setSkewFactorFromMidPoint(30.0);  // Centrar rango operativo 1-50ms
    rightBottomKnobs.relSlider.setDoubleClickReturnValue(true, 120.0);  // Valor por defecto 120ms
    rightBottomKnobs.relSlider.setPopupDisplayEnabled(false, false, this);
    rightBottomKnobs.relSlider.setTextBoxIsEditable(true);
    // Custom text formatting con decimales progresivos - coherente con parámetro
    rightBottomKnobs.relSlider.textFromValueFunction = [](double value) {
        if (value < 1.0)
            return juce::String(value, 2) + " ms";  // 2 decimales: 0.10
        else if (value < 10.0)
            return juce::String(value, 1) + " ms";  // 1 decimal: 1.5
        else if (value < 100.0)
            return juce::String(value, 1) + " ms";  // 1 decimal: 10.5
        else
            return juce::String(value, 0) + " ms";  // Sin decimales >= 100ms
    };
    addAndMakeVisible(rightBottomKnobs.relSlider);
    if (auto* param = processor.apvts.getParameter("e_REL"))
    {
        rightBottomKnobs.relAttachment = std::make_unique<CustomSliderAttachment>(
            *param, rightBottomKnobs.relSlider, &undoManager);
        rightBottomKnobs.relAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }

    // HOLD - nuevo knob (donde estaba AUTO REL)
    rightBottomKnobs.holdSlider.setName("hold");  // Para mostrar HOLD dentro del knob
    rightBottomKnobs.holdSlider.setComponentID("hold");
    rightBottomKnobs.holdSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    rightBottomKnobs.holdSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
    rightBottomKnobs.holdSlider.setLookAndFeel(&sliderLAFBig);
    rightBottomKnobs.holdSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    rightBottomKnobs.holdSlider.setRange(0.0, 500.0, 0.1);
    rightBottomKnobs.holdSlider.setDoubleClickReturnValue(true, 0.0);
    rightBottomKnobs.holdSlider.setPopupDisplayEnabled(false, false, this);
    rightBottomKnobs.holdSlider.setTextBoxIsEditable(true);
    rightBottomKnobs.holdSlider.setNumDecimalPlacesToDisplay(1);
    // Formato de texto para HOLD en ms
    rightBottomKnobs.holdSlider.textFromValueFunction = [](double value) {
        if (value == 0.0)
            return juce::String("0 ms");
        else if (value < 10.0)
            return juce::String(value, 2) + " ms";  // 1 decimal para valores < 10
        else
            return juce::String(value, 1) + " ms";  // Sin decimales >= 10ms
    };
    addAndMakeVisible(rightBottomKnobs.holdSlider);
    if (auto* param = processor.apvts.getParameter("f_HOLD"))
    {
        rightBottomKnobs.holdAttachment = std::make_unique<CustomSliderAttachment>(
            *param, rightBottomKnobs.holdSlider, &undoManager);
        rightBottomKnobs.holdAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }

    // AR - botón con texto dinámico AR OFF/AR ON (removido - ya no existe)
    
    // SOLO DELTA - botón con texto "SOLO DELTA" (estilo SC)
    parameterButtons.deltaButton.setButtonText("SOLO DELTA");
    parameterButtons.deltaButton.setClickingTogglesState(true);
    parameterButtons.deltaButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    parameterButtons.deltaButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green.withAlpha(0.3f)); // Color verde cuando ON
    parameterButtons.deltaButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary.withAlpha(0.7f));
    parameterButtons.deltaButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    parameterButtons.deltaButton.addListener(this);
    addAndMakeVisible(parameterButtons.deltaButton);
    // deltaAttachment movido a parameterButtons - manejado en setupParameterButtons()
    
    // Callback de DELTA movido al método buttonClicked()
    
    
    // DMODE - botón de dos posiciones TRANS/SUST (reemplaza RANGE slider)
    rightTopControls.dmodeButton.setClickingTogglesState(true);
    rightTopControls.dmodeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    rightTopControls.dmodeButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accentWarm.withAlpha(0.3f));  // Naranja coordinado con Attack
    rightTopControls.dmodeButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary.withAlpha(0.7f));
    rightTopControls.dmodeButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    rightTopControls.dmodeButton.setLookAndFeel(&smallButtonLAF);
    rightTopControls.dmodeButton.addListener(this);
    addAndMakeVisible(rightTopControls.dmodeButton);
    // Custom attachment para DMODE - mapea botón toggle (OFF/ON) a parámetro (0/2)
    if (auto* dmodeParam = processor.apvts.getParameter("h_DELTAMODE")) {
        // Configurar estado inicial del botón basado en el parámetro
        float currentValue = dmodeParam->convertFrom0to1(dmodeParam->getValue());
        rightTopControls.dmodeButton.setToggleState(currentValue >= 1.5f, juce::dontSendNotification);
        
        // Listener del parámetro -> botón
        processor.apvts.addParameterListener("h_DELTAMODE", this);
        
        // Listener del botón -> parámetro (implementado en buttonClicked)
    }
    
    // SMOOTH - knob con mismo estilo que los demás
    rightTopControls.smoothSlider.setName("smo");  // Para mostrar SMO dentro del knob
    rightTopControls.smoothSlider.setComponentID("smo");
    rightTopControls.smoothSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    rightTopControls.smoothSlider.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 70, 16);
    rightTopControls.smoothSlider.setLookAndFeel(&sliderLAFBig);  // Usar LAF grande como los demás
    rightTopControls.smoothSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    rightTopControls.smoothSlider.setRange(0.0, 1.0, 0.01);
    // No llamar setValue() - el SliderAttachment manejará el valor inicial
    rightTopControls.smoothSlider.setDoubleClickReturnValue(true, 0.0);
    rightTopControls.smoothSlider.setPopupDisplayEnabled(false, false, this);
    rightTopControls.smoothSlider.setTextBoxIsEditable(true);
    addAndMakeVisible(rightTopControls.smoothSlider);
    if (auto* param = processor.apvts.getParameter("z_SMOOTH"))
    {
        rightTopControls.smoothAttachment = std::make_unique<CustomSliderAttachment>(
            *param, rightTopControls.smoothSlider, &undoManager);
        rightTopControls.smoothAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }
    // Visualización de texto para SMOOTH - RAW en mínimo, SMOOTH en máximo
    // IMPORTANTE: Asignar textFromValueFunction DESPUÉS de crear el SliderAttachment
    rightTopControls.smoothSlider.textFromValueFunction = [](double value) {
        if (value < 0.01)
            return juce::String("RAW");
        else if (value > 0.99)
            return juce::String("SMOOTH");
        else
            return juce::String(value, 2);
    };
    // El tooltip se establecerá en updateAllTooltips()
    
    // === CONTROLES SUPERIORES DERECHOS (SLIDERS) ===
    // REACT ELIMINADO - parámetro ya no existe en Gen~
    
    // Conectar perillas al transfer display
    leftTopKnobs.attackSlider.onValueChange = [this]() {
        // THD ahora está en rango de dB (-60 a 0)
        float thresholdDB = static_cast<float>(leftTopKnobs.attackSlider.getValue());
        transferDisplay.setThreshold(thresholdDB);
        updateTransferDisplay();
    };
    leftTopKnobs.sustainSlider.onValueChange = [this]() {
        transferDisplay.setRatio(static_cast<float>(leftTopKnobs.sustainSlider.getValue()));
        updateTransferDisplay();
    };
    leftTopKnobs.sensSlider.onValueChange = [this]() {
        transferDisplay.setKnee(static_cast<float>(leftTopKnobs.sensSlider.getValue()));
        updateTransferDisplay();
    };
    // ELIMINADO: rangeSlider callback - dmodeButton no necesita transferDisplay
    
    
}

void JCBTransientAudioProcessorEditor::setupButtons()
{
    // === BOTÓN FLIP ===
    // Configurar botón FLIP para intercambio de valores TRANS/SUST
    leftTopKnobs.flipButton.setComponentID("flip");
    leftTopKnobs.flipButton.setClickingTogglesState(false);  // Botón de acción, no toggle
    leftTopKnobs.flipButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    leftTopKnobs.flipButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFFFFB366).withAlpha(0.3f)); // Naranja
    leftTopKnobs.flipButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary);
    leftTopKnobs.flipButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFFFB366)); // Naranja
    leftTopKnobs.flipButton.setLookAndFeel(&smallButtonLAF);
    leftTopKnobs.flipButton.addListener(this);  // CRÍTICO: Conectar eventos a buttonClicked()
    addAndMakeVisible(leftTopKnobs.flipButton);
    leftTopKnobs.flipButton.setTooltip(getTooltipText("flip"));
    
}

void JCBTransientAudioProcessorEditor::setupMeters()
{
    // Medidores de entrada
    addAndMakeVisible(inputMeterL);
    addAndMakeVisible(inputMeterR);
    // Medidor GR
    
    // Medidores de salida
    addAndMakeVisible(outputMeterL);
    addAndMakeVisible(outputMeterR);
    
    // Medidores de sidechain (siempre visibles)
    scMeterL.setVisible(true);
    scMeterR.setVisible(true);
    addAndMakeVisible(scMeterL);
    addAndMakeVisible(scMeterR);
    
    // Slider de trim
    trimSlider.setComponentID("trim");
    addAndMakeVisible(trimSlider);
    
    // Vincular slider de trim al parámetro - ahora usando attachment thread-safe
    if (auto* param = processor.apvts.getParameter("a_TRIM"))
    {
        trimAttachment = std::make_unique<CustomSliderAttachment>(
            *param, trimSlider, &undoManager);
        trimAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }
    
    
    // Slider de makeup
    makeupSlider.setComponentID("makeup");
    addAndMakeVisible(makeupSlider);
    
    // Vincular slider de makeup al parámetro - ahora usando attachment thread-safe
    if (auto* param = processor.apvts.getParameter("i_MAKEUP"))
    {
        makeupAttachment = std::make_unique<CustomSliderAttachment>(
            *param, makeupSlider, &undoManager);
        makeupAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }

    // Slider de trim de sidechain
    scTrimSlider.setComponentID("sctrim");
    addAndMakeVisible(scTrimSlider);
    
    // Establecer propiedades iniciales para el slider de trim de sidechain
    scTrimSlider.setTooltip(JUCE_UTF8("SC TRIM: ganancia de entrada del sidechain entre -12 y +12 dB.\nAjusta el nivel del sidechain externo.\nValor por defecto: 0 dB, se activa con EXT KEY"));
    
    // Vincular slider de trim de sidechain al parámetro y_SCTRIM - ahora usando attachment thread-safe
    if (auto* param = processor.apvts.getParameter("y_SCTRIM"))
    {
        scTrimAttachment = std::make_unique<CustomSliderAttachment>(
            *param, scTrimSlider, &undoManager);
        scTrimAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }
    
    // Inicialmente visible pero con alpha reducido cuando EXT KEY está apagado
    scTrimSlider.setVisible(true);
    scTrimSlider.setEnabled(false);
    scTrimSlider.setAlpha(0.25f);
    
    //==========================================================================
    // CONFIGURAR DISPLAYS INDEPENDIENTES
    //==========================================================================
    
}

void JCBTransientAudioProcessorEditor::setupSidechainControls()
{
    // Establecer undo manager y editor para sliders de sidechain
    
    // Slider HPF - copia EXACTA de ExpansorGate
    sidechainControls.hpfSlider.setName("hpf");
    sidechainControls.hpfSlider.setComponentID("hpf");
    sidechainControls.hpfSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sidechainControls.hpfSlider.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 60, 16);
    sidechainControls.hpfSlider.setLookAndFeel(&sliderLAFBig);  // Usar LAF grande como ExpansorGate
    sidechainControls.hpfSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white);  // Blanco fijo
    sidechainControls.hpfSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);  // Blanco fijo
    sidechainControls.hpfSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    sidechainControls.hpfSlider.setTextBoxIsEditable(false);
    sidechainControls.hpfSlider.setEnabled(false);  // Inicialmente deshabilitado
    sidechainControls.hpfSlider.setAlpha(0.0f);  // Inicialmente invisible
    sidechainControls.hpfSlider.setDoubleClickReturnValue(true, 20.0f);
    sidechainControls.hpfSlider.setPopupDisplayEnabled(false, false, this);
    sidechainControls.hpfSlider.setNumDecimalPlacesToDisplay(0);
    // Formato de texto personalizado para frecuencia
    sidechainControls.hpfSlider.textFromValueFunction = [](double value) {
        if (value < 1000.0)
            return juce::String(static_cast<int>(value));
        else
            return juce::String(value / 1000.0, 1) + "k";
    };
    sidechainControls.hpfSlider.setTextValueSuffix(" Hz");
    // Configurar rango y sesgo para poner 1kHz en el centro
    sidechainControls.hpfSlider.setRange(20.0, 20000.0, 1.0);
    sidechainControls.hpfSlider.setSkewFactorFromMidPoint(1000.0);  // 1kHz en el centro
    addAndMakeVisible(sidechainControls.hpfSlider);
    if (auto* param = processor.apvts.getParameter("j_HPF"))
    {
        sidechainControls.hpfAttachment = std::make_unique<CustomSliderAttachment>(
            *param, sidechainControls.hpfSlider, &undoManager);
        sidechainControls.hpfAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }
    
    // Slider LPF - copia EXACTA de ExpansorGate
    sidechainControls.lpfSlider.setName("lpf");
    sidechainControls.lpfSlider.setComponentID("lpf");
    sidechainControls.lpfSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sidechainControls.lpfSlider.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 60, 16);
    sidechainControls.lpfSlider.setLookAndFeel(&sliderLAFBig);  // Usar LAF grande como ExpansorGate
    sidechainControls.lpfSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white);  // Blanco fijo
    sidechainControls.lpfSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);  // Blanco fijo
    sidechainControls.lpfSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    sidechainControls.lpfSlider.setTextBoxIsEditable(false);
    sidechainControls.lpfSlider.setEnabled(false);  // Inicialmente deshabilitado
    sidechainControls.lpfSlider.setAlpha(0.0f);  // Inicialmente invisible
    sidechainControls.lpfSlider.setDoubleClickReturnValue(true, 20000.0f);
    sidechainControls.lpfSlider.setPopupDisplayEnabled(false, false, this);
    sidechainControls.lpfSlider.setNumDecimalPlacesToDisplay(0);
    // Formato de texto personalizado para frecuencia
    sidechainControls.lpfSlider.textFromValueFunction = [](double value) {
        if (value < 1000.0)
            return juce::String(static_cast<int>(value));
        else
            return juce::String(value / 1000.0, 1) + "k";
    };
    sidechainControls.lpfSlider.setTextValueSuffix(" Hz");
    // Configurar rango y sesgo para poner 1kHz en el centro
    sidechainControls.lpfSlider.setRange(20.0, 20000.0, 1.0);
    sidechainControls.lpfSlider.setSkewFactorFromMidPoint(1000.0);  // 1kHz en el centro
    addAndMakeVisible(sidechainControls.lpfSlider);
    if (auto* param = processor.apvts.getParameter("k_LPF"))
    {
        sidechainControls.lpfAttachment = std::make_unique<CustomSliderAttachment>(
            *param, sidechainControls.lpfSlider, &undoManager);
        sidechainControls.lpfAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }

    // Botón FILTERS (antes SC) - VISIBLE para activar/desactivar filtros
    sidechainControls.scButton.setClickingTogglesState(true);
    sidechainControls.scButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    sidechainControls.scButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkviolet.withAlpha(0.3f));
    sidechainControls.scButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary.withAlpha(0.7f));
    sidechainControls.scButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    sidechainControls.scButton.addListener(this);
    addAndMakeVisible(sidechainControls.scButton);
    sidechainControls.scAttachment = std::make_unique<UndoableButtonAttachment>(
        *processor.apvts.getParameter("l_SC"), sidechainControls.scButton, &undoManager);
    sidechainControls.scAttachment->onParameterChange = [this]() { handleParameterChange(); };
    sidechainControls.scButton.setTooltip(JUCE_UTF8("FILTERS: activa/desactiva los filtros del sidechain.\nPermite filtrar la señal que controla el compresor.\nValor por defecto: OFF"));
    
    // Botón EXT KEY - estilizado como ExpansorGate, SIEMPRE HABILITADO
    sidechainControls.keyButton.setClickingTogglesState(true);
    sidechainControls.keyButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    sidechainControls.keyButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF0097A7).withAlpha(0.3f));  // Teal oscuro distintivo
    sidechainControls.keyButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary.withAlpha(0.7f));
    sidechainControls.keyButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    sidechainControls.keyButton.setEnabled(true);  // SIEMPRE HABILITADO
    sidechainControls.keyButton.addListener(this);
    addAndMakeVisible(sidechainControls.keyButton);
    sidechainControls.keyAttachment = std::make_unique<UndoableButtonAttachment>(
        *processor.apvts.getParameter("r_KEY"), sidechainControls.keyButton, &undoManager);
    sidechainControls.keyAttachment->onParameterChange = [this]() { handleParameterChange(); };
    // Tooltip se establece via updateAllTooltips() usando getTooltipText("extkey")
    sidechainControls.keyButton.setTooltip("");
    
    // Botón Solo SC - estilizado como ExpansorGate con acento cálido, SIEMPRE HABILITADO
    sidechainControls.soloScButton.setClickingTogglesState(true);
    sidechainControls.soloScButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    sidechainControls.soloScButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accentWarm.withAlpha(0.3f));
    sidechainControls.soloScButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary.withAlpha(0.7f));
    sidechainControls.soloScButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    sidechainControls.soloScButton.setEnabled(true); // SIEMPRE HABILITADO
    sidechainControls.soloScButton.addListener(this);
    sidechainControls.soloScButton.onStateChange = [this]() {
        updateButtonStates();
        // Actualizar estado SOLO SIDECHAIN del transfer display
        transferDisplay.setSoloSidechain(sidechainControls.soloScButton.getToggleState());
    };
    addAndMakeVisible(sidechainControls.soloScButton);
    // Attachment de SOLO SC - excluido del undo por consistencia con DELTA y BYPASS (sin UndoManager)
    sidechainControls.soloScAttachment = std::make_unique<UndoableButtonAttachment>(
        *processor.apvts.getParameter("m_SOLOSC"), sidechainControls.soloScButton, nullptr);
    sidechainControls.soloScButton.setTooltip(JUCE_UTF8("SOLO SC: escucha solo la señal del sidechain.\nPermite monitorear qué está controlando el compresor.\nSe procesa en dual mono"));
    
    // Botón de orden HPF - control de orden del filtro
    sidechainControls.hpfOrderButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    sidechainControls.hpfOrderButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    sidechainControls.hpfOrderButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    sidechainControls.hpfOrderButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    sidechainControls.hpfOrderButton.setClickingTogglesState(true);
    sidechainControls.hpfOrderButton.setEnabled(true);   // Enable BEFORE creating attachment
    sidechainControls.hpfOrderButton.setAlpha(1.0f);     // Make visible BEFORE creating attachment
    addAndMakeVisible(sidechainControls.hpfOrderButton);
    // Crear UndoableButtonAttachment para HPF order
    if (auto* param = processor.apvts.getParameter("j_HPFORDER"))
    {
        sidechainControls.hpfOrderAttachment = std::make_unique<UndoableButtonAttachment>(
            *param, sidechainControls.hpfOrderButton, &undoManager);
        sidechainControls.hpfOrderAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }
    sidechainControls.hpfOrderButton.setTooltip(JUCE_UTF8("HPF ORDER: orden del filtro paso alto.\n12 dB/oct, 24 dB/oct o 48 dB/oct.\nClick para ciclar entre opciones.\nValor por defecto: 12 dB/oct"));
    
    // Botón de orden LPF - control de orden del filtro
    sidechainControls.lpfOrderButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    sidechainControls.lpfOrderButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    sidechainControls.lpfOrderButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    sidechainControls.lpfOrderButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    sidechainControls.lpfOrderButton.setClickingTogglesState(true);
    sidechainControls.lpfOrderButton.setEnabled(true);  // Enable BEFORE creating attachment
    sidechainControls.lpfOrderButton.setAlpha(1.0f);  // Make visible BEFORE creating attachment
    addAndMakeVisible(sidechainControls.lpfOrderButton);
    // Crear UndoableButtonAttachment para LPF order
    if (auto* param = processor.apvts.getParameter("k_LPFORDER"))
    {
        sidechainControls.lpfOrderAttachment = std::make_unique<UndoableButtonAttachment>(
            *param, sidechainControls.lpfOrderButton, &undoManager);
        sidechainControls.lpfOrderAttachment->onParameterChange = [this]() { handleParameterChange(); };
    }
    sidechainControls.lpfOrderButton.setTooltip(JUCE_UTF8("LPF ORDER: orden del filtro paso bajo.\n12 dB/oct, 24 dB/oct o 48 dB/oct.\nClick para ciclar entre opciones.\nValor por defecto: 12 dB/oct"));
    
    // Actualizar el texto inicial del botón EXT KEY basándose en el estado actual del parámetro
    if (auto* keyParam = processor.apvts.getRawParameterValue("r_KEY")) {
        bool isExtKeyActive = keyParam->load() > 0.5f;
        if (isExtKeyActive) {
            sidechainControls.keyButton.setButtonText("SC EXT");
        } else {
            sidechainControls.keyButton.setButtonText("SC INT");
        }
        // Nota: El tooltip se establece via updateAllTooltips() usando getTooltipText("extkey")
    }
}

void JCBTransientAudioProcessorEditor::setupPresetArea()
{
    // Botón Save - estilo transparente
    presetArea.saveButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    presetArea.saveButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    presetArea.saveButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
    presetArea.saveButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    presetArea.saveButton.addListener(this);
    addAndMakeVisible(presetArea.saveButton);
    presetArea.saveButton.setTooltip(JUCE_UTF8("SAVE: guarda el preset actual.\nSobrescribe el preset seleccionado con valores actuales.\nNo funciona con DEFAULT"));
    
    // Botón Save As - estilo transparente
    presetArea.saveAsButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    presetArea.saveAsButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    presetArea.saveAsButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
    presetArea.saveAsButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    presetArea.saveAsButton.addListener(this);
    addAndMakeVisible(presetArea.saveAsButton);
    presetArea.saveAsButton.setTooltip(JUCE_UTF8("SAVE AS: guarda como nuevo preset.\nCrea un nuevo archivo de preset con los valores actuales.\nPermite crear presets personalizados"));
    
    // Botón Delete - estilo transparente
    presetArea.deleteButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    presetArea.deleteButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    presetArea.deleteButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
    presetArea.deleteButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    presetArea.deleteButton.addListener(this);
    addAndMakeVisible(presetArea.deleteButton);
    presetArea.deleteButton.setTooltip(JUCE_UTF8("DELETE: elimina el preset seleccionado.\nBorra permanentemente el archivo de preset.\nRequiere confirmación"));
    
    // Botón Back - estilo transparente
    presetArea.backButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    presetArea.backButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    presetArea.backButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
    presetArea.backButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    presetArea.backButton.addListener(this);
    addAndMakeVisible(presetArea.backButton);
    presetArea.backButton.setTooltip(JUCE_UTF8("BACK: selecciona el preset anterior.\nNavega al preset previo en la lista.\nCicla al final si está al principio"));
    
    // Botón Next - estilo transparente
    presetArea.nextButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    presetArea.nextButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    presetArea.nextButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
    presetArea.nextButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    presetArea.nextButton.addListener(this);
    addAndMakeVisible(presetArea.nextButton);
    presetArea.nextButton.setTooltip(JUCE_UTF8("NEXT: selecciona el siguiente preset.\nNavega al siguiente preset en la lista.\nCicla al principio si está al final"));
    
    // Menú de preset - usando CustomComboBox como en JCBExpansorGate
    presetArea.presetMenu.setJustificationType(juce::Justification::centred);
    presetArea.presetMenu.setTextWhenNothingSelected("");
    presetArea.presetMenu.setTextWhenNoChoicesAvailable("No presets");
    
    // Configurar onChange para cargar presets
    presetArea.presetMenu.onChange = [this]() {
        int selectedId = presetArea.presetMenu.getSelectedId();
        if (selectedId == 0) return;
        
        // CRÍTICO: Establecer flag de carga PRIMERO para prevenir transacciones de undo
        isLoadingPreset = true;
        
        // Guard de scope para asegurar que el flag siempre se resetea
        struct LoadingGuard {
            bool& flag;
            LoadingGuard(bool& f) : flag(f) { flag = true; }
            ~LoadingGuard() { flag = false; }
        } guard(isLoadingPreset);
        
        // NOTA: El historial de undo se borrará al final para evitar grabar cambios de parámetros
        
        juce::String presetName = presetArea.presetMenu.getItemText(selectedId - 1);
        
        // Ignorar separadores
        if (presetName.startsWith("---")) {
            presetArea.presetMenu.setSelectedId(0);
            return;
        }
        
        if (presetName == "DEFAULT") {
            // Preset DEFAULT: Resetear todos los parámetros a sus valores por defecto definidos
            // Usar acceso directo a parámetros para actualizaciones inmediatas y confiables
            
            // Deshabilitar undo durante la carga de preset
            isLoadingPreset = true;
            
            // Establecer todos los parámetros a sus valores por defecto usando gesture mechanism
            // Esto replica el mismo mecanismo usado por CustomSliderAttachment (doble-click)
            if (auto* param = processor.apvts.getParameter("a_TRIM")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                trimSlider.setValue(realValue, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("b_THD")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                leftTopKnobs.attackSlider.setValue(realValue, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("c_RATIO")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                leftTopKnobs.sustainSlider.setValue(realValue, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("d_ATK")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                rightBottomKnobs.atkSlider.setValue(realValue, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("e_REL")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                rightBottomKnobs.relSlider.setValue(realValue, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("f_HOLD")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                rightBottomKnobs.holdSlider.setValue(realValue, juce::sendNotificationSync);
            }
            // REACT ELIMINADO - parámetro ya no existe
            // ELIMINADO: h_RANGE reset - dmodeButton para h_DELTAMODE se resetea automáticamente
            if (auto* param = processor.apvts.getParameter("q_KNEE")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                leftTopKnobs.sensSlider.setValue(realValue, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("i_MAKEUP")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                makeupSlider.setValue(realValue, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("j_HPF")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                sidechainControls.hpfSlider.setValue(realValue, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("k_LPF")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                sidechainControls.lpfSlider.setValue(realValue, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("l_SC")) {
                float defaultValue = param->getDefaultValue();
                bool toggleState = defaultValue >= 0.5f;
                sidechainControls.scButton.setToggleState(toggleState, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("m_SOLOSC")) {
                float defaultValue = param->getDefaultValue();
                bool toggleState = defaultValue >= 0.5f;
                sidechainControls.soloScButton.setToggleState(toggleState, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("n_LOOKAHEAD")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                leftBottomKnobs.lookaheadSlider.setValue(realValue, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("o_DRYWET")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                leftBottomKnobs.drywetSlider.setValue(realValue, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("p_BYPASS")) {
                float defaultValue = param->getDefaultValue();
                bool toggleState = defaultValue >= 0.5f;
                parameterButtons.bypassButton.setToggleState(toggleState, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("r_KEY")) {
                float defaultValue = param->getDefaultValue();
                bool toggleState = defaultValue >= 0.5f;
                sidechainControls.keyButton.setToggleState(toggleState, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("u_SOFTCLIP")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                leftBottomKnobs.clipSlider.setValue(realValue, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("v_DELTA")) {
                float defaultValue = param->getDefaultValue();
                bool toggleState = defaultValue >= 0.5f;
                parameterButtons.deltaButton.setToggleState(toggleState, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("y_SCTRIM")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                scTrimSlider.setValue(realValue, juce::sendNotificationSync);
            }
            if (auto* param = processor.apvts.getParameter("z_SMOOTH")) {
                float defaultValue = param->getDefaultValue();
                float realValue = param->getNormalisableRange().convertFrom0to1(defaultValue);
                rightTopControls.smoothSlider.setValue(realValue, juce::sendNotificationSync);
            }
            // Los botones de orden de filtro se actualizan automáticamente a través de sus attachments
            
            // Reactivar undo después carga de preset
            isLoadingPreset = false;
            
            // IMPORTANTE: Forzar la sincronización directa de Gen~ para asegurar valores correctos de los parámetros
            // Esto replica la misma sincronización realizada durante la instanciación del plugin
            for (int i = 0; i < JCBTransient::num_params(); i++) {
                auto paramName = juce::String(JCBTransient::getparametername(processor.getPluginState(), i));
                if (auto* param = processor.apvts.getRawParameterValue(paramName)) {
                    float value = param->load();
                    
                    // Aplicar la misma validación que en parameterChanged()
                    if (paramName == "d_ATK" && value < 0.f) {
                        value = 0.f;
                    }
                    if (paramName == "e_REL" && value < 0.1f) {
                        value = 0.1f;
                    }
                    
                    JCBTransient::setparameter(processor.getPluginState(), i, value, nullptr);
                }
            }
        } 
        else if (presetName.startsWith("[F] ")) {
            // Es un factory preset - cargar desde BinaryData
            juce::String factoryPresetName = presetName.substring(4); // Quitar "[F] "
            
            // Convertir el nombre a formato de recurso BinaryData
            juce::String resourceName = factoryPresetName.replace(" ", "_") + "_preset";
            
            // Buscar el recurso en BinaryData
            for (int i = 0; i < BinaryData::namedResourceListSize; ++i) {
                if (resourceName == BinaryData::namedResourceList[i]) {
                    // Cargar el preset desde BinaryData
                    int dataSize = 0;
                    const char* data = BinaryData::getNamedResource(BinaryData::namedResourceList[i], dataSize);
                    
                    if (data != nullptr && dataSize > 0) {
                        // Parsear el XML desde memoria
                        juce::String xmlContent(data, dataSize);
                        juce::XmlDocument xmlDoc(xmlContent);
                        std::unique_ptr<juce::XmlElement> xmlState(xmlDoc.getDocumentElement());
                        
                        if (xmlState != nullptr && xmlState->hasTagName(processor.apvts.state.getType())) {
                            processor.apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
                            
                            // Queue las actualizaciones de parámetros para botones momentáneos - DELTA ahora se guarda
                            queueParameterUpdate("p_BYPASS", 0.0f);
                            queueParameterUpdate("m_SOLOSC", 0.0f);
                        }
                    }
                    break;
                }
            }
        }
        else {
            // Es un user preset - cargar desde archivo
            juce::File presetFile = getPresetsFolder().getChildFile(presetName + ".preset");
            
            if (presetFile.existsAsFile()) {
                juce::XmlDocument xmlDoc(presetFile);
                std::unique_ptr<juce::XmlElement> xmlState(xmlDoc.getDocumentElement());
                
                if (xmlState != nullptr && xmlState->hasTagName(processor.apvts.state.getType())) {
                    processor.apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
                    
                    // Queue actualizaciones de parámetros para botones momentáneos
                    queueParameterUpdate("p_BYPASS", 0.0f);
                    queueParameterUpdate("m_SOLOSC", 0.0f);
                }
            }
        }
        
        // Actualizar estado en processor
        processor.setLastPreset(selectedId);
        processor.setPresetDisplayText(presetName);
        processor.setPresetTextItalic(false);
        presetArea.presetMenu.setTextItalic(false);
        
        // Actualizar sliders desde APVTS
        updateSliderValues();
        
        // Actualizar la gráfica de transferencia con los valores actuales
        // Es necesario obtener los valores directamente de los parámetros
        // porque los sliders pueden no estar actualizados todavía
        if (auto* thdParam = processor.apvts.getRawParameterValue("b_THD")) {
            transferDisplay.setThreshold(thdParam->load());
        }
        if (auto* ratioParam = processor.apvts.getRawParameterValue("c_RATIO")) {
            transferDisplay.setRatio(ratioParam->load());
        }
        if (auto* kneeParam = processor.apvts.getRawParameterValue("q_KNEE")) {
            transferDisplay.setKnee(kneeParam->load());
        }
        updateTransferDisplay();
        
        // Borrar el historial de undos DESPUÉS de haber establecido todos los valores.
        // Esto previene que los cambios de parámetros se registren en el historial de undo
        undoManager.clearUndoHistory();
        
        // Asegurar que el transfer display se actualice después de limpiar el historial de undo
        updateTransferDisplay();
        
        
        // Nota: el flag isLoadingPreset se resetea automáticamente por el destructor LoadingGuard
    };
    
    addAndMakeVisible(presetArea.presetMenu);
    
    // Inicializar menú de presets
    refreshPresetMenu();
    
    // Configurar el texto inicial del menú según el estado guardado
    auto savedText = processor.getPresetDisplayText();
    auto isItalic = processor.getPresetTextItalic();
    if (!savedText.isEmpty()) {
        presetArea.presetMenu.setTextWhenNothingSelected(savedText);
        presetArea.presetMenu.setTextItalic(isItalic);
    }
}

void JCBTransientAudioProcessorEditor::setupUtilityButtons()
{
    // Undo - estilo transparente
    utilityButtons.undoButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    utilityButtons.undoButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    utilityButtons.undoButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
    utilityButtons.undoButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    utilityButtons.undoButton.addListener(this);
    addAndMakeVisible(utilityButtons.undoButton);
    
    // Redo - estilo transparente
    utilityButtons.redoButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    utilityButtons.redoButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    utilityButtons.redoButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
    utilityButtons.redoButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    utilityButtons.redoButton.addListener(this);
    addAndMakeVisible(utilityButtons.redoButton);
    
    // Establecer el estado inicial de los botones de undo/redo (normalmente desactivados al inicio)
    utilityButtons.undoButton.setAlpha(0.3f);
    utilityButtons.undoButton.setEnabled(false);
    utilityButtons.redoButton.setAlpha(0.3f);
    utilityButtons.redoButton.setEnabled(false);
    
    // Reset GUI - estilo transparente
    utilityButtons.resetGuiButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    utilityButtons.resetGuiButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    utilityButtons.resetGuiButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
    utilityButtons.resetGuiButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    utilityButtons.resetGuiButton.addListener(this);
    addAndMakeVisible(utilityButtons.resetGuiButton);
    utilityButtons.resetGuiButton.setTooltip(JUCE_UTF8("SIZE: cicla entre diferentes tamaños de ventana.\nClick cambia entre: Actual → Máximo → Mínimo → Actual.\nÚtil para ajustar rápidamente el tamaño del plugin"));
    
    // El botón de bypass se ha movido a parameterButtons

    // Ejecutar gráficos - caso especial: invertido (OFF con fondo, ON transparente)
    utilityButtons.runGraphicsButton.setClickingTogglesState(true);
    utilityButtons.runGraphicsButton.setColour(juce::TextButton::buttonColourId, DarkTheme::accent.withAlpha(0.3f));  // OFF: con fondo
    utilityButtons.runGraphicsButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);  // ON: transparente
    utilityButtons.runGraphicsButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
    utilityButtons.runGraphicsButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    utilityButtons.runGraphicsButton.addListener(this);
    addAndMakeVisible(utilityButtons.runGraphicsButton);
    utilityButtons.runGraphicsButton.setTooltip(JUCE_UTF8("GRAPHICS: muestra envolventes en tiempo real.\nVisualiza las formas de onda de entrada y salida.\nDesactivar mejora el rendimiento en CPUs lentas"));
    
    // Botón toggle de tooltip - estilo transparente
    utilityButtons.tooltipToggleButton.setClickingTogglesState(true);
    utilityButtons.tooltipToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    utilityButtons.tooltipToggleButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    utilityButtons.tooltipToggleButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
    utilityButtons.tooltipToggleButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    utilityButtons.tooltipToggleButton.setToggleState(true, juce::dontSendNotification); // ON por defecto
    utilityButtons.tooltipToggleButton.addListener(this);
    addAndMakeVisible(utilityButtons.tooltipToggleButton);
    utilityButtons.tooltipToggleButton.setTooltip(JUCE_UTF8("TOOLTIP: muestra/oculta los tooltips de ayuda.\nActiva o desactiva las ventanas de ayuda emergentes."));
    
    // Botón de idioma - estilo transparente
    utilityButtons.tooltipLangButton.setClickingTogglesState(false);  // No toggle, solo botón normal
    utilityButtons.tooltipLangButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    utilityButtons.tooltipLangButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    utilityButtons.tooltipLangButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
    utilityButtons.tooltipLangButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    utilityButtons.tooltipLangButton.setButtonText("esp");  // Texto inicial para español
    utilityButtons.tooltipLangButton.addListener(this);
    addAndMakeVisible(utilityButtons.tooltipLangButton);
    utilityButtons.tooltipLangButton.setTooltip(JUCE_UTF8("IDIOMA: cambia entre español e inglés.\nAlterna el idioma de los tooltips."));
    
    // Botón HQ - oversampling
    utilityButtons.hqButton.setClickingTogglesState(true);
    utilityButtons.hqButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    utilityButtons.hqButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    utilityButtons.hqButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary.withAlpha(0.7f));
    utilityButtons.hqButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    utilityButtons.hqButton.addListener(this);
    addAndMakeVisible(utilityButtons.hqButton);
    utilityButtons.hqButton.setEnabled(false);  // TODO: Implementar
    
    // Dual Mono button
    utilityButtons.dualMonoButton.setClickingTogglesState(true);
    utilityButtons.dualMonoButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    utilityButtons.dualMonoButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    utilityButtons.dualMonoButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary.withAlpha(0.7f));
    utilityButtons.dualMonoButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    utilityButtons.dualMonoButton.addListener(this);
    addAndMakeVisible(utilityButtons.dualMonoButton);
    utilityButtons.dualMonoButton.setEnabled(false);  // TODO: Implementar
    
    // Stereo Linked button (ALWAYS ON - plugin only works in stereo linked mode)
    utilityButtons.stereoLinkedButton.setClickingTogglesState(false);  // No toggle
    utilityButtons.stereoLinkedButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    utilityButtons.stereoLinkedButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    utilityButtons.stereoLinkedButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);  // Siempre texto blanco
    utilityButtons.stereoLinkedButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    utilityButtons.stereoLinkedButton.setToggleState(true, juce::dontSendNotification); // Siempre ON
    // No listener necesario ya que siempre está activo
    addAndMakeVisible(utilityButtons.stereoLinkedButton);
    utilityButtons.stereoLinkedButton.setTooltip(JUCE_UTF8("LINK: procesamiento stereo enlazado.\nSiempre activo - el plugin solo funciona en modo stereo linked."));
    utilityButtons.stereoLinkedButton.setEnabled(false);  // Disabled - can't be changed
    
    // Botón M/S
    utilityButtons.msButton.setClickingTogglesState(true);
    utilityButtons.msButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    utilityButtons.msButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    utilityButtons.msButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary.withAlpha(0.7f));
    utilityButtons.msButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    utilityButtons.msButton.addListener(this);
    addAndMakeVisible(utilityButtons.msButton);
    utilityButtons.msButton.setEnabled(false);  // TODO: Implementar
    
    // A/B State button
    topButtons.abStateButton.setClickingTogglesState(false);  // No es toggle, es un indicador
    topButtons.abStateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff9c27b0));  // Comenzar con púrpura para A
    topButtons.abStateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    topButtons.abStateButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    topButtons.abStateButton.addListener(this);
    topButtons.abStateButton.setButtonText("A");  // Comenzar con A
    addAndMakeVisible(topButtons.abStateButton);
    topButtons.abStateButton.setEnabled(true);  // Now implemented!
    
    // Botón copiar A/B
    topButtons.abCopyButton.setClickingTogglesState(false);
    topButtons.abCopyButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    topButtons.abCopyButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
    topButtons.abCopyButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    topButtons.abCopyButton.addListener(this);
    topButtons.abCopyButton.setButtonText("A-B");  // Start showing A to B
    addAndMakeVisible(topButtons.abCopyButton);
    topButtons.abCopyButton.setTooltip("Copy A to B");  // Tooltip inicial
    
    // MIDI Learn button
    utilityButtons.midiLearnButton.setClickingTogglesState(true);
    utilityButtons.midiLearnButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    utilityButtons.midiLearnButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::purple.withAlpha(0.3f));
    utilityButtons.midiLearnButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary.withAlpha(0.7f));
    utilityButtons.midiLearnButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    utilityButtons.midiLearnButton.addListener(this);
    addAndMakeVisible(utilityButtons.midiLearnButton);
    utilityButtons.midiLearnButton.setEnabled(false);  // TODO: Implementar
    
    // Botón Zoom
    utilityButtons.zoomButton.setClickingTogglesState(true);
    utilityButtons.zoomButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    utilityButtons.zoomButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    utilityButtons.zoomButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);  // Cambiado para igualar otros botones
    utilityButtons.zoomButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    utilityButtons.zoomButton.addListener(this);
    addAndMakeVisible(utilityButtons.zoomButton);
    utilityButtons.zoomButton.setButtonText("zoom"); // Estado inicial: Normal
    utilityButtons.zoomButton.setToggleState(false, juce::dontSendNotification); // Toggle OFF para normal
    utilityButtons.zoomButton.setTooltip(JUCE_UTF8("ZOOM: cambia entre vista normal y ampliada.\nNormal: -72dB a 0dB\nZoom x2: -48dB a 0dB"));
    
    // Botón Diagram
    centerButtons.diagramButton.setClickingTogglesState(true);
    centerButtons.diagramButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    centerButtons.diagramButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    centerButtons.diagramButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary.withAlpha(0.7f));
    centerButtons.diagramButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    centerButtons.diagramButton.addListener(this);
    addAndMakeVisible(centerButtons.diagramButton);
    centerButtons.diagramButton.setTooltip(JUCE_UTF8("DIAGRAM: muestra el diagrama de bloques del procesador.\nMuestra un overlay con el esquema de procesamiento de audio."));

}

void JCBTransientAudioProcessorEditor::setupParameterButtons()
{
    // Botón DELTA - movido desde rightBottomKnobs
    parameterButtons.deltaButton.setClickingTogglesState(true);
    parameterButtons.deltaButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    parameterButtons.deltaButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green.withAlpha(0.3f)); // Color verde cuando ON
    parameterButtons.deltaButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary.withAlpha(0.7f));
    parameterButtons.deltaButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    parameterButtons.deltaButton.setLookAndFeel(&smallButtonLAF);
    parameterButtons.deltaButton.addListener(this);
    addAndMakeVisible(parameterButtons.deltaButton);
    // Attachment de DELTA - ahora automatizable y con undo/redo habilitado
    parameterButtons.deltaAttachment = std::make_unique<UndoableButtonAttachment>(
        *processor.apvts.getParameter("v_DELTA"), parameterButtons.deltaButton, &undoManager);
    parameterButtons.deltaAttachment->onParameterChange = [this]() { handleParameterChange(); };
    parameterButtons.deltaButton.setTooltip(JUCE_UTF8("SOLO DELTA: escucha solo la diferencia (AUTOMATIZABLE).\nReproducir la señal procesada menos la original.\nSe guarda en presets y es controlable por DAW."));
    
    // Botón BYPASS - movido desde utilityButtons
    parameterButtons.bypassButton.setClickingTogglesState(true);
    parameterButtons.bypassButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    parameterButtons.bypassButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
    parameterButtons.bypassButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textSecondary.withAlpha(0.7f));
    parameterButtons.bypassButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
    parameterButtons.bypassButton.addListener(this);
    addAndMakeVisible(parameterButtons.bypassButton);
    // Attachment de BYPASS - excluido del undo (sin UndoManager) y no automatizable
    parameterButtons.bypassAttachment = std::make_unique<UndoableButtonAttachment>(
        *processor.apvts.getParameter("p_BYPASS"), parameterButtons.bypassButton, nullptr);
    parameterButtons.bypassButton.setTooltip(JUCE_UTF8("BYPASS: desactiva el procesamiento del plugin.\nEs independiente del bypass del DAW y está suavizado.\nValor por defecto: OFF"));
}

void JCBTransientAudioProcessorEditor::setupBackground()
{
    // Cargar imágenes de fondo
    normalBackground = juce::ImageCache::getFromMemory(BinaryData::fondo_png, BinaryData::fondo_pngSize);
    bypassBackground = juce::ImageCache::getFromMemory(BinaryData::bypass_png, BinaryData::bypass_pngSize);
    // ELIMINADO: deltaBackground - ya no se utiliza
    diagramBackground = juce::ImageCache::getFromMemory(BinaryData::diagramaFondo_png, BinaryData::diagramaFondo_pngSize);
    
    // Cargar iconos de filtro (solo usando imágenes de estado OFF)
    hpfOffImage = juce::ImageCache::getFromMemory(BinaryData::hpfOff_png, BinaryData::hpfOff_pngSize);
    lpfOffImage = juce::ImageCache::getFromMemory(BinaryData::lpfOff_png, BinaryData::lpfOff_pngSize);
    
    // Establecer background inicial
    if (normalBackground.isValid())
    {
        backgroundImage.setImage(normalBackground, juce::RectanglePlacement::stretchToFit);
        addAndMakeVisible(backgroundImage);
        backgroundImage.toBack();
    }
    
    // Configurar iconos de filtro - comenzar con estado OFF
    if (hpfOffImage.isValid())
    {
        hpfIcon.setImage(hpfOffImage, juce::RectanglePlacement::centred);
        addAndMakeVisible(hpfIcon);
    }
    
    if (lpfOffImage.isValid())
    {
        lpfIcon.setImage(lpfOffImage, juce::RectanglePlacement::centred);
        addAndMakeVisible(lpfIcon);
    }
}

//==============================================================================
// MÉTODOS DE UPDATE Y REFRESH
//==============================================================================

void JCBTransientAudioProcessorEditor::updateButtonStates()
{
    // Función maestra que actualiza todos los estados de UI
    updateBasicButtonStates();
    updateSidechainComponentStates();
    updateBackgroundState();
    updateMeterStates();
    updateDmodeButtonState();
}

void JCBTransientAudioProcessorEditor::updateBasicButtonStates()
{
    // Obtener estados actuales
    const bool bypassActive = parameterButtons.bypassButton.getToggleState();
    
    // Actualizar estados visuales básicos
    isBypassed = bypassActive;
    bypassTextVisible = bypassActive;  // Siempre mostrar texto cuando bypass está activo
}

void JCBTransientAudioProcessorEditor::updateSidechainComponentStates()
{
    // Los filtros HPF/LPF siempre visibles y activos
    sidechainControls.hpfSlider.setVisible(true);
    sidechainControls.lpfSlider.setVisible(true);
    sidechainControls.hpfSlider.setAlpha(1.0f);
    sidechainControls.lpfSlider.setAlpha(1.0f);
    sidechainControls.hpfSlider.setEnabled(true);
    sidechainControls.lpfSlider.setEnabled(true);
    
    // Actualizar visibilidad y estado de los botones de orden de filtro
    sidechainControls.hpfOrderButton.setVisible(true);
    sidechainControls.lpfOrderButton.setVisible(true);
    sidechainControls.hpfOrderButton.setAlpha(1.0f);
    sidechainControls.lpfOrderButton.setAlpha(1.0f);
    sidechainControls.hpfOrderButton.setEnabled(true);
    sidechainControls.lpfOrderButton.setEnabled(true);
    
    // Iconos siempre muestran imágenes blancas/OFF - no cambio dinámico basado en estado FILTERS
    if (hpfOffImage.isValid())
        hpfIcon.setImage(hpfOffImage, juce::RectanglePlacement::centred);
    if (lpfOffImage.isValid())
        lpfIcon.setImage(lpfOffImage, juce::RectanglePlacement::centred);
    
    // Forzar repaint para actualizar colores e iconos
    sidechainControls.hpfSlider.repaint();
    sidechainControls.lpfSlider.repaint();
    sidechainControls.hpfOrderButton.repaint();
    sidechainControls.lpfOrderButton.repaint();
    hpfIcon.repaint();
    lpfIcon.repaint();
    
    // Los sliders de sidechain trim según EXT KEY
    const bool extKeyActive = sidechainControls.keyButton.getToggleState();
    scTrimSlider.setEnabled(extKeyActive);
    scTrimSlider.setAlpha(extKeyActive ? 1.0f : 0.25f);
}

void JCBTransientAudioProcessorEditor::updateBackgroundState()
{
    // Obtener estados actuales - DELTA ya no cambia el fondo
    const bool bypassActive = parameterButtons.bypassButton.getToggleState();
    
    // Solo BYPASS cambia el fondo, DELTA funciona como botón normal
    if (bypassActive) {
        backgroundImage.setImage(bypassBackground, juce::RectanglePlacement::stretchToFit);
    }
    else {
        backgroundImage.setImage(normalBackground, juce::RectanglePlacement::stretchToFit);
    }
}

void JCBTransientAudioProcessorEditor::updateFilterButtonText()
{
    // Actualizar texto del botón HPF Order
    if (auto* hpfParam = processor.apvts.getParameter("j_HPFORDER"))
    {
        float normalized = hpfParam->getValue();
        float denormalized = hpfParam->convertFrom0to1(normalized);
        int value = static_cast<int>(denormalized + 0.5f);
        
        switch (value)
        {
            case 0: sidechainControls.hpfOrderButton.setButtonText("12"); break;
            case 1: sidechainControls.hpfOrderButton.setButtonText("24"); break;
            default: sidechainControls.hpfOrderButton.setButtonText("12"); break;
        }
    }
    
    // Actualizar texto del botón LPF Order
    if (auto* lpfParam = processor.apvts.getParameter("k_LPFORDER"))
    {
        float normalized = lpfParam->getValue();
        float denormalized = lpfParam->convertFrom0to1(normalized);
        int value = static_cast<int>(denormalized + 0.5f);
        
        switch (value)
        {
            case 0: sidechainControls.lpfOrderButton.setButtonText("12"); break;
            case 1: sidechainControls.lpfOrderButton.setButtonText("24"); break;
            default: sidechainControls.lpfOrderButton.setButtonText("12"); break;
        }
    }
}

void JCBTransientAudioProcessorEditor::updateDmodeButtonText()
{
    // Actualizar texto del botón DMODE basado en el parámetro h_DELTAMODE
    if (auto* dmodeParam = processor.apvts.getParameter("h_DELTAMODE"))
    {
        float normalized = dmodeParam->getValue();
        float denormalized = dmodeParam->convertFrom0to1(normalized);
        int value = static_cast<int>(denormalized + 0.5f);
        
        // 0 = TRANS (OFF), 2 = SUST (ON), 1 = Unused
        if (value == 0) {
            rightTopControls.dmodeButton.setButtonText("TRANS");
            // TRANS está OFF: usar buttonColourId (estado OFF) con naranja claro
            rightTopControls.dmodeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffFD9966).withAlpha(0.4f));
            rightTopControls.dmodeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
            // Texto blanco para máxima legibilidad sobre fondo naranja
            rightTopControls.dmodeButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
        } else if (value == 2) {
            rightTopControls.dmodeButton.setButtonText("SUST");
            // SUST está ON: usar buttonOnColourId (estado ON) con azul
            rightTopControls.dmodeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            rightTopControls.dmodeButton.setColour(juce::TextButton::buttonOnColourId, DarkTheme::accent.withAlpha(0.3f));
            // Texto blanco para máxima legibilidad sobre fondo azul
            rightTopControls.dmodeButton.setColour(juce::TextButton::textColourOnId, DarkTheme::textPrimary);
        } else {
            // Fallback para valor inesperado
            rightTopControls.dmodeButton.setButtonText("TRANS");
            rightTopControls.dmodeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffFD9966).withAlpha(0.5f));
            rightTopControls.dmodeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
            rightTopControls.dmodeButton.setColour(juce::TextButton::textColourOffId, DarkTheme::textPrimary);
        }
    }
}

void JCBTransientAudioProcessorEditor::updateDmodeButtonState()
{
    // Habilitar/deshabilitar botón DMODE basado en el estado de DELTA
    const bool deltaActive = parameterButtons.deltaButton.getToggleState();
    
    rightTopControls.dmodeButton.setEnabled(deltaActive);
    rightTopControls.dmodeButton.setAlpha(deltaActive ? 1.0f : 0.5f);  // Alpha reducido cuando deshabilitado
}

void JCBTransientAudioProcessorEditor::updateMeterStates()
{
    // Obtener estados actuales - DELTA ya no cambia colores de medidores
    const bool bypassActive = parameterButtons.bypassButton.getToggleState();
    const bool soloScActive = sidechainControls.soloScButton.getToggleState();
    
    // Solo SOLO SC cambia colores de medidores
    // DELTA funciona como botón normal sin cambios visuales
    bool soloMode = soloScActive && !bypassActive;
    
    // ELIMINADO: inputMeterL.setDeltaMode() - DELTA ya no cambia colores
    
    inputMeterL.setSoloScMode(soloMode);
    inputMeterR.setSoloScMode(soloMode);
    outputMeterL.setSoloScMode(soloMode);
    outputMeterR.setSoloScMode(soloMode);
    scMeterL.setSoloScMode(soloMode);
    scMeterR.setSoloScMode(soloMode);
    
    // Ocultar gain reduction meter cuando SOLO SC está activo (no hay compresión activa)
    // También considerar si graphics está activo para mantener consistencia
    // Ya no hay gain reduction meter que ocultar
    
    // Actualizar gradiente de salida para modo bypass
    outputMeterL.setBypassMode(bypassActive);
    outputMeterR.setBypassMode(bypassActive);
}

void JCBTransientAudioProcessorEditor::updateTransferDisplay()
{
    // Actualizar los valores de los parámetros desde los sliders
    float thresholdDB = leftTopKnobs.attackSlider.getValue();
    transferDisplay.setThreshold(thresholdDB);
    transferDisplay.setRatio(leftTopKnobs.sustainSlider.getValue());
    transferDisplay.setKnee(leftTopKnobs.sensSlider.getValue());
    
    // Actualizar estados de sidechain
    transferDisplay.setExtKeyActive(sidechainControls.keyButton.getToggleState());
    transferDisplay.setSoloSidechain(sidechainControls.soloScButton.getToggleState());
    
    // Actualizar la curva visual
    transferDisplay.updateCurve();
}

void JCBTransientAudioProcessorEditor::updateMeters()
{
    // CORRECCIÓN CRÍTICA: Cachear estados de visibilidad para evitar llamadas repetidas a setVisible
    static bool lastSoloScActive = false;
    static bool lastExtKeyActive = false;
    
    // Verificar si SOLO SC está activo
    const bool soloScActive = sidechainControls.soloScButton.getToggleState();
    const bool extKeyActive = sidechainControls.keyButton.getToggleState();
    
    // Solo actualizar visibilidad cuando el estado realmente cambia
    if (soloScActive != lastSoloScActive || extKeyActive != lastExtKeyActive) {
        if (soloScActive) {
            // SOLO SC activo - mostrar meters apropiados basados en EXT KEY
            if (extKeyActive) {
                // Ocultar medidores principales de entrada
                inputMeterL.setVisible(false);
                inputMeterR.setVisible(false);
                // Hacer visibles los medidores de sidechain
                scMeterL.setVisible(true);
                scMeterR.setVisible(true);
            } else {
                // Mostrar medidores principales de entrada en modo SOLO SC (sidechain interno)
                inputMeterL.setVisible(true);
                inputMeterR.setVisible(true);
                scMeterL.setVisible(true);
                scMeterR.setVisible(true);
            }
        } else {
            // Modo normal - mostrar todos los medidores normalmente
            inputMeterL.setVisible(true);
            inputMeterR.setVisible(true);
            scMeterL.setVisible(true);
            scMeterR.setVisible(true);
        }
        
        lastSoloScActive = soloScActive;
        lastExtKeyActive = extKeyActive;
    }
    
    // Actualizar medidores con control centralizado a 90 Hz
    // Llamar a updateLevel() de cada medidor para procesar valores y animaciones
    if (inputMeterL.isVisible()) {
        inputMeterL.updateLevel();
        inputMeterR.updateLevel();
        inputMeterL.repaint();
        inputMeterR.repaint();
    }
    
    if (scMeterL.isVisible()) {
        scMeterL.updateLevel();
        scMeterR.updateLevel();
        scMeterL.repaint();
        scMeterR.repaint();
    }
    
    // Siempre actualizar detección de clip de sidechain
    scMeterL.setClipDetected(processor.getSidechainClipDetected(0));
    scMeterR.setClipDetected(processor.getSidechainClipDetected(1));
    
    // Siempre actualizar medidores de salida y reducción de ganancia
    // Ya no hay grMeter que actualizar
    
    outputMeterL.updateLevel();
    outputMeterR.updateLevel();
    outputMeterL.repaint();
    outputMeterR.repaint();
}


void JCBTransientAudioProcessorEditor::updateSliderValues()
{
    // Actualizar todos los sliders con los valores actuales del APVTS
    // Esto soluciona el problema de que los valores no se actualizan al cargar sesión
    // FIXED: Los comentarios anteriores eran incorrectos - todos los sliders usan CustomSliderAttachment
    
    // Left top knobs - Todos usan CustomSliderAttachment
    if (auto* param = processor.apvts.getRawParameterValue("b_THD"))
        leftTopKnobs.attackSlider.setValue(param->load(), juce::dontSendNotification);
    
    if (auto* param = processor.apvts.getRawParameterValue("c_RATIO"))
        leftTopKnobs.sustainSlider.setValue(param->load(), juce::dontSendNotification);
        
    if (auto* param = processor.apvts.getRawParameterValue("q_KNEE"))
        leftTopKnobs.sensSlider.setValue(param->load(), juce::dontSendNotification);
    
    // Left bottom knobs
    if (auto* param = processor.apvts.getRawParameterValue("o_DRYWET"))
        leftBottomKnobs.drywetSlider.setValue(param->load(), juce::dontSendNotification);
        
    if (auto* param = processor.apvts.getRawParameterValue("n_LOOKAHEAD"))
        leftBottomKnobs.lookaheadSlider.setValue(param->load(), juce::dontSendNotification);
        
    if (auto* param = processor.apvts.getRawParameterValue("u_SOFTCLIP"))
        leftBottomKnobs.clipSlider.setValue(param->load(), juce::dontSendNotification);
        
    
    // Right top controls
        
    // REACT ELIMINADO - parámetro ya no existe
    
    // Right bottom knobs - Todos usan CustomSliderAttachment
    if (auto* param = processor.apvts.getRawParameterValue("d_ATK"))
        rightBottomKnobs.atkSlider.setValue(param->load(), juce::dontSendNotification);
    
    if (auto* param = processor.apvts.getRawParameterValue("e_REL"))
        rightBottomKnobs.relSlider.setValue(param->load(), juce::dontSendNotification);
        
        
    if (auto* param = processor.apvts.getRawParameterValue("z_SMOOTH"))
        rightTopControls.smoothSlider.setValue(param->load(), juce::dontSendNotification);
    
    // Controles de sidechain - Todos usan CustomSliderAttachment
    if (auto* param = processor.apvts.getRawParameterValue("j_HPF"))
        sidechainControls.hpfSlider.setValue(param->load(), juce::dontSendNotification);
        
    if (auto* param = processor.apvts.getRawParameterValue("k_LPF"))
        sidechainControls.lpfSlider.setValue(param->load(), juce::dontSendNotification);
    

    // Slider de trims (both linked to the same parameter)
    if (auto* param = processor.apvts.getRawParameterValue("a_TRIM")) {
        float trimValue = param->load();
        trimSlider.setValue(trimValue, juce::dontSendNotification);
    }
    
    // Slider de makeups (both linked to the same parameter)
    if (auto* param = processor.apvts.getRawParameterValue("i_MAKEUP")) {
        float makeupValue = param->load();
        makeupSlider.setValue(makeupValue, juce::dontSendNotification);
    }
    
    // Slider de trim de sidechain
    if (auto* param = processor.apvts.getRawParameterValue("y_SCTRIM")) {
        float scTrimValue = param->load();
        scTrimSlider.setValue(scTrimValue, juce::dontSendNotification);
    }
}

void JCBTransientAudioProcessorEditor::resetGuiSize()
{
    // Ciclar al siguiente estado: Current -> Maximum -> Minimum -> Current
    switch (currentSizeState) {
        case GuiSizeState::Current:
            // Current -> Maximum
            processor.setSavedSize({MAX_WIDTH, MAX_HEIGHT});
            setSize(MAX_WIDTH, MAX_HEIGHT);
            currentSizeState = GuiSizeState::Maximum;
            break;

        case GuiSizeState::Maximum:
            // Maximum -> Minimum
            processor.setSavedSize({MIN_WIDTH, MIN_HEIGHT});
            setSize(MIN_WIDTH, MIN_HEIGHT);
            currentSizeState = GuiSizeState::Minimum;
            break;

        case GuiSizeState::Minimum:
            // Minimum -> Current (back to default)
            processor.setSavedSize({DEFAULT_WIDTH, DEFAULT_HEIGHT});
            setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
            currentSizeState = GuiSizeState::Current;
            break;
    }
    
    // Forzar repaint para asegurar que se actualice correctamente
    repaint();
}




//==============================================================================
// GESTIÓN DE PRESETS
//==============================================================================
juce::File JCBTransientAudioProcessorEditor::getPresetsFolder()
{
    juce::File folder = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory)
        .getChildFile("Audio")
        .getChildFile("Presets")
        .getChildFile("Coeval")
        .getChildFile("JCBTransient");

    if (!folder.isDirectory() && !folder.existsAsFile()) {
        folder.createDirectory();
    }
    
    return folder;
}

juce::Array<juce::File> JCBTransientAudioProcessorEditor::populatePresetFolder()
{
    return getPresetsFolder().findChildFiles(2, false, "*.preset");
}

void JCBTransientAudioProcessorEditor::refreshPresetMenu()
{
    presetArea.presetMenu.clear();
    juce::StringArray presetNames;
    
    // Añadir DEFAULT como primer preset siempre
    presetNames.add("DEFAULT");
    
    // Añadir separador para factory presets
    presetNames.add("--- Factory Presets ---");
    
    // Añadir factory presets desde BinaryData
    juce::StringArray factoryPresetNames;
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        juce::String resourceName(BinaryData::namedResourceList[i]);
        
        // Buscar archivos que terminen en "_preset" (los .preset se convierten a _preset en BinaryData)
        if (resourceName.endsWith("_preset"))
        {
            // Limpiar el nombre para mostrar
            juce::String cleanName = resourceName.replace("_preset", "")
                                               .replace("_", " ");
            factoryPresetNames.add(cleanName);
        }
    }
    
    // Ordenar factory presets alfabéticamente
    factoryPresetNames.sort(true);
    for (const auto& name : factoryPresetNames)
    {
        presetNames.add("[F] " + name);  // [F] indica Factory preset
    }
    
    // Añadir separador para user presets
    presetNames.add("--- User Presets ---");
    
    // Añadir los presets del disco (user presets)
    juce::StringArray userPresets;
    for (int i = 0; i < getUpdatedNumPresets(); i++) {
        userPresets.add(populatePresetFolder()[i].getFileNameWithoutExtension());
    }
    
    // Ordenar user presets alfabéticamente
    userPresets.sort(true);
    presetNames.addArray(userPresets);
    
    // Añadir todos los items al menú
    presetArea.presetMenu.addItemList(presetNames, 1);
    
    // Si había un preset seleccionado previamente, intentar restaurarlo
    if (processor.getLastPreset() > 0 && processor.getLastPreset() <= presetNames.size()) {
        presetArea.presetMenu.setSelectedId(processor.getLastPreset());
    }
}

void JCBTransientAudioProcessorEditor::savePresetFile()
{
    // Obtener estado del menú de presets y del processor
    juce::String currentText = presetArea.presetMenu.getText();
    juce::String processorText = processor.getPresetDisplayText();
    
    // Si no hay texto en el menú, usar el texto del processor
    if (currentText.isEmpty() && !processorText.isEmpty()) {
        currentText = processorText;
    }
    
    // Extraer el nombre del preset sin asterisco
    juce::String currentPresetName = currentText.endsWith("*") ? 
        currentText.dropLastCharacters(1).trimEnd() : currentText;
    
    // Si no hay preset o es DEFAULT, ir a Save As
    if (currentPresetName.isEmpty() || currentPresetName == "DEFAULT") {
        saveAsPresetFile();
        return;
    }
    
    // Verificar si es un factory preset
    bool isFactoryPreset = currentPresetName.startsWith("[F] ");
    if (isFactoryPreset) {
        // Quitar el prefijo [F] para mostrar el nombre limpio
        juce::String cleanName = currentPresetName.substring(4);
        showCustomAlertDialog(JUCE_UTF8("Cannot overwrite"), 
                            JUCE_UTF8("Factory Presets are read-only.\nUse 'Save As' to create a copy."));
        return;
    }
    
    // Si hay un preset cargado (modificado o no), ofrecer opciones
    // Mostrar diálogo si hay un preset cargado (modificado O no modificado)
    if (!currentPresetName.isEmpty()) {
        overwritePresetDialog = std::make_unique<CustomThreeButtonDialog>(
            "Overwrite \"" + currentPresetName + "\"",
            "",  // No mensaje adicional, el título es suficiente
            "Overwrite",
            "Save As",
            "Cancel"
        );
        
        overwritePresetDialog->onDialogClosed = [this, currentPresetName](bool wasAccepted) {
            if (wasAccepted) {
                int result = overwritePresetDialog->getSelectedButton();
                if (result == 1) { // Overwrite
                // Guardar directamente en el archivo existente
                auto presetFile = getPresetsFolder().getChildFile(currentPresetName + ".preset");
                auto state = processor.apvts.copyState();
                std::unique_ptr<juce::XmlElement> xml(state.createXml());
                
                if (xml != nullptr) {
                    xml->writeTo(presetFile);
                    
                    // Actualizar estado visual
                    processor.setPresetDisplayText(currentPresetName);
                    processor.setPresetTextItalic(false);
                    presetArea.presetMenu.setTextWhenNothingSelected(currentPresetName);
                    presetArea.presetMenu.setTextItalic(false);
                    
                    // Actualizar menú si es necesario
                    if (presetArea.presetMenu.getSelectedId() <= 0) {
                        refreshPresetMenu();
                        for (int i = 0; i < presetArea.presetMenu.getNumItems(); i++) {
                            if (presetArea.presetMenu.getItemText(i) == currentPresetName) {
                                presetArea.presetMenu.setSelectedId(i + 1);
                                processor.setLastPreset(i + 1);
                                break;
                            }
                        }
                    }
                }
                } else if (result == 2) { // Save As...
                    saveAsPresetFile();
                }
                // result == 3 is Cancel, do nothing
            }
            
            // Limpiar el diálogo después de usarlo
            overwritePresetDialog.reset();
        };
        
        addChildComponent(overwritePresetDialog.get());
        overwritePresetDialog->showDialog();
    } else {
        // No hay preset cargado, ir directamente a "Save As..."
        saveAsPresetFile();
    }
}

void JCBTransientAudioProcessorEditor::saveAsPresetFile()
{
    // Crear y mostrar el diálogo personalizado
    savePresetDialog = std::make_unique<SavePresetDialog>("");
    savePresetDialog->onDialogClosed = [this](bool shouldSave) {
        if (shouldSave) {
            juce::String presetName = savePresetDialog->getPresetName();
            
            // No permitir usar el nombre DEFAULT
            if (presetName.toUpperCase() == "DEFAULT") {
                showCustomAlertDialog("Error", 
                                    "Cannot use the name DEFAULT for a preset.");
                return;
            }
            
            // Verificar si el nombre coincide con algún factory preset
            for (const auto& factoryName : factoryPresetNames) {
                if (presetName.compareIgnoreCase(factoryName) == 0) {
                    showCustomAlertDialog(JUCE_UTF8("Reserved name"), 
                                        JUCE_UTF8("Name reserved for Factory Preset.\nPlease choose another name."));
                    return;
                }
            }
            
            // Crear archivo con el nombre del preset
            juce::File presetFile = getPresetsFolder().getChildFile(presetName + ".preset");
            
            // Si el archivo ya existe, preguntar si sobrescribir
            if (presetFile.existsAsFile()) {
                showCustomConfirmDialog(JUCE_UTF8("Preset already exists"), 
                                      JUCE_UTF8("Do you want to overwrite the existing preset?"), 
                                      [this, presetFile, presetName](bool shouldOverwrite) {
                    if (shouldOverwrite) {
                        // Guardar el preset
                        auto state = processor.apvts.copyState();
                        std::unique_ptr<juce::XmlElement> xml(state.createXml());
                        
                        if (xml != nullptr) {
                            xml->writeTo(presetFile);
                            
                            // Actualizar el menú
                            refreshPresetMenu();
                            
                            // Seleccionar el preset recién guardado
                            for (int i = 0; i < presetArea.presetMenu.getNumItems(); i++) {
                                if (presetArea.presetMenu.getItemText(i) == presetName) {
                                    presetArea.presetMenu.setSelectedId(i + 1);
                                    processor.setLastPreset(i + 1);
                                    processor.setPresetDisplayText(presetName);
                                    processor.setPresetTextItalic(false);
                                    break;
                                }
                            }
                        }
                    }
                });
            } else {
                // Guardar directamente si no existe
                auto state = processor.apvts.copyState();
                std::unique_ptr<juce::XmlElement> xml(state.createXml());
                
                if (xml != nullptr) {
                    xml->writeTo(presetFile);
                    
                    // Actualizar el menú
                    refreshPresetMenu();
                    
                    // Seleccionar el preset recién guardado
                    for (int i = 0; i < presetArea.presetMenu.getNumItems(); i++) {
                        if (presetArea.presetMenu.getItemText(i) == presetName) {
                            presetArea.presetMenu.setSelectedId(i + 1);
                            processor.setLastPreset(i + 1);
                            processor.setPresetDisplayText(presetName);
                            processor.setPresetTextItalic(false);
                            break;
                        }
                    }
                }
            }
        }
        
        // Limpiar el diálogo
        savePresetDialog.reset();
    };
    
    addChildComponent(savePresetDialog.get());
    savePresetDialog->showDialog();
}

void JCBTransientAudioProcessorEditor::deletePresetFile()
{
    int selectedId = presetArea.presetMenu.getSelectedId();
    juce::String presetName;
    
    if (selectedId > 0) {
        // Hay un preset seleccionado en el menú
        presetName = presetArea.presetMenu.getItemText(selectedId - 1);
    } else {
        // No hay selección, verificar si hay un preset modificado
        juce::String displayText = presetArea.presetMenu.getTextWhenNothingSelected();
        if (displayText.isEmpty()) {
            displayText = processor.getPresetDisplayText();
        }
        
        if (displayText.isEmpty()) {
            showCustomAlertDialog(JUCE_UTF8("Error"), 
                                JUCE_UTF8("Ningún preset seleccionado."));
            return;
        }
        
        // Quitar asterisco si lo tiene
        presetName = displayText.endsWith("*") ? 
            displayText.dropLastCharacters(1).trimEnd() : displayText;
    }
    
    // No permitir borrar DEFAULT ni factory presets
    if (presetName == "DEFAULT" || presetName.startsWith("[F] ")) {
        juce::String errorMsg = presetName == "DEFAULT" ? 
            JUCE_UTF8("The DEFAULT preset cannot be deleted.") :
            JUCE_UTF8("Factory Presets cannot be deleted.");
        showCustomAlertDialog(JUCE_UTF8("Error"), errorMsg);
        return;
    }
    
    showCustomConfirmDialog(JUCE_UTF8("Delete Preset"), 
                            JUCE_UTF8("The preset \"") + presetName + JUCE_UTF8("\" will be moved to trash."), 
                            [this, presetName](bool confirmed) {
        if (confirmed) {
            juce::File presetFile = getPresetsFolder().getChildFile(presetName + ".preset");
            
            if (presetFile.existsAsFile()) {
                presetFile.moveToTrash();
                
                // Actualizar el menú
                refreshPresetMenu();
                
                // Dejar el menú sin selección y sin texto
                presetArea.presetMenu.setSelectedId(0);
                presetArea.presetMenu.setTextWhenNothingSelected("");
                presetArea.presetMenu.setTextItalic(false);
                processor.setPresetDisplayText("");
                processor.setPresetTextItalic(false);
                processor.setLastPreset(0);
                
                // NO cargar DEFAULT - dejar los parámetros como están
            }
        }
    });
}

void JCBTransientAudioProcessorEditor::selectNextPreset()
{
    int currentId = presetArea.presetMenu.getSelectedId();
    int numItems = presetArea.presetMenu.getNumItems();
    
    if (numItems > 1) {
        int nextId = currentId;
        
        // Buscar el siguiente preset válido (no separador)
        do {
            nextId = (nextId % numItems) + 1;
            juce::String itemText = presetArea.presetMenu.getItemText(nextId - 1);
            
            // Si no es un separador, es un preset válido
            if (!itemText.startsWith("---")) {
                break;
            }
        } while (nextId != currentId); // Evitar bucle infinito
        
        presetArea.presetMenu.setSelectedId(nextId);
        
        // Trigger onChange para cargar el preset
        if (presetArea.presetMenu.onChange) {
            presetArea.presetMenu.onChange();
        }
    }
}

void JCBTransientAudioProcessorEditor::selectPreviousPreset()
{
    int currentId = presetArea.presetMenu.getSelectedId();
    int numItems = presetArea.presetMenu.getNumItems();
    
    if (numItems > 1) {
        int prevId = currentId;
        
        // Buscar el anterior preset válido (no separador)
        do {
            prevId = prevId - 1;
            if (prevId < 1) prevId = numItems;
            
            juce::String itemText = presetArea.presetMenu.getItemText(prevId - 1);
            
            // Si no es un separador, es un preset válido
            if (!itemText.startsWith("---")) {
                break;
            }
        } while (prevId != currentId); // Evitar bucle infinito
        
        presetArea.presetMenu.setSelectedId(prevId);
        
        // Trigger onChange para cargar el preset
        if (presetArea.presetMenu.onChange) {
            presetArea.presetMenu.onChange();
        }
    }
}

//==============================================================================
// DIÁLOGOS Y OVERLAYS
//==============================================================================
void JCBTransientAudioProcessorEditor::showCustomConfirmDialog(const juce::String& message, 
                                                          const juce::String& subMessage,
                                                          std::function<void(bool)> callback,
                                                          const juce::String& confirmText,
                                                          const juce::String& cancelText)
{
    // Combinar mensaje principal y submensaje si existe
    juce::String fullMessage = message;
    if (subMessage.isNotEmpty()) {
        fullMessage += "\n\n" + subMessage;
    }
    
    // Usar el nuevo diálogo personalizado
    deleteConfirmDialog = std::make_unique<CustomConfirmDialog>("Confirmation", fullMessage, confirmText, cancelText);
    deleteConfirmDialog->onDialogClosed = callback;
    addChildComponent(deleteConfirmDialog.get());
    deleteConfirmDialog->showDialog();
}

void JCBTransientAudioProcessorEditor::showCustomAlertDialog(const juce::String& title, const juce::String& message)
{
    // Crear un diálogo de alerta simple con solo OK
    alertDialog = std::make_unique<CustomAlertDialog>(title, message);
    alertDialog->onDialogClosed = [](bool) {}; // Callback vacío
    addChildComponent(alertDialog.get());
    alertDialog->showDialog();
}

void JCBTransientAudioProcessorEditor::showCredits()
{
    // Desactivar estados operacionales antes de mostrar créditos - DELTA ahora es independiente
    parameterButtons.bypassButton.setToggleState(false, juce::sendNotification);
    sidechainControls.soloScButton.setToggleState(false, juce::sendNotification);
    
    if (creditsOverlay == nullptr)
    {
        // Obtener el formato del plugin desde el processor
        juce::String format = processor.getPluginFormat();
        
        creditsOverlay = std::make_unique<CreditsOverlay>(format);
        creditsOverlay->setBounds(getLocalBounds());
        creditsOverlay->onClose = [this]() {
            hideCredits();
        };
        
        addAndMakeVisible(creditsOverlay.get());
        creditsOverlay->grabKeyboardFocus();
    }
}

void JCBTransientAudioProcessorEditor::hideCredits()
{
    if (creditsOverlay != nullptr)
    {
        removeChildComponent(creditsOverlay.get());
        creditsOverlay.reset();
    }
}

//==============================================================================
// SISTEMA DE TOOLTIPS
//==============================================================================
void JCBTransientAudioProcessorEditor::updateTodoButtonTexts()
{
    // Update TODO button tooltips based on current language
    bool isSpanish = (currentLanguage == TooltipLanguage::Spanish);
    
    // HQ button tooltip
    utilityButtons.hqButton.setTooltip(isSpanish ? JUCE_UTF8("POR HACER: Habilita oversampling para mayor calidad.") : "TODO: Enables oversampling for higher quality.");
    
    // Dual Mono button tooltip
    utilityButtons.dualMonoButton.setTooltip(isSpanish ? JUCE_UTF8("POR HACER: Procesa canales L/R independientemente.") : "TODO: Processes L/R channels independently.");
    
    // Stereo Linked button - mantener tooltip actual
    // utilityButtons.stereoLinkedButton tooltip ya está implementado
    
    // Botón M/S tooltip
    utilityButtons.msButton.setTooltip(isSpanish ? JUCE_UTF8("POR HACER: Procesa en formato Mid/Side.") : "TODO: Processes in M/S format.");
    
    // A/B State button tooltip
    topButtons.abStateButton.setTooltip(isSpanish ? JUCE_UTF8("Alterna entre dos configuraciones A/B para comparar ajustes.") : "Switches between two A/B configurations to compare settings.");
    
    // MIDI Learn button tooltip
    utilityButtons.midiLearnButton.setTooltip(isSpanish ? JUCE_UTF8("POR HACER: Asigna control MIDI.") : "TODO: Assigns MIDI control.");
    
    
    // Zoom y Diagram mantienen sus tooltips actuales ya que son funcionales
}

void JCBTransientAudioProcessorEditor::updateAllTooltips()
{
    // Actualizar todos los tooltips de componentes basado en el idioma actual
    
    // Título
    titleLink.setTooltip(getTooltipText("title"));
    
    // Perillas - superiores izquierdas
    leftTopKnobs.attackSlider.setTooltip(getTooltipText("tran"));
    leftTopKnobs.sustainSlider.setTooltip(getTooltipText("sust"));
    leftTopKnobs.sensSlider.setTooltip(getTooltipText("sens"));
    
    // Botón FLIP
    leftTopKnobs.flipButton.setTooltip(getTooltipText("flip"));
    
    // Perillas - inferiores izquierdas
    leftBottomKnobs.drywetSlider.setTooltip(getTooltipText("drywet"));
    leftBottomKnobs.lookaheadSlider.setTooltip(getTooltipText("lookahead"));
    leftBottomKnobs.clipSlider.setTooltip(getTooltipText("clip"));
    
    // Perillas - superiores derechas
    rightTopControls.dmodeButton.setTooltip(getTooltipText("dmode"));
    // REACT ELIMINADO
    rightTopControls.smoothSlider.setTooltip(getTooltipText("smooth"));
    
    // Perillas - inferiores derechas
    rightBottomKnobs.atkSlider.setTooltip(getTooltipText("attack"));
    rightBottomKnobs.relSlider.setTooltip(getTooltipText("release"));
    rightBottomKnobs.holdSlider.setTooltip(getTooltipText("hold"));
    // speedButton removed
    parameterButtons.deltaButton.setTooltip(getTooltipText("delta"));
    
    // Sliders de trim y makeup
    trimSlider.setTooltip(getTooltipText("trim"));
    makeupSlider.setTooltip(getTooltipText("makeup"));
    
    // Sliders de trim de sidechain
    scTrimSlider.setTooltip(getTooltipText("sctrim"));
    
    // Controles de sidechain
    sidechainControls.scButton.setTooltip(getTooltipText("sc"));
    sidechainControls.keyButton.setTooltip(getTooltipText("extkey"));
    sidechainControls.soloScButton.setTooltip(getTooltipText("solosc"));
    sidechainControls.hpfSlider.setTooltip(getTooltipText("hpf"));
    sidechainControls.lpfSlider.setTooltip(getTooltipText("lpf"));
    
    // Área de presets
    presetArea.saveButton.setTooltip(getTooltipText("save"));
    presetArea.saveAsButton.setTooltip(getTooltipText("saveas"));
    presetArea.deleteButton.setTooltip(getTooltipText("delete"));
    presetArea.backButton.setTooltip(getTooltipText("back"));
    presetArea.nextButton.setTooltip(getTooltipText("next"));
    
    // Utility buttons
    utilityButtons.undoButton.setTooltip(getTooltipText("undo"));
    utilityButtons.redoButton.setTooltip(getTooltipText("redo"));
    utilityButtons.resetGuiButton.setTooltip(getTooltipText("resetgui"));
    utilityButtons.runGraphicsButton.setTooltip(getTooltipText("graphics"));
    utilityButtons.zoomButton.setTooltip(getTooltipText("zoom"));
    centerButtons.diagramButton.setTooltip(getTooltipText("diagram"));
    utilityButtons.tooltipToggleButton.setTooltip(getTooltipText("tooltiptoggle"));
    utilityButtons.tooltipLangButton.setTooltip(getTooltipText("tooltiplang"));

    // Botones de parámetros
    parameterButtons.bypassButton.setTooltip(getTooltipText("bypass"));

    
    // Update TODO button tooltips
    updateTodoButtonTexts();
    
    // Actualizar tooltip del transfer display
    transferDisplay.setHelpText(getTooltipText("transfer"));
    
    // Botones de utilidad - fila inferior
    utilityButtons.stereoLinkedButton.setTooltip(getTooltipText("link"));
    // CODE button removed
}

juce::String JCBTransientAudioProcessorEditor::getTooltipText(const juce::String& key)
{
    if (currentLanguage == TooltipLanguage::Spanish)
    {
        // Spanish tooltips
        if (key == "title") return JUCE_UTF8("JCBTransient: diseñador de transientes v0.9.1 beta\nPlugin de audio open source\nClick para créditos");
        if (key == "tran") return JUCE_UTF8("TRANS: ganancia para transientes entre -18 y +18 dB.\nValores positivos realzan ataques, negativos los atenúan.\nValor por defecto: 0 dB");
        if (key == "sust") return JUCE_UTF8("SUST: ganancia para sustain entre -18 y +18 dB.\nValores positivos realzan sustains, negativos los atenúan.\nValor por defecto: 0 dB");
        if (key == "sens") return JUCE_UTF8("SENS: sensibilidad de detección entre 0% y 100%.\nControla la sensibilidad del algoritmo de detección de transientes.\nValor por defecto: 50%");
        if (key == "drywet") return JUCE_UTF8("DRY/WET: mezcla final entre señal original y procesada\nControl de balance entrada/salida\nRango: 0 a 100% | Por defecto: 100%");
        if (key == "lookahead") return JUCE_UTF8("LOOK AHEAD: retardo para evitar overshooting\nReporta latencia al host\nRango: 0 a 10 ms | Por defecto: 0 ms");
        if (key == "clip") return JUCE_UTF8("SOFT CLIP: limitador suave de salida\nPreviene saturación con distorsión armónica\nRango: 0/OFF a 100% | Por defecto: OFF");
        if (key == "attack") return JUCE_UTF8("ATTACK: tiempo para alcanzar máxima ganancia/atenuación\nVelocidad de respuesta\nRango: 0 a 250 ms | Por defecto: 0 ms");
        if (key == "release") return JUCE_UTF8("RELEASE: tiempo de recuperación\nPermite valores extremos para efectos creativos\nRango: 0.1 a 350 ms | Por defecto: 60 ms");
        if (key == "hold") return JUCE_UTF8("HOLD: tiempo de retención antes de que comience release\nMantiene la ganancia/atenuación por un período\nRango: 0 a 250 ms | Por defecto: 10 ms");
        if (key == "dmode") return JUCE_UTF8("DMODE: modo de escucha solo delta (TRANS/SUST)\nConmuta cuando Delta está activo entre:s\nTRANS: ataque | SUST: sustain | Por defecto: TRANS");
        if (key == "delta") return JUCE_UTF8("SOLO DELTA: escucha solo la ganancia/reducción aplicada\nParámetro automatizable, se guarda en presets\nRango: OFF/ON | Por defecto: OFF");
        if (key == "trim") return JUCE_UTF8("TRIM INPUT: ganancia de entrada del procesador\nAjusta el nivel antes del procesamiento\nRango: -12 a +12 dB | Por defecto: 0 dB");
        if (key == "makeup") return JUCE_UTF8("MAKEUP: ganancia de salida manual\nAjusta el nivel final después del procesamiento\nRango: -12 a +12 dB | Por defecto: 0 dB");
        if (key == "sc") return JUCE_UTF8("FILTERS: activa los filtros del sidechain.\nPermite filtrar la señal, tanto interna como externa, que controla el expansor.\nValor por defecto: OFF");
        if (key == "extkey") return JUCE_UTF8("SIDECHAIN: selecciona cadena lateral interna o externa.\nINT usa la propia señal, EXT usa entradas auxiliares.\nValor por defecto: INT");
        if (key == "solosc") return JUCE_UTF8("SOLO SC: escucha filtros sidechain int/ext\nParámetro global, no automatizable\nRango: OFF/ON | Por defecto: OFF");
        if (key == "hpf") return JUCE_UTF8("HPF: filtro pasa altos del sidechain\nFiltra frecuencias del detector de expansión\nRango: 20 a 20k Hz | Por defecto: 20 Hz");
        if (key == "lpf") return JUCE_UTF8("LPF: filtro pasa bajos del sidechain\nElimina frecuencias agudas del detector\nRango: 20 Hz a 20 kHz | Por defecto: 20 kHz");
        if (key == "save") return JUCE_UTF8("GUARDAR: guarda o sobrescribe preset\nGuarda nuevo o actualiza el preset actual");
        if (key == "saveas") return JUCE_UTF8("SAVE AS: guarda como nuevo preset\nCrea un nuevo archivo de preset con los valores actuales\nPermite crear presets personalizados");
        if (key == "delete") return JUCE_UTF8("BORRAR: elimina el preset seleccionado\nRequiere confirmación antes de borrar");
        if (key == "back") return JUCE_UTF8("ANTERIOR: selecciona el preset previo\nNavega hacia atrás en la lista de presets");
        if (key == "next") return JUCE_UTF8("SIGUIENTE: selecciona el próximo preset\nNavega hacia adelante en la lista de presets");
        if (key == "undo") return JUCE_UTF8("DESHACER: revierte el último cambio\nDeshace modificación realizada manualmente por el usuario\nHistorial: hasta 20 pasos");
        if (key == "redo") return JUCE_UTF8("REHACER: aplica el cambio deshecho\nRehace modificación manual previamente revertida\nHistorial: hasta 20 pasos");
        if (key == "resetgui") return JUCE_UTF8("SIZE: cicla entre tamaños de ventana\nActual → Máximo → Mínimo → Actual\nAjuste rápido del tamaño del plugin");
        if (key == "bypass") return JUCE_UTF8("BYPASS: desactiva el procesamiento del plugin\nParámetro global, no automatizable. Transición suave\nRango: OFF/ON | Por defecto: OFF");
        if (key == "graphics") return JUCE_UTF8("GRAPHICS: muestra envolventes en tiempo real\nVisualiza env entrada/salida e histograma expansión\nDesactivar mejora rendimiento en CPUs lentas");
        if (key == "zoom") return JUCE_UTF8("ZOOM: cicla entre vista normal y ampliado x2\n");
        if (key == "diagram") return JUCE_UTF8("DIAGRAM: muestra diagrama de bloques del procesador\nDespliega menú con código GenExpr por bloque para copiar");
        if (key == "transfer") return JUCE_UTF8("GRÁFICA: visualización de envolventes del transient shaper\nMuestra formas de onda de entrada y salida\nReducción de ganancia en tiempo real");
        if (key == "tooltiptoggle") return JUCE_UTF8("TOOLTIP: muestra/oculta los tooltips de ayuda\nActiva o desactiva las ventanas de ayuda emergentes");
        if (key == "tooltiplang") return JUCE_UTF8("IDIOMA: cambia entre español e inglés.\nAlterna el idioma de los tooltips.");
        if (key == "sctrim") return JUCE_UTF8("SC TRIM: ganancia entrada sidechain -12 a +12 dB\nAjusta nivel del sidechain externo\nPor defecto: 0 dB, se activa con SC EXT");
        if (key == "link") return JUCE_UTF8("STEREO LINKED: siempre activo.\nEl plugin solo funciona en modo stereo linked.\nAmbos canales siempre están vinculados");
        if (key == "smooth") return JUCE_UTF8("SMOOTH: suavizado extra del detector de envolvente\nControla cantidad de suavizado en la detección\nRango: 0 (RAW) a 1 (SMOOTH) | Por defecto: 0");
        if (key == "flip") return JUCE_UTF8("FLIP: intercambia valores de TRANS y SUST\nÚtil para probar rápidamente el efecto inverso");
    }
    else
	{
	    // English tooltips
	    if (key == "title") return JUCE_UTF8("JCBTransient: transient designer v0.9.1 beta\nOpen source audio plugin\nClick for credits");
	    if (key == "tran") return JUCE_UTF8("TRANS: gain for transients between -18 and +18 dB.\nPositive values enhance attacks, negative values attenuate them.\nDefault: 0 dB");
	    if (key == "sust") return JUCE_UTF8("SUST: gain for sustain between -18 and +18 dB.\nPositive values enhance sustains, negative values attenuate them.\nDefault: 0 dB");
	    if (key == "sens") return JUCE_UTF8("SENS: detection sensitivity between 0% and 100%.\nControls the sensitivity of the transient detection algorithm.\nDefault: 50%");
	    if (key == "drywet") return JUCE_UTF8("DRY/WET: final mix between original and processed signal\nInput/output balance control\nRange: 0 to 100% | Default: 100%");
	    if (key == "lookahead") return JUCE_UTF8("LOOK AHEAD: delay to prevent overshooting\nReports latency to host\nRange: 0 to 10 ms | Default: 0 ms");
	    if (key == "clip") return JUCE_UTF8("SOFT CLIP: soft output limiter\nPrevents clipping with harmonic distortion\nRange: 0/OFF to 100% | Default: OFF");
	    if (key == "attack") return JUCE_UTF8("ATTACK: time to reach max gain/attenuation\nResponse speed\nRange: 0 to 250 ms | Default: 0 ms");
	    if (key == "release") return JUCE_UTF8("RELEASE: recovery time\nAllows extreme values for creative effects\nRange: 0.1 to 350 ms | Default: 60 ms");
	    if (key == "hold") return JUCE_UTF8("HOLD: hold time before release starts\nMaintains gain/attenuation for a period\nRange: 0 to 250 ms | Default: 10 ms");
	    if (key == "dmode") return JUCE_UTF8("DMODE: delta listening mode (TRANS/SUST)\nSwitches when Delta is active between:\nTRANS: attack | SUST: sustain | Default: TRANS");
	    if (key == "delta") return JUCE_UTF8("SOLO DELTA: listen to gain/reduction only\nAutomatable parameter, saved in presets\nRange: OFF/ON | Default: OFF");
	    if (key == "trim") return JUCE_UTF8("TRIM INPUT: input gain of the processor\nAdjusts level before processing\nRange: -12 to +12 dB | Default: 0 dB");
	    if (key == "makeup") return JUCE_UTF8("MAKEUP: manual output gain\nAdjusts final level after processing\nRange: -12 to +12 dB | Default: 0 dB");
	    if (key == "sc") return JUCE_UTF8("FILTERS: activates sidechain filters.\nFilters the signal, internal or external, that controls the expander.\nDefault: OFF");
	    if (key == "extkey") return JUCE_UTF8("SIDECHAIN: selects internal or external sidechain.\nINT uses the main signal, EXT uses auxiliary inputs.\nDefault: INT");
	    if (key == "solosc") return JUCE_UTF8("SOLO SC: monitor int/ext sidechain filters\nGlobal parameter, not automatable\nRange: OFF/ON | Default: OFF");
	    if (key == "hpf") return JUCE_UTF8("HPF: sidechain high-pass filter\nFilters frequencies from the expander detector\nRange: 20 to 20k Hz | Default: 20 Hz");
	    if (key == "lpf") return JUCE_UTF8("LPF: sidechain low-pass filter\nRemoves high frequencies from detector\nRange: 20 Hz to 20 kHz | Default: 20 kHz");
	    if (key == "save") return JUCE_UTF8("SAVE: save or overwrite preset\nSave new or update current preset");
	    if (key == "saveas") return JUCE_UTF8("SAVE AS: save as new preset\nCreates a new preset file with current values\nAllows creating custom presets");
	    if (key == "delete") return JUCE_UTF8("DELETE: remove selected preset\nRequires confirmation before deleting");
	    if (key == "back") return JUCE_UTF8("PREVIOUS: select previous preset\nNavigate backwards in the preset list");
	    if (key == "next") return JUCE_UTF8("NEXT: select next preset\nNavigate forward in the preset list");
	    if (key == "undo") return JUCE_UTF8("UNDO: revert last change\nUndo manually made modifications\nHistory: up to 20 steps");
	    if (key == "redo") return JUCE_UTF8("REDO: reapply undone change\nRedo manual modification previously reverted\nHistory: up to 20 steps");
	    if (key == "resetgui") return JUCE_UTF8("SIZE: cycles through window sizes\nCurrent → Maximum → Minimum → Current\nQuick plugin size adjustment");
	    if (key == "bypass") return JUCE_UTF8("BYPASS: disables plugin processing\nGlobal parameter, not automatable. Smooth transition\nRange: OFF/ON | Default: OFF");
	    if (key == "graphics") return JUCE_UTF8("GRAPHICS: shows real-time envelopes\nDisplays input/output env and expansion histogram\nDisable to improve performance on slow CPUs");
	    if (key == "zoom") return JUCE_UTF8("ZOOM: cycles between normal and 2x view\n");
	    if (key == "diagram") return JUCE_UTF8("DIAGRAM: shows processor block diagram\nDisplays menu with GenExpr code per block for copying");
	    if (key == "transfer") return JUCE_UTF8("GRAPH: transient shaper envelope visualization\nShows input and output waveforms\nReal-time gain reduction display");
	    if (key == "tooltiptoggle") return JUCE_UTF8("TOOLTIP: show/hide help tooltips\nEnables or disables popup help windows");
	    if (key == "tooltiplang") return JUCE_UTF8("LANGUAGE: switch between Spanish and English.\nToggles tooltip language.");
	    if (key == "sctrim") return JUCE_UTF8("SC TRIM: sidechain input gain -12 to +12 dB\nAdjusts external sidechain level\nDefault: 0 dB, activated with SC EXT");
	    if (key == "link") return JUCE_UTF8("STEREO LINKED: always active.\nPlugin only works in stereo linked mode.\nBoth channels are always linked");
	    if (key == "smooth") return JUCE_UTF8("SMOOTH: extra envelope detector smoothing\nControls smoothing amount in detection\nRange: 0 (RAW) to 1 (SMOOTH) | Default: 0");
	    if (key == "flip") return JUCE_UTF8("FLIP: swaps TRANS and SUST values\nUseful for quickly testing inverse effect");
	}
    return "";
}


//==============================================================================
// HELPERS DE UI
//==============================================================================
void JCBTransientAudioProcessorEditor::applyAlphaToMainControls(float alpha)
{
    // Main knobs
    leftTopKnobs.attackSlider.setAlpha(alpha);
    leftTopKnobs.sustainSlider.setAlpha(alpha);
    leftTopKnobs.sensSlider.setAlpha(alpha);
    
    leftBottomKnobs.drywetSlider.setAlpha(alpha);
    leftBottomKnobs.lookaheadSlider.setAlpha(alpha);
    leftBottomKnobs.clipSlider.setAlpha(alpha);
    
    rightTopControls.dmodeButton.setAlpha(alpha);
    // REACT ELIMINADO
    rightTopControls.smoothSlider.setAlpha(alpha);
    
    rightBottomKnobs.atkSlider.setAlpha(alpha);
    rightBottomKnobs.relSlider.setAlpha(alpha);
    rightBottomKnobs.holdSlider.setAlpha(alpha);
    // speedButton removed
    
    // Transfer display
    transferDisplay.setAlpha(alpha);
    
    // Parameter buttons (not including BYPASS/DELTA which have special handling)
    parameterButtons.deltaButton.setAlpha(alpha);
    
    // Trim and makeup sliders
    trimSlider.setAlpha(alpha);
    makeupSlider.setAlpha(alpha);
}


// Nota: La funcionalidad de visualización de código ahora se maneja por CodeWindow desde DIAGRAM



//==============================================================================
// THREAD SAFETY Y AUTOMATIZACIÓN
//==============================================================================
void JCBTransientAudioProcessorEditor::queueParameterUpdate(const juce::String& paramID, float normalizedValue)
{
    {
        std::lock_guard<std::mutex> lock(parameterUpdateMutex);
        
        // Check if this parameter is already queued
        auto it = std::find_if(pendingParameterUpdates.begin(), pendingParameterUpdates.end(),
                               [&paramID](const DeferredParameterUpdate& update) {
                                   return update.paramID == paramID;
                               });
        
        if (it != pendingParameterUpdates.end()) {
            // Actualizar entrada existente
            it->normalizedValue = normalizedValue;
        } else {
            // Add new entry
            pendingParameterUpdates.push_back({paramID, normalizedValue});
        }
    }
    
    hasPendingParameterUpdates.store(true);
}

void JCBTransientAudioProcessorEditor::processPendingParameterUpdates()
{
    if (!hasPendingParameterUpdates.exchange(false)) {
        return;
    }
    
    std::vector<DeferredParameterUpdate> updates;
    {
        std::lock_guard<std::mutex> lock(parameterUpdateMutex);
        updates = std::move(pendingParameterUpdates);
        pendingParameterUpdates.clear();
    }
    
    // Establecer flag para prevenir transacciones de undo durante procesamiento de queue
    isProcessingQueue = true;
    
    // Process all updates on the message thread
    for (const auto& update : updates) {
        if (auto* param = processor.apvts.getParameter(update.paramID)) {
            // Usar beginChangeGesture/endChangeGesture para prevenir transacciones de undo
            param->beginChangeGesture();
            param->setValueNotifyingHost(update.normalizedValue);
            param->endChangeGesture();
            
            // CRÍTICO: Sincronización directa con Gen~ DSP
            // Esto asegura que los valores del DEFAULT button lleguen al DSP
            // Convertir valor normalizado a valor real usando el rango del parámetro
            auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param);
            if (floatParam) {
                float realValue = floatParam->getNormalisableRange().convertFrom0to1(update.normalizedValue);
                
                if (update.paramID == "d_ATK" || update.paramID == "e_REL") {
                    // Debug info for DEFAULT preset parameter updates
                }
                
                // Llamar al método parameterChanged del processor para sincronizar con Gen~
                processor.parameterChanged(update.paramID, realValue);
            }
            
            // Manejo de AudioParameterBool para sincronización con Gen~
            auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(param);
            if (boolParam) {
                float realValue = update.normalizedValue >= 0.5f ? 1.0f : 0.0f;

                
                // Llamar al método parameterChanged del processor para sincronizar con Gen~
                processor.parameterChanged(update.paramID, realValue);
            }
        }
    }
    
    // Visual feedback (cambios de alpha) se maneja en timerCallback() para updates consistentes
    
    // Limpiar flag después del procesamiento
    isProcessingQueue = false;
}

//==============================================================================
// DIAGRAM Y CODE WINDOW
//==============================================================================

void JCBTransientAudioProcessorEditor::showDiagram()
{
    // Thread-safe: usar MessageManager::callAsync para UI pesadas
    juce::Component::SafePointer<JCBTransientAudioProcessorEditor> safeThis(this);
    
    juce::MessageManager::callAsync([safeThis]() {
        if (!safeThis) return;
        
        // Crear overlay si no existe
        if (!safeThis->diagramOverlay)
        {
            safeThis->diagramOverlay = std::make_unique<DiagramOverlay>(*safeThis);
        }
        
        // Configurar y mostrar
        safeThis->addChildComponent(safeThis->diagramOverlay.get());
        safeThis->diagramOverlay->setBounds(safeThis->getLocalBounds());
        safeThis->diagramOverlay->setVisible(true);
        safeThis->diagramOverlay->toFront(true);
        safeThis->centerButtons.diagramButton.setToggleState(true, juce::dontSendNotification);
    });
}

void JCBTransientAudioProcessorEditor::hideDiagram()
{
    if (diagramOverlay && diagramOverlay->isVisible())
    {
        diagramOverlay->setVisible(false);
        removeChildComponent(diagramOverlay.get());
    }
    centerButtons.diagramButton.setToggleState(false, juce::dontSendNotification);
}

void JCBTransientAudioProcessorEditor::hideCodeWindow()
{
    if (codeWindow && codeWindow->isVisible())
    {
        codeWindow->setVisible(false);
        removeChildComponent(codeWindow.get());
    }
}

juce::String JCBTransientAudioProcessorEditor::loadCodeFromFile(const juce::String& blockName)
{
    // Thread-safe: usar cache pre-cargado en lugar de leer BinaryData cada vez
    if (codeContentCacheInitialized && codeContentCache.find(blockName) != codeContentCache.end()) {
        return codeContentCache[blockName];
    }
    
    // Fallback si el cache no está inicializado o no se encuentra el bloque
    return "// Code for " + blockName + "\n\n// Content not found in cache.\n// Please report this issue.\n\n// Basic functionality:\n" + getBasicBlockDescription(blockName);
}

void JCBTransientAudioProcessorEditor::initializeCodeContentCache()
{
    if (codeContentCacheInitialized) return;
    
    // Pre-cargar todo el contenido de código al inicializar para thread safety
    struct CodeMapping {
        juce::String blockName;
        const char* binaryData;
        int dataSize;
    };
    
    // Lista de todos los mappings con bloques separados corregidos
    std::vector<CodeMapping> mappings = {
        {"TRIM IN", BinaryData::InputTrim_txt, BinaryData::InputTrim_txtSize},
        {"TRIM SC", BinaryData::InputTrim_txt, BinaryData::InputTrim_txtSize},
        {"LOOKAHEAD", BinaryData::InputTrim_txt, BinaryData::InputTrim_txtSize},
        {"FILTERS", BinaryData::Filters_txt, BinaryData::Filters_txtSize},
        {"DETECTOR+GAIN CALC", BinaryData::DetectorGainCalc_txt, BinaryData::DetectorGainCalc_txtSize},
        {"GAIN CORE", BinaryData::GainCore_txt, BinaryData::GainCore_txtSize},
        {"MAKEUP", BinaryData::Output_txt, BinaryData::Output_txtSize},
        {"PARALLEL", BinaryData::Output_txt, BinaryData::Output_txtSize},
        {"OUTPUT", BinaryData::Output_txt, BinaryData::Output_txtSize},
        {"DELTA", BinaryData::Output_txt, BinaryData::Output_txtSize}
    };
    
    // Cargar todo en cache
    for (const auto& mapping : mappings) {
        if (mapping.binaryData != nullptr && mapping.dataSize > 0) {
            juce::String content = juce::String::createStringFromData(mapping.binaryData, mapping.dataSize);
            if (content.isNotEmpty()) {
                codeContentCache[mapping.blockName] = content;
            } else {
                // Fallback si falla la carga
                codeContentCache[mapping.blockName] = "// Code for " + mapping.blockName + 
                    "\n\n// Error cargando contenido\n\n// Funcionalidad básica:\n" + 
                    getBasicBlockDescription(mapping.blockName);
            }
        }
    }
    
    codeContentCacheInitialized = true;
}

juce::String JCBTransientAudioProcessorEditor::getBasicBlockDescription(const juce::String& blockName)
{
    if (blockName == "TRIM IN") {
        return "// Input trim gain applied to main signal\ninput_trimmed = input * dbtoa(trim_db);";
    } else if (blockName == "TRIM SC") {
        return "// Sidechain trim gain applied to sidechain signal\nsc_trimmed = sidechain * dbtoa(sc_trim_db);";
    } else if (blockName == "FILTERS") {
        return "// HPF and LPF filters for sidechain processing\nfiltered_sc = lpf(hpf(sc_signal, hpf_freq), lpf_freq);";
    } else if (blockName == "DETECTOR") {
        return "// Level detector with react control\nlevel = detector(filtered_sc, react_param);";
    } else if (blockName == "GAIN CALC") {
        return "// Compressor gain calculation with ratio and knee\ngain_reduction = compressor_curve(level, threshold, ratio, knee);";
    } else if (blockName == "APPLY") {
        return "// Apply gain reduction to main signal\ncompressed = input_trimmed * gain_reduction;";
    } else if (blockName == "MAKEUP") {
        return "// Apply makeup gain to compressed signal\nwith_makeup = compressed * dbtoa(makeup_db);";
    } else if (blockName == "PARALLEL") {
        return "// Parallel compression mixing\noutput = mix(input_trimmed, with_makeup, parallel_amount);";
    } else if (blockName == "OUTPUT") {
        return "// Final output with any additional processing\nfinal_output = output_stage(parallel_output);";
    } else {
        return "// Generic Gen~ block processing\n// See Max patch for detailed implementation";
    }
}

// Función helper para obtener índice de parámetro por ID (robusta, compatible con el futuro)
int JCBTransientAudioProcessorEditor::getParameterIndexByID(const juce::String& parameterID)
{
    auto& params = processor.getParameters();
    for (int i = 0; i < params.size(); ++i) {
        if (auto* param = dynamic_cast<juce::AudioProcessorParameterWithID*>(params[i])) {
            if (param->paramID == parameterID) {
                return i;
            }
        }
    }
    return -1; // Parámetro no encontrado
}

int JCBTransientAudioProcessorEditor::getControlParameterIndex(juce::Component& control)
{
    // Mapear componentes UI a sus IDs de parámetro (robusta, compatible con el futuro)
    // Retornar -1 para componentes que no representan parámetros automatizables
    
    juce::String parameterID;
    
    // Perillas Superiores Izquierdas (threshold, ratio, knee)
    if (&control == &leftTopKnobs.attackSlider) parameterID = "b_THD";
    else if (&control == &leftTopKnobs.sustainSlider) parameterID = "c_RATIO";
    else if (&control == &leftTopKnobs.sensSlider) parameterID = "q_KNEE";
    
    // Perillas Inferiores Izquierdas (drywet, lookahead, clip, autogain)
    else if (&control == &leftBottomKnobs.drywetSlider) parameterID = "o_DRYWET";
    else if (&control == &leftBottomKnobs.lookaheadSlider) parameterID = "n_LOOKAHEAD";
    else if (&control == &leftBottomKnobs.clipSlider) parameterID = "u_SOFTCLIP";  // Corregido: ahora usa mapeo dinámico
    
    // Controles Superiores Derechos (range, react, smooth)
    else if (&control == &rightTopControls.dmodeButton) parameterID = "h_DELTAMODE";
    // REACT ELIMINADO
    else if (&control == &rightTopControls.smoothSlider) parameterID = "z_SMOOTH";
    
    // Perillas Inferiores Derechas (attack, release, hold)
    else if (&control == &rightBottomKnobs.atkSlider) parameterID = "d_ATK";
    else if (&control == &rightBottomKnobs.relSlider) parameterID = "e_REL";
    else if (&control == &rightBottomKnobs.holdSlider) parameterID = "f_HOLD";
    // speedButton removido - parámetro t_AUTORELEASESPEED ya no existe
    
    // Controles de Sidechain (filters, key)
    else if (&control == &sidechainControls.hpfSlider) parameterID = "j_HPF";
    else if (&control == &sidechainControls.lpfSlider) parameterID = "k_LPF";
    else if (&control == &sidechainControls.keyButton) parameterID = "r_KEY";
    
    // Sliders de Trim
    else if (&control == &trimSlider) parameterID = "a_TRIM";
    else if (&control == &makeupSlider) parameterID = "i_MAKEUP";
    else if (&control == &scTrimSlider) parameterID = "y_SCTRIM";
    
    // Botones Automatizables
    else if (&control == &sidechainControls.scButton) parameterID = "l_SC";
    
    // Parámetros no automatizables (retornar -1)
    // Estos son parámetros globales/utility que no deberían mostrar carriles de automatización
    else if (&control == &sidechainControls.soloScButton) return -1;  // m_SOLOSC (no automatizable)
    else if (&control == &parameterButtons.deltaButton) return getParameterIndexByID("v_DELTA");  // v_DELTA automatizable
    else if (&control == &parameterButtons.bypassButton) return -1;     // p_BYPASS (no automatizable)
    
    // Obtener índice dinámico de parámetro
    if (parameterID.isNotEmpty()) {
        return getParameterIndexByID(parameterID);
    }
    
    // Cualquier otro componente que no representa un parámetro
    return -1;
}

void JCBTransientAudioProcessorEditor::applyMeterDecayIfNeeded()
{
    // Sistema universal de decay para todos los DAWs
    // Basado en la solución recomendada en los foros de JUCE
    // Implementación profesional: decay siempre activo
    
    // Los medidores decaen naturalmente por inactividad
    // Los repaints se manejan automáticamente por el timer normal del editor
}