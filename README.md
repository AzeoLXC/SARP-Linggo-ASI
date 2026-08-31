# SARP-Linggo-ASI

Native DirectX 9 / ImGui ASI translation and roleplay assistant plugin for GTA San Andreas and SA-MP. Fully open-source and free for everyone.

## Features

- **DirectX 9 Native ImGui Overlay**: Rendered natively inside the game engine loop without requiring external window capture or alt-tabbing.
- **Multi-Provider AI & Custom Endpoints**:
  - **Groq Cloud** (Default, ultra-fast LLM inference)
  - **OpenAI** (GPT-4o, GPT-4o-mini)
  - **DeepSeek** (DeepSeek-V3, DeepSeek-R1)
  - **Anthropic Claude** (Claude 3.5 Sonnet / Haiku)
  - **Google Gemini** (Gemini 1.5 Pro / Flash)
  - **Ollama** (Local offline LLM, no internet or API key required)
  - **OpenRouter** (Unified API gateway)
  - **Custom Endpoints** (LM Studio, vLLM, Text-Gen-WebUI, or any OpenAI-compatible API)
- **Multi-Token Dynamic Rolling Pool**: Automatically distributes requests and rotates through multiple API keys to avoid rate limits.
- **Roleplay Translator**: Tailored presets for GTA SA-MP (`Standard English`, `American Hood`, `/me`, `/do`, In-Character chat).
- **Clipboard & Chatlog Interceptors**: Seamless real-time translation with clipboard hotkeys and automatic `chatlog.txt` parsing.
- **100% Free & Open-Source**: No licensing keys, no hardware locks, no subscription checks.

## Installation

1. Download the latest release from the [Releases](https://github.com/AzeoLXC/SARP-Linggo-ASI/releases) page (e.g. `SARP-Linggo-ASI-v1.1-YYYYMMDD.asi` or `SARPLinggo.asi`).
2. Place the `.asi` file into your GTA San Andreas root directory (alongside `gta_sa.exe` and an ASI loader such as `Silent's ASI Loader` or `vorbisFile.dll`).
3. Launch the game.

## Hotkeys

- `Shift + H`: Toggle overlay menu visibility.
- `Shift + Enter`: Toggle mouse cursor mode (unlock cursor for menu interaction).

## Building from Source

Requirements:
- Windows SDK / MSVC (x86 32-bit)
- CMake >= 3.20
- Ninja

```bash
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_PROCESSOR=X86
cmake --build build --config Release
```

## License

MIT License. Free for roleplay communities and developers.
