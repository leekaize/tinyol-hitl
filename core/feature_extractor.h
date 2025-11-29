/**
 * @file feature_extractor.h
 * @brief Vibration + current feature extraction with Gravity Compensation
 * 
 * Feature schemas (configurable in config.h):
 *   SCHEMA_TIME_ONLY:     [rms, peak, crest]                    3D
 *   SCHEMA_TIME_CURRENT:  [rms, peak, crest, i1, i2, i3, i_rms] 7D
 *   SCHEMA_FFT_ONLY:      [rms, peak, crest, fft_peak_freq,     6D
 *                          fft_peak_amp, spectral_centroid]
 *   SCHEMA_FFT_CURRENT:   [above + i1, i2, i3, i_rms]           10D
 * 
 * GRAVITY COMPENSATION:
 *   Accelerometers read ~9.8 m/s² at rest. This file includes a 
 *   High-pass filter (VibrationFilter) to extract the AC component 
 *   (actual vibration) before calculating features.
 */

#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include <Arduino.h>
#include <math.h>
#include "config.h"

// =============================================================================
// FEATURE SCHEMA SELECTION
// =============================================================================

// Choose ONE schema in config.h:
//   #define FEATURE_SCHEMA_TIME_ONLY
//   #define FEATURE_SCHEMA_TIME_CURRENT
//   #define FEATURE_SCHEMA_FFT_ONLY
//   #define FEATURE_SCHEMA_FFT_CURRENT

#if defined(FEATURE_SCHEMA_FFT_CURRENT)
  #define FEATURE_DIM 10
  #define USE_FFT
  #define USE_CURRENT
#elif defined(FEATURE_SCHEMA_FFT_ONLY)
  #define FEATURE_DIM 6
  #define USE_FFT
#elif defined(FEATURE_SCHEMA_TIME_CURRENT)
  #define FEATURE_DIM 7
  #define USE_CURRENT
#else
  // Default: time-domain only
  #define FEATURE_SCHEMA_TIME_ONLY
  #define FEATURE_DIM 3
#endif

// FFT configuration
#ifdef USE_FFT
  #define FFT_SAMPLES 64          // Power of 2, fits in RAM
  #define FFT_SAMPLE_FREQ 1000.0  // Hz (actual accelerometer rate)
#endif

// =============================================================================
// GRAVITY-COMPENSATED VIBRATION EXTRACTOR
// =============================================================================

class VibrationFilter {
private:
  // Exponential moving average for baseline (gravity)
  float baselineX = 0, baselineY = 0, baselineZ = 0;
  float alpha = 0.1f;  // Low alpha = slow adaptation = good gravity tracking
  bool initialized = false;
  
  // Ring buffer for RMS calculation
  static const int WINDOW = 10;  // 1 second @ 10Hz
  float acBuffer[WINDOW];
  int bufIdx = 0;
  int bufCount = 0;

public:
  /**
   * Update baseline and compute AC vibration magnitude
   * @return AC magnitude (vibration without gravity)
   */
  float update(float ax, float ay, float az) {
    if (!initialized) {
      baselineX = ax;
      baselineY = ay;
      baselineZ = az;
      initialized = true;
      return 0;
    }
    
    // Update baseline (tracks gravity slowly)
    baselineX = alpha * ax + (1 - alpha) * baselineX;
    baselineY = alpha * ay + (1 - alpha) * baselineY;
    baselineZ = alpha * az + (1 - alpha) * baselineZ;
    
    // AC component = raw - baseline
    float acX = ax - baselineX;
    float acY = ay - baselineY;
    float acZ = az - baselineZ;
    
    // Magnitude of AC (actual vibration)
    float acMag = sqrtf(acX*acX + acY*acY + acZ*acZ);
    
    // Add to ring buffer
    acBuffer[bufIdx] = acMag;
    bufIdx = (bufIdx + 1) % WINDOW;
    if (bufCount < WINDOW) bufCount++;
    
    return acMag;
  }
  
  /**
   * Get RMS of AC vibration over window
   */
  float getRMS() {
    if (bufCount == 0) return 0;
    float sum = 0;
    for (int i = 0; i < bufCount; i++) {
      sum += acBuffer[i] * acBuffer[i];
    }
    return sqrtf(sum / bufCount);
  }
  
