# SARP-Linggo-ASI

Native DirectX 9 / ImGui ASI translation and roleplay assistant plugin for GTA San Andreas and SA-MP. Fully open-source and free for everyone.

## Features

- **DirectX 9 Native ImGui Overlay**: Clean in-game UI overlay rendered inside the game pipeline with keyboard shortcuts (`Shift + H` to toggle visibility, `Shift + Enter` to unlock mouse cursor).
- **Clipboard Translation**: Automatically translate copied Indonesian text into English (`CTRL+C`) in real-time with configurable styles (Standard English, American Hood / Ghetto RP).
- **Chatlog Inbound Monitoring**: Automatically tail and parse SA-MP `chatlog.txt` (including CodsMP support) and translate incoming chat messages.
- **Groq API Cloud Translation**: Ultra-fast LLM responses using Groq API (`openai/gpt-oss-20b` or custom models) with automatic multi-key rotation and rate-limit handling.
- **100% Open Source & Free**: No DRM, no licensing restrictions, no activation token required. Anyone can compile and run directly.

## Shortcuts

- `Shift + H`: Toggle overlay window visibility
- `Shift + Enter`: Toggle cursor interaction mode (Locked / Unlocked)

## Building from Source

### Prerequisites

- Visual Studio 2022 / MSVC (x86 32-bit toolchain)
- CMake 3.20+
- Ninja or Visual Studio Generator

### Local Compilation

```bash
cmake -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```

The compiled output `SARPLinggo.asi` will be generated in `build/Release/SARPLinggo.asi`.

## Configuration (`SARPLinggo.ini`)

Place `SARPLinggo.asi` and `SARPLinggo.ini` into your GTA San Andreas root folder:

```ini
[Settings]
GroqAPIKey=gsk_your_groq_api_key_here
GroqModel=openai/gpt-oss-20b
TargetLanguage=Indonesian
OutboundStyle=Standard English
AutoTranslateIC=1
AutoTranslateMeDo=1
EnableClipboardOutbound=1
EnableInboundChatlog=1
```

## Continuous Integration

Automated builds and artifact packaging are managed via GitHub Actions. Tagged releases (`vX.X`) trigger automated compilation and publishing under the release title `ASI Loader vX.X`.

## License

Distributed under the MIT License. See [LICENSE](LICENSE) for details.
