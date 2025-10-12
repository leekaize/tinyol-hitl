# Contributing to Edge Adaptive ML

Thank you for your interest in contributing.

## Development Setup

### Prerequisites
- ARM GCC toolchain (13.2.1+ or distribution package)
- CMake 3.20+
- Git with LFS support

### Building

```bash
git clone https://github.com/leekaize/edge-adaptive-ml.git
cd edge-adaptive-ml/platforms/rp2350
mkdir build && cd build
cmake .. -DPICO_SDK_PATH=/path/to/pico-sdk
make
```

## Code Standards

- **C code:** ISO C11, `-Wall -Wextra -Werror`
- **Line length:** 100 characters
- **Comments:** Doxygen format for public APIs
- **Testing:** Unit tests required for core algorithms

## Commit Guidelines

- **Format:** `[component] Brief description`
- **Examples:**
  - `[core/clustering] Add online k-means implementation`
  - `[platforms/rp2350] Fix WiFi reconnection bug`
  - `[docs] Update quickstart guide`

## Pull Request Process

1. Fork the repository
2. Create feature branch: `git checkout -b feature/your-feature`
3. Commit changes with descriptive messages
4. Add tests for new functionality
5. Update documentation
6. Submit PR with clear description

## Adding Platform Support

See [docs/guides/PORTING.md] for platform port checklist.

**Required implementations:**
- `platform_init()`
- `platform_read_sensor()`
- `platform_wifi_send()`
- `platform_flash_write()`

## Bug Reports

Include:
- Platform and version
- Steps to reproduce
- Expected vs actual behavior
- Relevant logs

## Feature Requests

Describe:
- Use case motivation
- Proposed API
- Performance implications
- Alternative approaches considered

## License

All contributions licensed under Apache-2.0. By submitting PR, you agree to this.