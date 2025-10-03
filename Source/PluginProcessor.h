//==============================================================================
//
//  Copyright 2025 Juan Carlos Blancas
//  This file is part of JCBTransient and is licensed under the GNU General Public License v3.0 or later.
//
//==============================================================================
#pragma once

//==============================================================================
// INCLUDES
//==============================================================================
// Módulos JUCE
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>

// Librerías estándar C++
#include <mutex>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <cmath>

// Archivos del proyecto
#include "JCBTransient.h"
#include "Helpers/MovingAverage4800.h"

using namespace juce;

//==============================================================================
// CLASE PRINCIPAL DEL PROCESADOR
//==============================================================================
class JCBTransientAudioProcessor : public juce::AudioProcessor,
                                    public juce::AudioProcessorValueTreeState::Listener,
                                    private juce::Timer,
                                    private juce::AsyncUpdater
{
public:
    //==============================================================================
    // Constructor y destructor
    JCBTransientAudioProcessor();
    ~JCBTransientAudioProcessor() override;
    
    //==============================================================================
    // Métodos principales del AudioProcessor
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
    bool isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    //==============================================================================
    // Gestión del editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const noexcept override { return true; }
    
    //==============================================================================
    // Propiedades básicas del plugin
    const juce::String getName() const noexcept override { return JucePlugin_Name; }
    
    bool acceptsMidi() const noexcept override { return false; }
    bool producesMidi() const noexcept override { return false; }
    bool silenceInProducesSilenceOut() const noexcept override { return false; }
    double getTailLengthSeconds() const noexcept override { return 5.0; }
    
    //==============================================================================
    // Gestión de presets/programs
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;
    
    //==============================================================================
    // Serialización del estado
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    //==============================================================================
    // Reportes de reducción de ganancia específicos por formato
    #if JucePlugin_Build_AAX
    float getAAXMeterValue(int meterID);
    #endif
    
    #if JucePlugin_Build_VST3
    void updateVST3GainReduction();
    #endif
    
    #if JucePlugin_Build_AU
    // AU: reducción de ganancia expuesta a través del wrapper
    #endif
    
    //==============================================================================
    // APVTS y gestión de parámetros
    juce::AudioProcessorValueTreeState apvts;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    
    //==============================================================================
    // Gestión de estado de la interfaz
    juce::Point<int> getSavedSize() const noexcept { return editorSize; }
    void setSavedSize(const juce::Point<int>& size) noexcept { editorSize = size; }
    
    int getLastPreset() const noexcept { return lastPreset; }
    void setLastPreset(const int& presetID) noexcept { lastPreset = presetID; }
    
    // Estado del menú de presets
    juce::String getPresetDisplayText() const noexcept { return presetDisplayText; }
    void setPresetDisplayText(const juce::String& text) noexcept { presetDisplayText = text; }
    bool getPresetTextItalic() const noexcept { return presetTextItalic; }
    void setPresetTextItalic(bool italic) noexcept { presetTextItalic = italic; }
    
    // Detección de formato del DAW
    bool isProTools() const noexcept;
    juce::String getPluginFormat() const noexcept;
    
    // Estado de tooltips e idioma
    bool getTooltipEnabled() const noexcept { return tooltipEnabled; }
    void setTooltipEnabled(bool enabled) noexcept { tooltipEnabled = enabled; }
    bool getEnvelopeVisualEnabled() const noexcept { return envelopeVisualEnabled; }
    void setEnvelopeVisualEnabled(bool enabled) noexcept { envelopeVisualEnabled = enabled; }
    bool getTooltipLanguageEnglish() const noexcept { return tooltipLanguageEnglish; }
    void setTooltipLanguageEnglish(bool english) noexcept { tooltipLanguageEnglish = english; }
    
    // Métodos de comparación A/B
    void saveCurrentStateToActive();
    void toggleAB();
    void copyAtoB();
    void copyBtoA();
    bool getIsStateA() const noexcept { return isStateA; }
    
    // Acceso al UndoManager de GUI
    juce::UndoManager& getGuiUndoManager() noexcept { return guiUndoManager; }
    
    //==============================================================================
    // Medidores de audio
    float getRmsInputValue(const int channel) const noexcept;
    float getRmsOutputValue(const int channel) const noexcept;
    float getSCValue(const int channel) const noexcept;
    
    // Detección de clipping
    bool getInputClipDetected(const int channel) const noexcept;
    bool getOutputClipDetected(const int channel) const noexcept;
    bool getSidechainClipDetected(const int channel) const noexcept;
    void resetClipIndicators();

    // Datos de forma de onda
    void getWaveformData(std::vector<float>& inputSamples, std::vector<float>& processedSamples) const;
    void getAttackSustainData(std::vector<float>& attackSamples, std::vector<float>& sustainSamples) const;
    
    // Sistema híbrido: timestamp + playhead para detección más robusta
    bool isPlaybackActive() const noexcept;
    
    //==============================================================================
    // Métodos de parámetros legacy
    int getNumParameters() override;
    float getParameter(int index) override;
    void setParameter(int index, float newValue) override;
    const juce::String getParameterName(int index) override;
    const juce::String getParameterText(int index) override;
    
    //==============================================================================
    // Métodos de canales
    const juce::String getInputChannelName(int channelIndex) const override;
    const juce::String getOutputChannelName(int channelIndex) const override;
    bool isInputChannelStereoPair(int index) const override;
    bool isOutputChannelStereoPair(int index) const override;
    
    // Acceso al estado de Gen~ (para sincronización directa)
    CommonState* getPluginState() const { return m_PluginState; }
    
    // Estado del botón graphics para persistencia en presets
    bool displayGraphicsEnvelopes{true};
    
private:
    //==============================================================================
    // UndoManager separado para GUI
    juce::UndoManager guiUndoManager;
    
    //==============================================================================
    // Integración Gen~
    void assureBufferSize(long bufferSize);
    void fillGenInputBuffers(const juce::AudioBuffer<float>& buffer);
    void processGenAudio(int numSamples);
    void fillOutputBuffers(juce::AudioBuffer<float>& buffer);

    // Safety: sanitizar buffers de salida antes de entregar al host
    inline void sanitizeStereo(float* left, float* right, int numSamples, std::atomic<bool>& tripped) noexcept
    {
        bool localTrip = false;
        for (int i = 0; i < numSamples; ++i)
        {
            float sampleL = left[i];
            float sampleR = right ? right[i] : sampleL;

            if (!std::isfinite(sampleL) || sampleL > 8.0f || sampleL < -8.0f) { sampleL = 0.0f; localTrip = true; }
            if (!std::isfinite(sampleR) || sampleR > 8.0f || sampleR < -8.0f) { sampleR = 0.0f; localTrip = true; }

            left[i] = sampleL;
            if (right) right[i] = sampleR;
        }

        if (localTrip)
            tripped.store(true, std::memory_order_release);
    }
    
    // Actualizaciones de medidores
    void updateInputMeters(const juce::AudioBuffer<float>& buffer);
    void updateOutputMeters(const juce::AudioBuffer<float>& buffer);
    void updateSidechainMeters(const juce::AudioBuffer<float>& buffer);
    void updateAttackSustainGains(int numSamples);
    void captureInputWaveformData(const juce::AudioBuffer<float>& inputBuffer, int numSamples);
    void captureOutputWaveformData(int numSamples);
    
    // Detección de clipping
    void updateClipDetection(const juce::AudioBuffer<float>& inputBuffer, const juce::AudioBuffer<float>& outputBuffer);
    
    // Gestión de layout y buses
    juce::AudioProcessor::BusesProperties createBusesProperties();
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    //==============================================================================
    // Estado Gen~
    CommonState* m_PluginState;
    long m_CurrentBufferSize;
    t_sample** m_InputBuffers;
    t_sample** m_OutputBuffers;

    std::atomic<bool> nanTripped{false};
    
    // Valores de medidores thread-safe
    std::atomic<float> leftInputRMS{-100.0f};
    std::atomic<float> rightInputRMS{-100.0f};
    std::atomic<float> leftOutputRMS{-100.0f};
    std::atomic<float> rightOutputRMS{-100.0f};
    
    std::atomic<float> leftSC{-100.0f};
    std::atomic<float> rightSC{-100.0f};
    
    // Buffers auxiliares
    juce::AudioBuffer<float> grBuffer;
    juce::AudioBuffer<float> sidechainBuffer;
    juce::AudioBuffer<float> trimInputBuffer;
    
    // Buffers para forma de onda
    mutable std::vector<float> currentInputSamples;
    mutable std::vector<float> currentProcessedSamples;
    mutable std::mutex waveformMutex;
    
    // Buffers para Attack/Sustain gain histograms
    mutable std::vector<float> currentAttackGainSamples;
    mutable std::vector<float> currentSustainGainSamples;
    
    // Gestión de estado
    juce::Point<int> editorSize{1250, 350};
    int lastPreset{0};
    int currentProgram{-1};
    
    // Estado del menú de presets
    juce::String presetDisplayText{"DEFAULT"};
    bool presetTextItalic{false};
    
    bool tooltipEnabled{true};
    bool envelopeVisualEnabled{true};
    bool tooltipLanguageEnglish{false};
    
    // Estado de detección de clipping
    std::atomic<bool> inputClipDetected[2] = {false, false};
    std::atomic<bool> outputClipDetected[2] = {false, false};
    std::atomic<bool> sidechainClipDetected[2] = {false, false};

    // Flag para indicar destrucción del plugin
    std::atomic<bool> isBeingDestroyed{false};
    
    //==============================================================================
    // Timer callback para actualizaciones fuera del audio thread
    void timerCallback() override;
    
    // Sistema de comparación A/B
    struct ParameterState {
        std::unordered_map<juce::String, float> values;
        
        void captureFrom(const juce::AudioProcessorValueTreeState& apvts) {
            values.clear();
            auto& params = apvts.processor.getParameters();
            for (auto* param : params) {
                if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*>(param)) {
                    values[p->paramID] = p->getValue();
                }
            }
        }
        
        void applyTo(juce::AudioProcessorValueTreeState& apvts) const {
            for (const auto& [id, value] : values) {
                if (auto* param = apvts.getParameter(id)) {
                    param->setValueNotifyingHost(value);
                }
            }
        }
    };
    
    ParameterState stateA;
    ParameterState stateB;
    bool isStateA{true};

    // ==============================================================================
    // SISTEMA DE BYPASS COMPENSADO
    // ==============================================================================
    
    // --- Ring Buffer para compensación de latencia ---
    juce::AudioBuffer<float> bypassDelayBuffer;  // Ring buffer circular para DRY compensado
    int  bypassDelayWritePos { 0 };              // Posición de escritura actual en el ring
    juce::SpinLock bypassDelayLock;              // Lock para thread-safety del ring buffer
    int  bypassDelayCapacity { 0 };              // Capacidad fija preasignada (máximo lookahead)
    int  dryPrimedSamples   { 0 };               // Contador de muestras válidas en el ring

    // --- Scratch Buffers RT-safe ---
    juce::AudioBuffer<float> scratchIn;          // Buffer temporal para entrada (2ch: L/R)
    juce::AudioBuffer<float> scratchDry;         // Buffer temporal para DRY compensado (2ch: L/R)
    int scratchCapacitySamples { 0 };            // Capacidad actual de los scratch buffers

    // Helper: asegura capacidad de scratch sin allocations en audio thread
    inline void ensureScratchCapacity (int numSamples)
    {
        if (numSamples > scratchCapacitySamples)
        {
            scratchIn.setSize (2, numSamples, false, false, true);
            scratchDry.setSize(2, numSamples, false, false, true);
            scratchIn.clear();
            scratchDry.clear();
            scratchCapacitySamples = numSamples;
        }
    }

    // --- FSM de Bypass con Fade ---
    enum class BypassState { Active, FadingToBypass, Bypassed, FadingToActive };
    BypassState bypassState { BypassState::Active };  // Estado actual del bypass
    int  bypassFadeLen { 384 };                       // Longitud del fade en samples (calculado de bypassFadeMs)
    float bypassFadeMs { 7.0f };                      // Duración del fade en ms (7ms por defecto)
    int  bypassFadePos { 0 };                         // Posición actual del fade (0 a bypassFadeLen)

    // --- Control y sincronización ---
    std::atomic<bool> hostBypassMirror { false };     // Espejo atómico del estado de bypass del host
    bool lastWantsBypass { false };                   // Estado anterior para detección de flancos

    // LATENCIA SEGURA (message thread)
    std::atomic<int> pendingLatency { -1 };
    int currentLatency = 0;

    // LOOKAHEAD DEBOUNCE
    std::atomic<float>    stagedLookaheadMs { 0.0f };
    std::atomic<uint32_t> lastLAChangeMs   { 0 };
    std::atomic<bool>     laCommitPending  { false };
    int                   laDebounceMs     { 140 };

    // Offset intrínseco de Gen~ (0 ya que Gen~ está bien configurado)
    std::atomic<int> intrinsicGenOffset { 0 };

    // Helper inline: calcula latencia en muestras
    int computeLatencySamples (double sr) const
    {
        if (sr <= 0.0) return 0;
        float latMs = apvts.getRawParameterValue("n_LOOKAHEAD")->load();
        if (! std::isfinite(latMs) || latMs < 0.0f) latMs = 0.0f;
        const int base = juce::roundToInt (latMs * sr / 1000.0f);
        return juce::jmax (0, base + intrinsicGenOffset.load(std::memory_order_relaxed));
    }

    // Helper para leer el parámetro de bypass del host
     inline bool isHostBypassed() const noexcept
     {
         if (auto* p = getBypassParameter())
             return p->getValue() >= 0.5f;
         return false;
     }

    // Override de AsyncUpdater
    void handleAsyncUpdate() override;

    // Cachear índices de gen
    int genIdxLookahead { -1 };
    int genIdxBypass    { -1 };


    // DC-block para fades (opcional)
    float dc_x1L = 0.0f, dc_y1L = 0.0f, dc_x1R = 0.0f, dc_y1R = 0.0f;
    float dc_R   = 0.0f; // exp(-2*pi*fc/fs)
    inline float dcBlock (float x, float& x1, float& y1) noexcept
    {
        const float y = x - x1 + dc_R * y1;
        x1 = x; y1 = y;
        return y;
    }

    // Punto común de proceso
    void processBlockCommon(juce::AudioBuffer<float>& buffer, bool hostWantsBypass);


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JCBTransientAudioProcessor)
};
