#!/usr/bin/env python3
"""
Convert CSV features to MCU-compatible binary format.
Format: 16-byte header + Q16.16 fixed-point samples.
"""

import pandas as pd
import struct
from pathlib import Path
import argparse

MAGIC = 0x4B4D4541  # "KMEA"
FIXED_POINT_SHIFT = 16

def float_to_fixed(value):
    """Convert float to Q16.16 fixed-point."""
    return int(value * (1 << FIXED_POINT_SHIFT))

def label_to_code(label):
    """Map label to fault type code."""
    mapping = {'normal': 0, 'ball': 1, 'inner': 2, 'outer': 3}
    return mapping.get(label, 255)

def convert_to_binary(csv_path, output_dir):
    """Convert single CSV to binary format."""
    df = pd.read_csv(csv_path)

    # Extract metadata
    num_samples = len(df)
    feature_cols = [col for col in df.columns if col != 'label']
    feature_dim = len(feature_cols)
    fault_type = label_to_code(df['label'].iloc[0])

    # Write binary file
    output_path = output_dir / f"{csv_path.stem}.bin"

    with open(output_path, 'wb') as f:
        # Header (16 bytes)
        f.write(struct.pack('<I', MAGIC))              # 4 bytes: magic
        f.write(struct.pack('<H', num_samples))        # 2 bytes: num_samples
        f.write(struct.pack('<B', feature_dim))        # 1 byte: feature_dim
        f.write(struct.pack('<B', fault_type))         # 1 byte: fault_type
        f.write(struct.pack('<I', 12000))              # 4 bytes: sample_rate
        f.write(struct.pack('<I', 0))                  # 4 bytes: reserved

        # Data (num_samples × feature_dim × 4 bytes)
        for _, row in df.iterrows():
            for col in feature_cols:
                fixed_value = float_to_fixed(row[col])
                f.write(struct.pack('<i', fixed_value))

    print(f"✓ {csv_path.name} → {output_path.name} ({output_path.stat().st_size} bytes)")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', type=str, default='features', help='Input directory')
    parser.add_argument('--output', type=str, default='binary', help='Output directory')
    args = parser.parse_args()

    input_dir = Path(args.input)
    output_dir = Path(args.output)
    output_dir.mkdir(exist_ok=True)

    csv_files = list(input_dir.glob('*.csv'))
    print(f"Converting {len(csv_files)} files to binary...\n")

    for csv_path in csv_files:
        try:
            convert_to_binary(csv_path, output_dir)
        except Exception as e:
            print(f"✗ {csv_path.name}: {e}")

    print(f"\nBinary files ready in {output_dir}/")
    print("Copy .bin files to MCU SD card or stream via UART.")

if __name__ == "__main__":
    main()