  /**
   * Get peak of AC vibration over window
   */
  float getPeak() {
    float peak = 0;
    for (int i = 0; i < bufCount; i++) {
      if (acBuffer[i] > peak) peak = acBuffer[i];
    }
    return peak;
  }
  
  /**
   * Get baseline magnitude (should be ~9.8 m/s² = gravity)
   */
  float getBaseline() {
    return sqrtf(baselineX*baselineX + baselineY*baselineY + baselineZ*baselineZ);
  }
  
  void reset() {
    initialized = false;
    bufCount = 0;
    bufIdx = 0;
  }
};

// Global instance used by FeatureExtractor
static VibrationFilter vibFilter;

// =============================================================================
// FEATURE EXTRACTOR
// =============================================================================

class FeatureExtractor {
public:
  /**
   * Extract time-domain features with Gravity Compensation
   * @param ax, ay, az Acceleration (m/s²)
   * @param features Output: [rms, peak, crest]
   */
  static void extractTime(float ax, float ay, float az, float* features) {
    // Update filter and get AC vibration (gravity removed)
    vibFilter.update(ax, ay, az);
    
    float rms = vibFilter.getRMS();
    float peak = vibFilter.getPeak();
    // Crest Factor = peak/RMS (high = impulsive)
    float crest = (rms > 0.01f) ? (peak / rms) : 1.0f;
    
    features[0] = rms;
    features[1] = peak;
    features[2] = crest;
  }

#ifdef USE_FFT
  /**
   * Extract FFT features from magnitude signal buffer
   */
  static void extractFFT(const float* mag_buffer, float* features) {
    // Placeholder values
    features[0] = 0.0f;  // fft_peak_freq (Hz)
    features[1] = 0.0f;  // fft_peak_amp
    features[2] = 0.0f;  // spectral_centroid (Hz)
  }
#endif

  /**
   * Full feature extraction based on configured schema
   */
  static void extract(float ax, float ay, float az,
                      float i1, float i2, float i3,
                      const float* fft_buffer,  // NULL if not using FFT
                      float* features) {
    
    // Time-domain (always first 3)
    extractTime(ax, ay, az, features);
    
    int idx = 3;
    
    #ifdef USE_FFT
      if (fft_buffer != NULL) {
        extractFFT(fft_buffer, &features[idx]);
        idx += 3;
      } else {
        features[idx++] = 0.0f;
        features[idx++] = 0.0f;
        features[idx++] = 0.0f;
      }
    #endif
    
    #ifdef USE_CURRENT
      features[idx++] = i1;
      features[idx++] = i2;
      features[idx++] = i3;
      features[idx++] = sqrtf((i1*i1 + i2*i2 + i3*i3) / 3.0f);  // i_rms
    #endif
  }

  /**
   * Simplified extraction for time-only or time+current
   */
  static void extractSimple(float ax, float ay, float az,
                            float i1, float i2, float i3,
                            float* features) {
    extractTime(ax, ay, az, features);
    
    #ifdef USE_CURRENT
      features[3] = i1;
      features[4] = i2;
      features[5] = i3;
      features[6] = sqrtf((i1*i1 + i2*i2 + i3*i3) / 3.0f);
    #endif
  }

  static constexpr uint8_t getFeatureDim() {
    return FEATURE_DIM;
  }

  // Debug: get raw baseline from the filter
  static float getBaseline() { return vibFilter.getBaseline(); }

  static void getFeatureNames(char names[][32]) {
    strcpy(names[0], "vib_rms");
    strcpy(names[1], "vib_peak");
    strcpy(names[2], "vib_crest");
    
    int idx = 3;
    
    #ifdef USE_FFT
      strcpy(names[idx++], "fft_peak_freq");
      strcpy(names[idx++], "fft_peak_amp");
      strcpy(names[idx++], "spectral_centroid");
    #endif
    
    #ifdef USE_CURRENT
      strcpy(names[idx++], "current_l1");
      strcpy(names[idx++], "current_l2");
      strcpy(names[idx++], "current_l3");
      strcpy(names[idx++], "current_rms");
    #endif
  }
};

