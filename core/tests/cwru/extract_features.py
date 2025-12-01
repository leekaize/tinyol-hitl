#!/usr/bin/env python3
"""Extract normalized features from CWRU .mat files."""

from pathlib import Path
import numpy as np
from scipy.io import loadmat
from scipy.stats import kurtosis

CACHE_DIR = Path(__file__).parent / "cache"
OUTPUT = Path(__file__).parent / "features.csv"
WINDOW = 512

FILES = {"normal_0": 0, "ball_007": 1, "inner_007": 2, "outer_007": 3}
LABELS = ["normal", "ball", "inner", "outer"]

def load_signal(path):
    data = loadmat(str(path))
    for k in data:
        if k.endswith("_DE_time"): return data[k].flatten()
    for k, v in data.items():
        if not k.startswith("_") and isinstance(v, np.ndarray): return v.flatten()
    raise ValueError(f"No signal in {path}")

def extract(signal):
    feats = []
    for i in range(len(signal) // WINDOW):
        w = signal[i*WINDOW:(i+1)*WINDOW]
        rms = np.sqrt(np.mean(w**2))
        feats.append([rms, kurtosis(w), np.max(np.abs(w))/rms if rms else 0, np.var(w)])
    return feats

def main():
    print("CWRU Feature Extraction")
    X, y = [], []

    for name, label in FILES.items():
        path = CACHE_DIR / f"{name}.mat"
        if not path.exists(): print(f"  ✗ {name}"); continue
        sig = load_signal(path)
        feats = extract(sig)
        X.extend(feats); y.extend([label]*len(feats))
        print(f"  {name}: {len(feats)} samples")

    X, y = np.array(X), np.array(y)

    # Normalize to [0,1]
    xmin, xmax = X.min(0), X.max(0)
    X = (X - xmin) / (xmax - xmin + 1e-8)

    # Shuffle
    np.random.seed(42)
    idx = np.random.permutation(len(X))
    X, y = X[idx], y[idx]

    # Save
    with open(OUTPUT, "w") as f:
        f.write("rms,kurtosis,crest,variance,label\n")
        for i in range(len(X)):
            f.write(f"{X[i,0]:.6f},{X[i,1]:.6f},{X[i,2]:.6f},{X[i,3]:.6f},{int(y[i])}\n")

    print(f"\n✓ {OUTPUT.name}: {len(X)} samples")
    for i in range(4): print(f"  {LABELS[i]}: {np.sum(y==i)}")

if __name__ == "__main__":
    main()