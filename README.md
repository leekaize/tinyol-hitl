# Edge Adaptive ML

**An open-standard framework for adaptive TinyML with human-in-the-loop learning on resource-constrained edge devices.**

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Platform](https://img.shields.io/badge/platform-RP2350-green.svg)](platforms/rp2350/)

## Overview

Edge Adaptive ML enables continuous learning on microcontrollers with human feedback. No cloud required.

**Key Features:**
- Streaming unsupervised learning (online k-means)
- Human-in-the-loop corrections via industrial protocols
- Flash-based model persistence
- Memory-efficient (<100KB model footprint)
- Industrial integration (supOS-CE, Ignition)

**Target Hardware:**
- RP2350 (Raspberry Pi Pico 2 W/WH) - reference implementation
- Extensible to other Cortex-M33 platforms

## Quick Start

See [QUICKSTART.md](docs/guides/QUICKSTART.md) for hardware setup and first run.

## Architecture

```
Sensor → Feature Extraction → Online Clustering → WiFi → UNS → Visualization
                                        ↑                              ↓
                                        └──────── Human Labels ────────┘
```

**Core Components:**
- `core/clustering/` - Device-agnostic streaming algorithms
- `core/persistence/` - Model state management
- `platforms/rp2350/` - Hardware-specific implementation
- `integrations/` - External system connectors

## Use Cases

**Condition Monitoring:**
- Vibration anomaly detection
- Acoustic pattern recognition
- Temperature trend analysis

**Adaptive Systems:**
- User behavior learning
- Environmental adaptation
- Predictive maintenance

## Documentation

- [Setup Guide](docs/guides/QUICKSTART.md) - Hardware validation
- [Technical Spec](docs/research/SPEC.md) - System architecture
- [API Reference](docs/guides/API.md) - Function documentation
- [Research Paper](docs/research/paper.pdf) - Academic publication

## Project Status

**Current:** Reference implementation on RP2350 (Pico 2 WH)

**Roadmap:**
- [ ] Core algorithm library
- [ ] RP2350 platform support
- [ ] supOS-CE integration
- [ ] Ignition visualization templates
- [ ] Additional platform ports (STM32, ESP32)
- [ ] Energy profiling tools

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup and guidelines.

**We welcome:**
- Platform ports
- Algorithm improvements
- Integration modules
- Documentation enhancements

## License

Apache-2.0 - See [LICENSE](LICENSE) for details.

## Citation

If you use this framework in research, please cite:

```bibtex
@misc{edge-adaptive-ml,
  author = {Lee Kai Ze},
  title = {Edge Adaptive ML: An Open-Standard, Adaptive TinyML Framework for Unsupervised Condition Monitoring and Human-in-the-Loop Learning on Edge Devices},
  year = {2025},
  publisher = {GitHub},
  url = {https://github.com/leekaize/edge-adaptive-ml}
}
```

## Contact

- GitHub Issues: Bug reports and feature requests
- Discussions: Architecture questions and use cases
- Email: mail@leekaize.com (research collaboration)

---

**Built for:** Researchers, embedded engineers, industrial IoT developers