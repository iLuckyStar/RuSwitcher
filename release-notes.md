# RuSwitcher for Windows 0.10.0-beta.2 (macOS 3.5.0b Parity Release)

Windows build and feature update based on [rashn/RuSwitcher](https://github.com/rashn/RuSwitcher) (Rashid Sayfutdinov | [ruswitcher.app](https://ruswitcher.app)).

### Available Downloads:
- **`RuSwitcher-0.10.0-beta.2-native-x64.zip` (~123 KB / 209 KB exe)** 🚀 *Recommended & Fastest*:
  Ultra-lightweight native C++20 Win32 standalone binary with zero runtime dependencies (no .NET or VC++ runtimes required, statically linked `/MT`). Instant start, tiny RAM footprint.
- **`RuSwitcher-0.10.0-beta.2-win-x64-compact.exe` (~313 KB)**:
  C# .NET client for systems with .NET 8 / 9 / 10 Desktop Runtime installed.
- **`RuSwitcher-0.10.0-beta.2-win-x64-standalone.zip` (~65 MB)**:
  Full self-contained .NET bundle containing the complete embedded runtime.

### What's New in this Release:
- **Native C++ Port (`windows/native`):** Full feature parity between macOS 3.5.0b and the native C++ Win32 codebase (`RuSwitcher.exe`, ~209 KB).
- **Word-End Pipeline:** Auto-conversion and text fixes trigger on **Enter** and **Tab** in addition to **Space**.
- **Text Fixes (Typos & Formatting):**
  - Two initial capitals correction (`ПРивет` → `Привет`, `TWo` → `Two`).
  - Misplaced punctuation in numbers (`1ю8` → `1.8`, `5б2` → `5,2`).
- **Brand & IT Term Recognition (Issue #34):**
  - Built-in dictionary of 154 tech, AI, and developer brand names (`chatgpt`, `docker`, `github`, `kubernetes`, `vscode`, `gemini`, etc.).
  - Words typed in Russian layout reliably auto-convert to English brands.
- **Change-Case Hotkey (Issue #29, macOS 3.3.0 parity):**
  - Cycles case: `lower` → `UPPER` → `Title` → `lower` for the last typed word or selection.
- **Interactive System Tray Controls:** Live checkboxes for two-caps, number punctuation, and auto-conversion toggles.

### Checksums (SHA-256):
```
FEACC0FB0D52B6E6BF9FBAAC0CC0938181BDFE1FC8E91DC53597C0AEC7272E46  RuSwitcher-0.10.0-beta.2-native-x64.exe
51BCB710CB0C974E3428557E0C8DEC37765A73388D9BD30C0685D9077B896B61  RuSwitcher-0.10.0-beta.2-native-x64.zip
31dc6467cde8da3ad6b873334feeb8a0bfdefaa8afbebf83daf6b6d9124d9d41  RuSwitcher-0.10.0-beta.2-win-x64-compact.exe
4e1abb274722d3a10c015d36273b59dc473a26f38fa6dd700b1610cf567b8d2e  RuSwitcher-0.10.0-beta.2-win-x64-compact.zip
491a1c323cff663cde62bc8a51ec92ae67a9bcea1df137f71ad0000e200ec1a4  RuSwitcher-0.10.0-beta.2-win-x64-standalone.exe
25377fc1eb26c19ea877960bff695b7e81f3985ad0195b21f2372feb6f5b9f47  RuSwitcher-0.10.0-beta.2-win-x64-standalone.zip
```
