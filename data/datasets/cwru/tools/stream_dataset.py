#!/usr/bin/env python3
"""
CWRU Dataset Serial Streamer
Streams binary samples to ESP32/RP2350 via UART @ 115200 baud
"""

import serial
import struct
import time
import sys
from pathlib import Path

def verify_header(header_bytes):
    """Parse and validate 16-byte header"""
    magic, num_samples, feature_dim, fault_type, sample_rate = struct.unpack('<IHBBI', header_bytes[:12])

    if magic != 0x4B4D4541:  # "KMEA"
        raise ValueError(f"Invalid magic: 0x{magic:08X}")

    print(f"Header validated:")
    print(f"  Samples: {num_samples}")
    print(f"  Features: {feature_dim}")
    print(f"  Fault type: {fault_type} (0=normal, 1=ball, 2=inner, 3=outer)")
    print(f"  Sample rate: {sample_rate} Hz")

    return num_samples, feature_dim

def stream_binary(port, binary_file, verbose=False):
    """Stream binary dataset to MCU via Serial"""

    if not Path(binary_file).exists():
        print(f"ERROR: File not found: {binary_file}")
        return False

    try:
        ser = serial.Serial(port, 115200, timeout=5)
        print(f"Serial port opened: {port}")
        time.sleep(2)  # Wait for Arduino reset

        with open(binary_file, 'rb') as f:
            # Send header
            header_bytes = f.read(16)
            if len(header_bytes) != 16:
                print("ERROR: Invalid header size")
                return False

            num_samples, feature_dim = verify_header(header_bytes)
            ser.write(header_bytes)
            print("Header sent. Waiting for MCU confirmation...")

            # Wait for header ACK
            ack = ser.read(1)
            if ack != b'\x06':
                print(f"ERROR: No header ACK (got: {ack.hex() if ack else 'timeout'})")
                return False

            print("Header ACK received. Streaming samples...\n")

            # Stream samples
            sample_size = feature_dim * 4  # 4 bytes per fixed_t (Q16.16)
            samples_sent = 0
            start_time = time.time()
            failed_acks = 0

            while samples_sent < num_samples:
                sample_bytes = f.read(sample_size)
                if not sample_bytes:
                    break

                ser.write(sample_bytes)

                # Wait for ACK
                ack = ser.read(1)
                if ack != b'\x06':
                    failed_acks += 1
                    print(f"WARNING: No ACK for sample {samples_sent} (failed: {failed_acks})")
                    if failed_acks > 10:
                        print("ERROR: Too many failed ACKs. Aborting.")
                        return False

                samples_sent += 1

                if verbose or samples_sent % 10 == 0:
                    elapsed = time.time() - start_time
                    rate = samples_sent / elapsed if elapsed > 0 else 0
                    print(f"Progress: {samples_sent}/{num_samples} ({rate:.1f} samples/sec)")

            elapsed = time.time() - start_time
            avg_rate = samples_sent / elapsed if elapsed > 0 else 0

            print(f"\n=== Streaming Complete ===")
            print(f"Samples sent: {samples_sent}/{num_samples}")
            print(f"Time elapsed: {elapsed:.2f}s")
            print(f"Average rate: {avg_rate:.1f} samples/sec")
            print(f"Failed ACKs: {failed_acks}")

            return failed_acks == 0

    except serial.SerialException as e:
        print(f"ERROR: Serial port error: {e}")
        return False
    except Exception as e:
        print(f"ERROR: {e}")
        return False
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 stream_dataset.py <port> <binary_file> [--verbose]")
        print("\nExample:")
        print("  Linux:   python3 stream_dataset.py /dev/ttyUSB0 data/datasets/cwru/binary/normal.bin")
        print("  Windows: python3 stream_dataset.py COM3 data/datasets/cwru/binary/normal.bin")
        print("  macOS:   python3 stream_dataset.py /dev/tty.usbserial-* data/datasets/cwru/binary/normal.bin")
        sys.exit(1)

    port = sys.argv[1]
    binary_file = sys.argv[2]
    verbose = '--verbose' in sys.argv

    success = stream_binary(port, binary_file, verbose)
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()