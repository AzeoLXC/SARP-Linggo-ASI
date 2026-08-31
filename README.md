# SARP-Linggo-ASI

Native DirectX 9 / ImGui ASI translation and roleplay assistant plugin for GTA San Andreas and SA-MP.

## Overview

SARP-Linggo-ASI integrates directly with GTA SA-MP through DirectX 9 runtime hooking, intercepting both in-game chatlog files and Windows clipboard activity to provide real-time bilingual translation (Indonesian <-> English) utilizing LLM backends via the Groq API.

## Features

- **DirectX 9 Hooking & ImGui Overlay**: Embedded in-game graphical interface rendered on top of GTA SA-MP with support for translucent background, custom font size, and locked/unlocked cursor mode.
- **Inbound Chatlog Monitor**: Real-time parsing of `chatlog.txt` to capture dialogue (`SAYS`), actions (`/me`), and descriptions (`/do`).
- **Outbound Clipboard Auto-Translation**: Instant translation of clipboard text (`CTRL+C`) into authentic American Hood slang or Standard English.
- **Dynamic Key Pool Manager**: Multi-token load balancing with rate-limit tracking, cooldown queues, and RPD health checks.
- **Hardware-Locked Licensing**: HMAC-SHA256 based cryptographic token verification tied to hardware identifiers (HWID).

## Shortcuts

- `Shift + H`: Toggle overlay visibility.
- `Shift + Enter`: Toggle cursor interaction mode (Locked / Unlocked).

## Building from Source

### Prerequisites

- Visual Studio 2022 / MSVC v143 (x86 32-bit toolchain)
- CMake 3.20+
- Ninja build system

### Local Compilation

```bash
cmake -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```

The output file `SARPLinggo.asi` will be generated inside the `build/Release` folder.

## Continuous Integration & Release

Automated builds and artifact packaging are managed via GitHub Actions. Tagged releases (`vX.X`) trigger automated compilation and publishing under the release title `ASI Loader vX.X`.

## License

MIT License. See `LICENSE` for details.
