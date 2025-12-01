#!/usr/bin/env python3
"""
Download CWRU bearing dataset files.
Caches in ./cache/ directory (gitignored).

Usage: python3 download.py
"""

import os
import urllib.request
from pathlib import Path

# CWRU dataset URLs (12kHz drive end bearing data)
# Source: https://engineering.case.edu/bearingdatacenter/download-data-file
CWRU_FILES = {
    # Normal baseline
    "normal_0": "https://engineering.case.edu/sites/default/files/97.mat",

    # Ball fault (0.007 inch)
    "ball_007": "https://engineering.case.edu/sites/default/files/118.mat",

    # Inner race fault (0.007 inch)
    "inner_007": "https://engineering.case.edu/sites/default/files/105.mat",

    # Outer race fault (0.007 inch, centered)
    "outer_007": "https://engineering.case.edu/sites/default/files/130.mat",
}

CACHE_DIR = Path(__file__).parent / "cache"

def download_file(url: str, dest: Path) -> bool:
    """Download file if not cached."""
    if dest.exists():
        print(f"  ✓ Cached: {dest.name}")
        return True

    print(f"  ↓ Downloading: {dest.name}...")
    try:
        urllib.request.urlretrieve(url, dest)
        print(f"  ✓ Downloaded: {dest.name}")
        return True
    except Exception as e:
        print(f"  ✗ Failed: {e}")
        return False

def main():
    print("CWRU Bearing Dataset Downloader")
    print("=" * 40)

    CACHE_DIR.mkdir(exist_ok=True)

    success = 0
    for name, url in CWRU_FILES.items():
        dest = CACHE_DIR / f"{name}.mat"
        if download_file(url, dest):
            success += 1

    print(f"\nDownloaded: {success}/{len(CWRU_FILES)} files")
    print(f"Cache location: {CACHE_DIR.absolute()}")

    if success == len(CWRU_FILES):
        print("\n✓ Ready for feature extraction")
        print("Run: python3 extract_features.py")
    else:
        print("\n⚠ Some downloads failed. Check network and retry.")
        return 1

    return 0

if __name__ == "__main__":
    exit(main())