/**
 * @file feature_extractor.h
 * @brief Time-domain feature extraction for vibration analysis
 *
 * Extracts bearing fault indicators from 3-axis accelerometer.
 * Schema v1: [rms, peak, crest] (3D)
 * Future v2: Add kurtosis (4D) when windowing implemented
 */

#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include <Arduino.h>
#include <math.h>

class FeatureExtractor {
public:
  /**
   * Extract features from single 3-axis reading (instantaneous)
   * @param ax, ay, az Acceleration in m/s²
   * @param features Output array [rms, peak, crest]
   */
  static void extractInstantaneous(float ax, float ay, float az, float* features) {
    // Compute magnitude vector
    float mag_x = abs(ax);
    float mag_y = abs(ay);
    float mag_z = abs(az);

    // Feature 0: RMS (root mean square)
    // RMS = sqrt((x² + y² + z²) / 3)
    float rms = sqrt((ax*ax + ay*ay + az*az) / 3.0f);

    // Feature 1: Peak (maximum component)
    float peak = max(mag_x, max(mag_y, mag_z));

    // Feature 2: Crest Factor (peak/RMS)
    // High crest = impulsive spikes (bearing defects)
    float crest = (rms > 0.01f) ? (peak / rms) : 1.0f;  // Avoid div by zero

    features[0] = rms;
    features[1] = peak;
    features[2] = crest;
  }

  /**
   * Get feature dimension for schema v1
   */
  static constexpr uint8_t getFeatureDim() {
    return 3;  // [rms, peak, crest]
  }

  /**
   * Get feature names (for SCADA mapping)
   */
  static void getFeatureNames(char names[][32]) {
    strcpy(names[0], "rms");
    strcpy(names[1], "peak");
    strcpy(names[2], "crest");
  }
};

// TODO: Add windowed feature extractor when needed
// class WindowedFeatureExtractor {
//   // Will add: kurtosis, variance using circular buffer
// };

#endif // FEATURE_EXTRACTOR_H