# SARP-Linggo-ASI

An open-source DirectX 9 ASI plugin for Grand Theft Auto: San Andreas (SA-MP) providing real-time bidirectional translation and roleplay text assistance powered by multi-provider Large Language Models (LLMs).

## Architecture

The plugin is structured as an x86 dynamic-link library (`.asi`) that interfaces directly with the GTA:SA rendering and window message loops:

- **Direct3D 9 Hooking**: Intercepts `IDirect3DDevice9::Present` and `IDirect3DDevice9::Reset` via MinHook to inject Dear ImGui rendering into the game's presentation pipeline.
- **Input & Window Subclassing**: Hooks the game window `WNDPROC`, `SetCursorPos`, and `ClipCursor` to capture mouse and keyboard input cleanly when the configuration overlay is toggled (`Shift + H`).
- **Asynchronous HTTP Engine**: Utilizes native Windows HTTP (`winhttp.dll`) worker threads for non-blocking API interactions with zero frame drops.
- **Dynamic Key Pool**: Implements a rolling API key manager with failure counters, cool-down timers, and automatic failover.
- **Bidirectional Pipeline**:
  - **Inbound**: Reads `chatlog.txt` changes in real-time, extracts foreign language chat entries, and translates them asynchronously into the overlay log.
  - **Outbound**: Monitors the Windows Clipboard (`CF_TEXT`), translates drafted Indonesian text into context-appropriate roleplay English, and updates the clipboard buffer for in-game pasting (`Ctrl + V` / `T`).

## Supported AI Providers

The translation engine supports standard OpenAI-compatible endpoints, Anthropic Claude, and custom local/remote servers:

| Provider | Default Endpoint | Default Model | Authentication |
| :--- | :--- | :--- | :--- |
| **Groq** | `https://api.groq.com/openai/v1/chat/completions` | `openai/gpt-oss-20b` | `Bearer <API_KEY>` |
| **OpenAI** | `https://api.openai.com/v1/chat/completions` | `gpt-4o-mini` | `Bearer <API_KEY>` |
| **DeepSeek** | `https://api.deepseek.com/chat/completions` | `deepseek-chat` | `Bearer <API_KEY>` |
| **Anthropic Claude** | `https://api.anthropic.com/v1/messages` | `claude-3-5-haiku-latest` | `x-api-key: <API_KEY>` |
| **Google Gemini** | `https://generativelanguage.googleapis.com/v1beta/openai/chat/completions` | `gemini-1.5-flash` | `Bearer <API_KEY>` |
| **OpenRouter** | `https://openrouter.ai/api/v1/chat/completions` | `meta-llama/llama-3.3-70b-instruct` | `Bearer <API_KEY>` |
| **Ollama (Local)** | `http://localhost:11434/v1/chat/completions` | `llama3.2` | None (Optional) |
| **Custom Endpoint** | User Defined (HTTP/HTTPS) | User Defined | Bearer Token / None |

## Configuration (`SARPLinggo.ini`)

Configuration parameters are automatically loaded from and persisted to `SARPLinggo.ini` in the GTA root directory:

```ini
[General]
AutoTranslate=1
OutboundTranslate=1
SelectedStyle=Standard English
SelectedProvider=0
CustomEndpoint=https://api.groq.com/openai/v1/chat/completions
CustomModel=openai/gpt-oss-20b
ChatlogPath=

[API]
Keys=gsk_key1,gsk_key2
```

### Configuration Keys

- `AutoTranslate`: Enables background inbound chatlog parsing (`0` or `1`).
- `OutboundTranslate`: Enables automatic clipboard translation hook (`0` or `1`).
- `SelectedStyle`: Output English styling (`Standard English`, `Street Slang`, `Formal English`, `Casual Conversation`).
- `SelectedProvider`: Provider index (`0`: Groq, `1`: OpenAI, `2`: DeepSeek, `3`: Claude, `4`: Gemini, `5`: Ollama, `6`: OpenRouter, `7`: Custom).
- `CustomEndpoint`: Target API completion URL (supports HTTP/HTTPS and custom ports).
- `CustomModel`: Model identifier string sent in payload `model` field.
- `ChatlogPath`: Custom path to `chatlog.txt` (defaults to standard SA-MP user directory if left empty).
- `Keys`: Comma-separated list of API keys for the dynamic pool.

## Building from Source

### Prerequisites

- Microsoft Visual C++ Compiler (MSVC) with x86 target support (Visual Studio 2022 or Build Tools).
- CMake 3.20 or later.
- Ninja build system (optional, recommended).
- Git.

### Compilation

```bash
# Clone the repository
git clone https://github.com/AzeoLXC/SARP-Linggo-ASI.git
cd SARP-Linggo-ASI

# Configure 32-bit x86 build
cmake -B build -G "Visual Studio 17 2022" -A Win32 -DCMAKE_BUILD_TYPE=Release

# Compile release binary
cmake --build build --config Release
```

The compiled output `SARPLinggo.asi` will be located in `build/Release/`.

## Installation

1. Ensure an ASI loader is installed in your GTA:SA root directory (e.g., Silent's ASI Loader `vorbisFile.dll` or `dinput8.dll`).
2. Copy `SARP-Linggo-ASI-vX.X-YYYYMMDD.asi` (or `SARPLinggo.asi`) into your GTA San Andreas root folder.
3. Launch GTA:SA / SA-MP.
4. Press `Shift + H` in-game to open the configuration overlay and enter your API keys or configure a custom endpoint.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
