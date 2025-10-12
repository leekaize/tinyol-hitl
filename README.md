# TinyOL-HITL

**Incremental learning framework for edge devices with human-in-the-loop adaptation.**

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Platform](https://img.shields.io/badge/platform-RP2350-green.svg)](platforms/rp2350/)

## Overview

TinyOL-HITL enables on-device incremental learning with human feedback. No cloud dependency. Streaming algorithms. Industrial integration.

**Key Features:**
- Streaming k-means clustering (<100KB model footprint)
- Human-in-the-loop corrections via MQTT
- Flash-based model persistence
- Industrial protocols (supOS-CE, Ignition)
- 1-year battery life target

**Target Hardware:**
- RP2350 (Raspberry Pi Pico 2 W/WH) - reference implementation
- Extensible to Cortex-M33 platforms

**Baseline:** Improves on TinyOL (2021) with streaming updates and HITL integration.

## Quick Start

See [QUICKSTART.md](docs/guides/QUICKSTART.md) for hardware setup and first run.

## Architecture

```
Sensor → Features → Clustering → WiFi → supOS-CE → Ignition
                           ↑                              ↓
                           └──────── Human Labels ────────┘
```

**Core Components:**
- `core/clustering/` - Streaming algorithms
- `core/persistence/` - Model state management
- `platforms/rp2350/` - Hardware implementation
- `integrations/` - supOS-CE and Ignition connectors

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
- [Research Report](docs/research/report_outline.md) - Paper structure

## Project Status

**Phase:** Active development (Day 1 complete - toolchain validated)

**Roadmap:**
- [x] Toolchain validation
- [ ] Core k-means implementation
- [ ] RP2350 platform support
- [ ] supOS-CE integration
- [ ] Ignition visualization
- [ ] Energy profiling

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup.

**We welcome:**
- Platform ports
- Algorithm improvements
- Integration modules
- Documentation enhancements

## License

Apache-2.0 - See [LICENSE](LICENSE) for details.

## Citation

If you use this framework in research:

```bibtex
@misc{tinyol-hitl,
  author = {Lee Kai Ze},
  title = {TinyOL-HITL: Incremental Learning Framework for Edge Devices with Human-in-the-Loop},
  year = {2025},
  publisher = {GitHub},
  url = {https://github.com/leekaize/tinyol-hitl}
}
```

## References

**Baseline:** TinyOL (2021) - On-device learning with 256KB model, batch processing.

**Our improvements:** Streaming updates, <100KB memory, HITL integration, industrial protocols.

## Contact

- **Issues:** Bug reports and feature requests
- **Discussions:** Architecture questions and use cases
- **Email:** mail@leekaize.com (research collaboration)

---

**Built for:** Embedded researchers, industrial IoT developers, edge ML practitioners