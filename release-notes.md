# RuSwitcher for Windows 0.10.0-beta.2 (macOS 3.5.0b Parity Release)

Windows build and feature update based on [rashn/RuSwitcher](https://github.com/rashn/RuSwitcher) (Rashid Sayfutdinov | [ruswitcher.app](https://ruswitcher.app)).

### Available Downloads:
- **`RuSwitcher-0.10.0-beta.2-native-x64.zip` (~128 KB / 221 KB exe)** 🚀 *Recommended & Fastest*:
  Ultra-lightweight native C++20 Win32 standalone binary with zero runtime dependencies (no .NET or VC++ runtimes required, statically linked `/MT`). Instant start, tiny RAM footprint, full Mac 3.5.0b feature parity.
- **`RuSwitcher-0.10.0-beta.2-win-x64-compact.exe` (~313 KB)**:
  C# .NET client for systems with .NET 8 / 9 / 10 Desktop Runtime installed.
- **`RuSwitcher-0.10.0-beta.2-win-x64-standalone.zip` (~65 MB)**:
  Full self-contained .NET bundle containing the complete embedded runtime.

### Unified Features (macOS 3.5.0b & Windows):
- **Full Hotkey & Trigger Configuration:**
  - **Conversion Trigger:** Double-tap Ctrl, Double-tap Shift, Double-tap Alt, Caps Lock, or Pause/Break.
  - **Instant Layout Switch Hotkey:** Dedicated key to immediately switch between Russian and English (Caps Lock, Double Shift, Double Ctrl, Double Alt, Pause/Break).
  - **Caps Lock Interception:** Clean Punto/Mac-style CapsLock switching without toggling upper case mode!
  - **Fallback Layout Switching:** Pressing trigger with empty buffer switches layout directly instead of doing nothing.
  - **Previous Word Memory:** Pressing trigger after Space still converts the typed word and deletes trailing spaces.
- **Native Settings Dialog:** Full Win32 settings window with hotkey selection, feature checkboxes, and layout pairs.
- **Rich Tray Menu:** Active layout indicator (`⌨ Раскладка: Русский / English`), submenus for trigger, switch, and case keys.
- **Word-End Pipeline:** Auto-conversion and text fixes trigger on **Enter** and **Tab** in addition to **Space**.
- **Text Fixes (Typos & Formatting):**
  - Two initial capitals correction (`ПРивет` → `Привет`, `TWo` → `Two`).
  - Misplaced punctuation in numbers (`1ю8` → `1.8`, `5б2` → `5,2`).
- **Brand & IT Term Recognition (Issue #34):**
  - Built-in dictionary of 154 tech, AI, and developer brand names (`chatgpt`, `docker`, `github`, `kubernetes`, `vscode`, `gemini`, etc.).
- **Change-Case Hotkey (Issue #29, macOS 3.3.0 parity):**
  - Cycles case: `lower` → `UPPER` → `Title` → `lower` for the last typed word or selection.

### Checksums (SHA-256):
```
57BACA52C74D6985FC9D74112711B68739584C6A331E3C6906C714ED8F657EA2  RuSwitcher-0.10.0-beta.2-native-x64.exe
01E7FB1C90DD7095D32732D0ECCEC93B357330AA4D4C5239A7CABC4014AF606A  RuSwitcher-0.10.0-beta.2-native-x64.zip
31dc6467cde8da3ad6b873334feeb8a0bfdefaa8afbebf83daf6b6d9124d9d41  RuSwitcher-0.10.0-beta.2-win-x64-compact.exe
4e1abb274722d3a10c015d36273b59dc473a26f38fa6dd700b1610cf567b8d2e  RuSwitcher-0.10.0-beta.2-win-x64-compact.zip
491a1c323cff663cde62bc8a51ec92ae67a9bcea1df137f71ad0000e200ec1a4  RuSwitcher-0.10.0-beta.2-win-x64-standalone.exe
25377fc1eb26c19ea877960bff695b7e81f3985ad0195b21f2372feb6f5b9f47  RuSwitcher-0.10.0-beta.2-win-x64-standalone.zip
```
