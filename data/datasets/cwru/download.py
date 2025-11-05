#!/usr/bin/env python3
"""
Download CWRU bearing dataset from Case Western Reserve University.
~2 GB total, 240 .mat files across 4 fault types and 4 load conditions.
"""

import os
import urllib.request
from pathlib import Path

BASE_URL = "https://engineering.case.edu/sites/default/files/"
OUTPUT_DIR = Path("raw")

# Fault types: Normal, Ball, Inner Race, Outer Race
# Load conditions: 0, 1, 2, 3 HP
# Fault sizes: 0.007", 0.014", 0.021"

DATASET_FILES = [
    # Normal baseline (0 HP)
    "97.mat", "98.mat", "99.mat", "100.mat",

    # Ball faults (0 HP, 0.007")
    "118.mat", "119.mat", "120.mat", "121.mat",

    # Inner race faults (0 HP, 0.007")
    "105.mat", "106.mat", "107.mat", "108.mat",

    # Outer race faults (0 HP, 0.007", @6:00)
    "130.mat", "131.mat", "132.mat", "133.mat",

    # Add more files as needed for complete dataset
]

def download_file(filename):
    """Download single .mat file from CWRU website."""
    url = BASE_URL + filename
    output_path = OUTPUT_DIR / filename

    if output_path.exists():
        print(f"Skip: {filename} (already exists)")
        return

    print(f"Downloading: {filename}...")
    try:
        urllib.request.urlretrieve(url, output_path)
        print(f"✓ {filename}")
    except Exception as e:
        print(f"✗ {filename}: {e}")

def main():
    OUTPUT_DIR.mkdir(exist_ok=True)

    print(f"Downloading {len(DATASET_FILES)} files from CWRU...")
    print(f"Output directory: {OUTPUT_DIR.resolve()}\n")

    for filename in DATASET_FILES:
        download_file(filename)

    print(f"\nDownload complete. Files in {OUTPUT_DIR}/")
    print("Next: python3 extract_features.py --input raw/ --output features/")

if __name__ == "__main__":
    main()