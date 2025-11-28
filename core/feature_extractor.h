/**
 * @file feature_extractor.h
 * @brief Time-domain feature extraction for vibration + current analysis
 *
 * Schema v1: [rms, peak, crest] (3D) - Vibration only
 * Schema v2: [rms, peak, crest, i1, i2, i3, i_rms] (7D) - Vibration + Current
 */

#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include <Arduino.h>
#include <math.h>
#include "config.h"

// Feature dimension based on sensor config
#ifdef SENSOR_CURRENT_ZMCT103C
  #define FEATURE_DIM 7  // 3 vibration + 4 current features
#else
  #define FEATURE_DIM 3  // Vibration only
#endif

class FeatureExtractor {
public:
  /**
   * Extract features from single 3-axis reading (instantaneous)
   * @param ax, ay, az Acceleration in m/s²
   * @param features Output array [rms, peak, crest]
   */
  static void extractVibration(float ax, float ay, float az, float* features) {
    float mag_x = abs(ax);
    float mag_y = abs(ay);
    float mag_z = abs(az);

    // RMS = sqrt((x² + y² + z²) / 3)
    float rms = sqrt((ax*ax + ay*ay + az*az) / 3.0f);

    // Peak = max absolute component
    float peak = max(mag_x, max(mag_y, mag_z));

    // Crest Factor = peak/RMS (high = impulsive, bearing faults)
    float crest = (rms > 0.01f) ? (peak / rms) : 1.0f;

    features[0] = rms;
    features[1] = peak;
    features[2] = crest;
  }

  /**
   * Extract combined vibration + current features
   * @param ax, ay, az Acceleration in m/s²
   * @param i1, i2, i3 Phase currents in Amps
   * @param features Output array [rms, peak, crest, i1, i2, i3, i_rms]
   */
  static void extractCombined(float ax, float ay, float az,
                              float i1, float i2, float i3,
                              float* features) {
    // Vibration features (first 3)
    extractVibration(ax, ay, az, features);

    // Current features (next 4)
    features[3] = i1;
    features[4] = i2;
    features[5] = i3;
    features[6] = sqrt((i1*i1 + i2*i2 + i3*i3) / 3.0f);  // 3-phase RMS
  }

  static constexpr uint8_t getFeatureDim() {
    return FEATURE_DIM;
  }

  static void getFeatureNames(char names[][32]) {
    strcpy(names[0], "vib_rms");
    strcpy(names[1], "vib_peak");
    strcpy(names[2], "vib_crest");
    #ifdef SENSOR_CURRENT_ZMCT103C
    strcpy(names[3], "current_l1");
    strcpy(names[4], "current_l2");
    strcpy(names[5], "current_l3");
    strcpy(names[6], "current_rms");
    #endif
  }
};

// =============================================================================
// ZMCT103C Current Sensing (3-phase)
// =============================================================================

#ifdef SENSOR_CURRENT_ZMCT103C

class CurrentSensor {
private:
  static constexpr int SAMPLES = 2000;
  static constexpr float V_REF = 3.3f;
  static constexpr int ADC_MAX = 4095;
  static constexpr float NOISE_FLOOR = 0.05f;
  static constexpr int AVG_COUNT = 5;
  static constexpr float SENSITIVITY = 0.1f;  // Adjust with calibration

  float zeroOffset[3] = {0, 0, 0};
  float buffer[3][AVG_COUNT] = {{0}};
  int bufferIndex = 0;
  bool calibrated = false;

  float measureRawRMS(int pin) {
    float sum = 0;
    float sumSq = 0;

    // First pass: compute mean
    for (int i = 0; i < SAMPLES; i++) {
      int raw = analogRead(pin);
      float v = (raw * V_REF) / ADC_MAX;
      sum += v;
    }
    float midpoint = sum / SAMPLES;

    // Second pass: RMS with dynamic midpoint
    for (int i = 0; i < SAMPLES; i++) {
      int raw = analogRead(pin);
      float v = (raw * V_REF) / ADC_MAX;
      float centered = v - midpoint;
      sumSq += centered * centered;
    }

    return sqrt(sumSq / SAMPLES);
  }

  float measureCurrent(int pin, float zeroOff) {
    float rawRMS = measureRawRMS(pin);
    float corrected = rawRMS - zeroOff;
    if (corrected < 0) corrected = 0;

    float rmsCurrent = corrected / SENSITIVITY;
    return (rmsCurrent < NOISE_FLOOR) ? 0 : rmsCurrent;
  }

  float average(float* buf) {
    float sum = 0;
    for (int i = 0; i < AVG_COUNT; i++) {
      sum += buf[i];
    }
    return sum / AVG_COUNT;
  }

public:
  void calibrate() {
    Serial.println("Calibrating CT sensors... Keep motor OFF!");
    delay(2000);

    zeroOffset[0] = measureRawRMS(ADC_CURRENT_L1);
    zeroOffset[1] = measureRawRMS(ADC_CURRENT_L2);
    zeroOffset[2] = measureRawRMS(ADC_CURRENT_L3);

    Serial.printf("Zero offsets: L1=%.3f L2=%.3f L3=%.3f\n",
                  zeroOffset[0], zeroOffset[1], zeroOffset[2]);
    calibrated = true;
  }

  void read(float* i1, float* i2, float* i3) {
    if (!calibrated) {
      *i1 = *i2 = *i3 = 0;
      return;
    }

    // Measure with zero offset correction
    float raw1 = measureCurrent(ADC_CURRENT_L1, zeroOffset[0]);
    float raw2 = measureCurrent(ADC_CURRENT_L2, zeroOffset[1]);
    float raw3 = measureCurrent(ADC_CURRENT_L3, zeroOffset[2]);

    // Rolling average
    buffer[0][bufferIndex] = raw1;
    buffer[1][bufferIndex] = raw2;
    buffer[2][bufferIndex] = raw3;
    bufferIndex = (bufferIndex + 1) % AVG_COUNT;

    *i1 = average(buffer[0]);
    *i2 = average(buffer[1]);
    *i3 = average(buffer[2]);
  }

  bool isMotorRunning(float i1, float i2, float i3) {
    float total = (i1 + i2 + i3) / 3.0f;
    return total > 0.1f;  // >0.1A average = motor running
  }
};

#endif // SENSOR_CURRENT_ZMCT103C

#endif // FEATURE_EXTRACTOR_H