// =============================================================================
// ZMCT103C CURRENT SENSOR (IMPROVED ZEROING)
// =============================================================================

#ifdef USE_CURRENT

class CurrentSensor {
private:
  static constexpr int SAMPLES = 1000;
  static constexpr float V_REF = 3.3f;
  static constexpr int ADC_MAX = 4095;
  // Increased noise floor and added absolute cutoff
  static constexpr float NOISE_FLOOR = 0.20f; // 200mA noise floor
  static constexpr float CUTOFF = 0.30f;      // Force 0 if < 300mA
  static constexpr int AVG_COUNT = 5;
  static constexpr float SENSITIVITY = 0.1f;

  float zeroOffset[3] = {0, 0, 0};
  float buffer[3][AVG_COUNT] = {{0}};
  int bufferIndex = 0;
  bool calibrated = false;

  float measureRawRMS(int pin) {
    float sum = 0;
    // Get DC bias
    for (int i = 0; i < SAMPLES; i++) sum += analogRead(pin);
    float midpoint = sum / SAMPLES;
    
    float sumSq = 0;
    for (int i = 0; i < SAMPLES; i++) {
      float v = analogRead(pin) - midpoint;
      sumSq += v * v;
    }
    float rmsAdc = sqrtf(sumSq / SAMPLES);
    return (rmsAdc * V_REF) / ADC_MAX;
  }

  float measureCurrent(int pin, float zeroOff) {
    float rawVolts = measureRawRMS(pin);
    
    // Safety gate: if raw voltage is very close to offset, ignore
    if (rawVolts < zeroOff * 1.05f) return 0.0f;

    float correctedVolts = rawVolts - zeroOff;
    float current = correctedVolts / SENSITIVITY;
    
    // Absolute cutoff to prevent "growing" small values
    if (current < CUTOFF) return 0.0f;
    
    return current;
  }

  float average(float* buf) {
    float sum = 0;
    for (int i = 0; i < AVG_COUNT; i++) sum += buf[i];
    return sum / AVG_COUNT;
  }

public:
  void calibrate() {
    Serial.println("[CT] Calibrating - motor OFF...");
    delay(2000);
    
    // Take multiple samples to get a stable zero
    float sum1=0, sum2=0, sum3=0;
    int n=10;
    for(int i=0; i<n; i++) {
      sum1 += measureRawRMS(ADC_CURRENT_L1);
      sum2 += measureRawRMS(ADC_CURRENT_L2);
      sum3 += measureRawRMS(ADC_CURRENT_L3);
      delay(50);
    }
    
    zeroOffset[0] = sum1/n;
    zeroOffset[1] = sum2/n;
    zeroOffset[2] = sum3/n;
    
    Serial.printf("[CT] Noise Floors (V): %.3f %.3f %.3f\n", zeroOffset[0], zeroOffset[1], zeroOffset[2]);
    calibrated = true;
    
    // Reset buffers
    for(int i=0; i<3; i++) for(int j=0; j<AVG_COUNT; j++) buffer[i][j] = 0.0f;
  }

  void read(float* i1, float* i2, float* i3) {
    if (!calibrated) { *i1 = *i2 = *i3 = 0; return; }
    
    float raw1 = measureCurrent(ADC_CURRENT_L1, zeroOffset[0]);
    float raw2 = measureCurrent(ADC_CURRENT_L2, zeroOffset[1]);
    float raw3 = measureCurrent(ADC_CURRENT_L3, zeroOffset[2]);
    
    buffer[0][bufferIndex] = raw1;
    buffer[1][bufferIndex] = raw2;
    buffer[2][bufferIndex] = raw3;
    bufferIndex = (bufferIndex + 1) % AVG_COUNT;
    
    *i1 = average(buffer[0]);
    *i2 = average(buffer[1]);
    *i3 = average(buffer[2]);
  }

  bool isMotorRunning(float i1, float i2, float i3) {
    return ((i1 + i2 + i3) / 3.0f) > CUTOFF;
  }
};

#endif // USE_CURRENT

#endif // FEATURE_EXTRACTOR_H