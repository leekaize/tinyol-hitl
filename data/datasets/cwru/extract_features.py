#!/usr/bin/env python3
"""
Extract time-domain features from CWRU .mat files.
Window size: 256 samples. Features: RMS, kurtosis, crest, variance.
"""

import scipy.io
import numpy as np
import pandas as pd
from pathlib import Path
import argparse

def extract_features(signal, window_size=256, overlap=0.5):
    """Extract features from time-domain signal."""
    step = int(window_size * (1 - overlap))
    features = []

    for i in range(0, len(signal) - window_size, step):
        window = signal[i:i+window_size]

        # RMS: root mean square
        rms = np.sqrt(np.mean(window ** 2))

        # Kurtosis: measure of impulsiveness
        kurtosis = np.mean((window - window.mean()) ** 4) / (window.std() ** 4)

        # Crest factor: peak-to-RMS ratio
        crest = np.max(np.abs(window)) / (rms + 1e-10)

        # Variance: spread
        variance = window.var()

        features.append([rms, kurtosis, crest, variance])

    return np.array(features)

def process_file(mat_path, output_dir):
    """Process single .mat file."""
    data = scipy.io.loadmat(mat_path)

    # CWRU files have variable names like 'X097_DE_time'
    # Find first key containing '_DE_time' (drive-end accelerometer)
    signal_key = [k for k in data.keys() if '_DE_time' in k][0]
    signal = data[signal_key].flatten()

    # Extract features
    features = extract_features(signal)

    # Create DataFrame
    df = pd.DataFrame(features, columns=['rms', 'kurtosis', 'crest', 'variance'])

    # Label from filename (parse fault type)
    label = parse_label(mat_path.name)
    df['label'] = label

    # Save to CSV
    output_path = output_dir / f"{mat_path.stem}_features.csv"
    df.to_csv(output_path, index=False)
    print(f"✓ {mat_path.name} → {output_path.name}")

def parse_label(filename):
    """Extract fault label from CWRU filename convention."""
    # Example: 118.mat → Ball fault
    # Add your labeling logic based on CWRU dataset documentation
    file_num = int(filename.split('.')[0])

    if 97 <= file_num <= 104:
        return 'normal'
    elif 118 <= file_num <= 121:
        return 'ball'
    elif 105 <= file_num <= 108:
        return 'inner'
    elif 130 <= file_num <= 133:
        return 'outer'
    else:
        return 'unknown'

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', type=str, default='raw', help='Input directory')
    parser.add_argument('--output', type=str, default='features', help='Output directory')
    args = parser.parse_args()

    input_dir = Path(args.input)
    output_dir = Path(args.output)
    output_dir.mkdir(exist_ok=True)

    mat_files = list(input_dir.glob('*.mat'))
    print(f"Processing {len(mat_files)} files...\n")

    for mat_path in mat_files:
        try:
            process_file(mat_path, output_dir)
        except Exception as e:
            print(f"✗ {mat_path.name}: {e}")

    print(f"\nFeatures extracted to {output_dir}/")
    print("Next: python3 to_binary.py --input features/ --output binary/")

if __name__ == "__main__":
    main()