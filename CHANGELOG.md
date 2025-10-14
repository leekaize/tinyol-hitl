# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Types of changes:
- **Added** - New features
- **Changed** - Changes to existing functionality
- **Deprecated** - Soon-to-be removed features
- **Removed** - Removed features
- **Fixed** - Bug fixes
- **Security** - Vulnerability patches

## [Unreleased]

### Added
- Initial framework structure
- Core streaming k-means implementation
- RP2350 platform support
- supOS-CE integration module
- Ignition visualization templates

## [0.2.0] - 2025-10-14

### Added
- Core streaming k-means algorithm (`core/clustering/`)
  - Q16.16 fixed-point implementation
  - Adaptive learning rate decay
  - 9 unit tests with convergence validation
- RP2350 platform layer (`platforms/rp2350/`)
  - Pico SDK wrapper with USB serial
  - CYW43 LED support for Pico 2 W
  - Synthetic cluster test application
- Documentation
  - Core algorithm README
  - Platform README with build instructions

## [0.1.0] - 2025-10-12

### Added
- Initial repository structure
- Toolchain validation
- Documentation templates (SPEC.md, report_outline.md)
- Build system scaffolding