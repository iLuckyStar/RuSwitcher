# RuSwitcher for Windows — Changelog & History

RuSwitcher is an open-source, lightweight, keyboard layout switcher and auto-correction utility created by **Rashid Sayfutdinov ([rashn](https://github.com/rashn))**.
- Official Website: [ruswitcher.app](https://ruswitcher.app)
- Upstream Repository: [github.com/rashn/RuSwitcher](https://github.com/rashn/RuSwitcher)
- Fork Repository: [github.com/iLuckyStar/RuSwitcher](https://github.com/iLuckyStar/RuSwitcher)

---

## [0.10.0-beta.3] — 2026-10-07

### 🖥️ Console & Terminal Compatibility Fix (PowerShell, CMD, Windows Terminal)
*Dedicated update restoring seamless keyboard layout switching and text conversion in all Windows consoles and terminals.*

#### Fixed & Improved
- **🔀 Reliable Layout Switching in Console Windows (`conhost.exe` / `cmd.exe` / `powershell.exe`):**
  - Resolved classic Win32 console subsystem limitation where `conhost.exe` silently ignores foreign unattached `PostMessageW(..., WM_INPUTLANGCHANGEREQUEST)` messages.
  - Implemented dynamic, non-blocking `AttachThreadInput` scope around `ActivateKeyboardLayout(..., KLF_SETFORPROCESS)` and `WM_INPUTLANGCHANGEREQUEST(1, target_layout)` with immediate detachment.
  - Layout switching in PowerShell, CMD, Windows Terminal, and MSYS/Git Bash now occurs instantaneously without dropped events or UI freezes.
- **🛡️ Decoupled Process Safety Architecture (Factory-First with macOS Parity):**
  - Separated `is_protected_foreground()` (strictly for password fields and password managers: `1password.exe`, `bitwarden.exe`, `keepass.exe`, `keepassxc.exe`) from `is_auto_convert_denied()` (terminals and code editors).
  - Terminals (`windowsterminal.exe`, `cmd.exe`, `powershell.exe`, `pwsh.exe`, `conhost.exe`, `bash.exe`, `wsl.exe`, etc.) and code editors (`code.exe`, `devenv.exe`, `idea64.exe`, etc.) are now safely protected against *accidental automatic conversion* on Space/Enter/Tab, preventing command/code mangling.
  - Manual triggers (`convert_or_undo`, Double Ctrl, Shift+Pause, Pause/Break, CapsLock) and layout switching are now fully operational in terminals.
- **🛡️ Prompt Protection in Terminal Line & Case Conversion:**
  - Prevented sending unsupported GUI selection sequences (`Home` $\to$ `Shift+End`, `Ctrl+Shift+Left`) to console windows which previously caused cursor drift or stray escape characters.
  - Line conversion in console windows gracefully operates on typed buffer keystrokes (`VK_BACK` + Unicode typing) or performs direct layout switching.
- **🔄 Upstream Synchronization:**
  - Pulled and reconciled upstream daily snapshots (`4f16414`, `d66db17`).

## [0.10.0-beta.2] — 2026-10-05

### 🚀 macOS 3.5.0b Feature Parity & Windows Stability Update
*Fork maintained by [iLuckyStar](https://github.com/iLuckyStar) bringing the latest algorithmic breakthroughs from macOS v3.5.0b to Windows.*

#### Added & Improved
- **🔤 Cyrillic Punctuation Keys Auto-Conversion (macOS parity issue #22):**
  - Resolved regression where Russian words containing letters situated on punctuation keys (`х` on `[`, `ъ` on `]`, `ж` on `;`, `э` on `'`, `б` on `,`, `ю` on `.`, `ё` on `` ` ``) failed auto-conversion (e.g. `[jhjij` $\to$ `хорошо`, `rf;tncz` $\to$ `кажется`).
  - Generalized heuristic: allows auto-conversion whenever either the typed text or the converted target consists entirely of valid letters.
- **💾 Per-App Keyboard Layout Memory (Запоминание раскладки для каждого приложения):**
  - Full parity with macOS `PerAppLayoutManager.swift` and Windows `AppLayoutTracker`.
  - Remembers the active keyboard layout for each process and restores it automatically upon regaining focus.
  - Safely ignores remote desktop sessions (`mstsc.exe`, `anydesk.exe`, `teamviewer.exe`) so the remote session controls layout natively.
  - Fully configurable via Settings Dialog and Tray context menu.
- **🛡️ Process Safety Policy & Cached UI Automation (Безопасность процессов):**
  - Hardened foreground safety checks to protect terminals (`cmd.exe`, `powershell.exe`, `pwsh.exe`, `windowsterminal.exe`, `bash.exe`, `wsl.exe`, `alacritty.exe`, `wezterm-gui.exe`, `kitty.exe`, `putty.exe`) and password managers (`1password.exe`, `bitwarden.exe`, `keepass.exe`, `keepassxc.exe`).
  - Cached `IUIAutomation` COM singleton for zero-overhead password field detection.
- **⚡ Modal Settings Dialog Isolation Fix:**
  - Removed `PostQuitMessage(0)` from child `SettingsDialog` window destruction, ensuring closing settings never inadvertently terminates the application thread.
- **🎯 Empty Line Cursor Drift Fix:**
  - Fixed cursor jump on line conversion (`convert_line`) when invoked on an empty line: only drops selection with `VK_RIGHT` if text was actually selected.
- **🛡️ Bulletproof Clipboard Protection (Защита буфера обмена):**
  - Complete isolation and preservation of the user's clipboard during text conversion and case cycling.
  - Replaced unreliable OLE `IDataObject` / `OleFlushClipboard` with an exact Win32 multi-format snapshot (`EnumClipboardFormats`, `GlobalAlloc`, `GlobalLock`, `SetClipboardData`).
  - Prior copied data (plain text, formatted HTML/RTF, DIB bitmaps, files `CF_HDROP`) is 100% guaranteed to be preserved and restored.
- **⚡ Zero-Latency Layout Switching & Microfreeze Elimination:**
  - Eliminated inter-process thread queue synchronization (`AttachThreadInput`), switching layouts instantly ($<0.2$ ms) via asynchronous `PostMessageW(..., WM_INPUTLANGCHANGEREQUEST)`.
  - Replaced slow multi-keystroke synthetic typing (`replace_text`, 150+ `SendInput` events per line) with atomic `paste_text` (`Ctrl+V`) for instant line and selection conversions.
  - Reduced clipboard selection detection timeout from 410 ms down to an adaptive $\le 45$ ms.
  - Batched line selection (`Home` $\to$ `Shift+End`) into a single atomic `SendInput` packet.
- **🔤 Full Unicode Cyrillic Case Cycling (Смена регистра кириллицы):**
  - Migrated casing logic from default "C" CRT `<cwctype>` to native Win32 Unicode APIs (`IsCharAlphaW`, `CharUpperBuffW`, `CharLowerBuffW`).
  - Perfectly handles mixed Latin and Cyrillic text (e.g. `llllllllдддд` $\to$ `LLLLLLLLДДДД`).
- **Word-End Pipeline (Space, Enter, Tab):**
  - Auto-conversion and text fixes now trigger on `Enter` and `Tab` in addition to `Space`.
- **Text Fixes (Auto-correction of common typos):**
  - **Two initial capitals:** automatically corrects words like `ПРивет` $\to$ `Привет` or `TWo` $\to$ `Two` while preserving valid acronyms like `IDs`, `PCs`, `CDs`.
  - **Number separator correction:** automatically fixes misplaced dot/comma between digits caused by typing in the wrong layout (e.g. `1ю8` $\to$ `1.8`, `5б2` $\to$ `5,2`).
- **Brand & IT Term Recognition (Issue #34):**
  - Built-in curated dictionary of 154 tech, AI, and developer brand names (`chatgpt`, `docker`, `github`, `kubernetes`, `vscode`, `gemini`, `claude`, etc.).
  - Words typed in Russian layout (e.g. `срфепзе`, `вщслук`, `пуьштш`) now reliably auto-convert to English brands even if the Windows system spellchecker does not have them.
- **Change-Case Hotkey (Issue #29, macOS 3.3.0 parity):**
  - Cycles case: `lower` $\to$ `UPPER` $\to$ `Title Case` $\to$ `lower` for the last typed word or active text selection.
  - Dedicated configurable hotkey support in Settings.
- **UI & Settings:**
  - Added checkboxes for Two-Caps correction, Number punctuation correction, and Enter/Tab word-end pipeline.
  - Added Change-Case hotkey configuration in Settings.
  - Full localization in Russian and English.
- **Ultra-Lightweight Native x64 C++ Engine (`windows/native`, ~247 KB):**
  - Full unification of macOS 3.5.0b algorithmic capabilities and Windows usability:
    - **Full Hotkey & Trigger Configuration:**
      - Conversion trigger: Double-tap Ctrl, Double-tap Shift, Double-tap Alt, Caps Lock, or Pause/Break.
      - Instant layout switch hotkey: Caps Lock, Double Shift, Double Ctrl, Double Alt, Pause/Break, or Off.
      - **Caps Lock Interception:** Punto/Mac-style single-tap CapsLock switching without toggling upper case mode!
      - **Fallback Layout Switching:** Pressing trigger with empty buffer switches layout directly instead of doing nothing.
      - **Previous Word Memory:** Pressing trigger after Space still converts the typed word and deletes trailing spaces (macOS `prevWordKeys` parity).
      - **Robust Layout Switching (x64):** Multi-target dispatch (`AttachThreadInput` + `ActivateKeyboardLayout` + `WM_INPUTLANGCHANGEREQUEST` to focused control).
    - **Native x64 Settings Window (Win32 GUI `SettingsDialog`):** Full graphical settings dialog with comboboxes, checkboxes, and layout selectors.
    - **Rich System Tray Presence:** Active layout indicator (`⌨ Раскладка: Русский / English`), trigger and switch submenus.
    - Zero-allocation binary search lookup for all 154 brand words (`brand_words.h`).
    - Two-caps correction and number punctuation fix (`text_fixes.cpp`).
    - Case cycling (`next_case`: lower $\to$ UPPER $\to$ Title Case).
    - Asynchronous Word-End pipeline triggered on Space, Enter, and Tab.
    - Defensive integration with Windows Spell Checking API (`ISpellCheckerFactory` in `dict.cpp`).
    - Native pure-logic test suite (`RuSwitcherNativeTests.exe`).
    - Binary size: only **247 KB** (139 KB zipped), statically compiled for x64 with `/MT` and `/O1 /Os /GL /LTCG` — zero runtime dependencies!
- **⌨️ Punto Switcher Phrase & Continuous Line Conversion (Буферизация фразы/строки):**
  - Integrated canonical continuous keystroke buffer (`KeystrokeBuffer`) tracking multi-word typing (`ghbdtn vbh rfr ltkf`).
  - Hotkey conversion targets the exact typed phrase (`BufferedLine`) without destructive `Home` $\to$ `Shift+End` selection or wiping console history.
  - Dynamic backward word reconstruction on Backspace (`rebuild_current_word`).
  - 3-scope conversion: Word (`Word`), Phrase (`Phrase`), and System Line (`SystemLine`), configurable in Settings and Tray.
  - Added `Shift + Pause/Break` hotkey for instant phrase conversion.
- **Complete Retirement of .NET Builds & Migration to Pure Native x64:**
  - Fully decommissioned and removed legacy .NET 8/9/10 assemblies and standalone packages (~160+ MB bloat).
  - Windows distribution is now 100% native C++20 (`/MT`, x64), ultra-fast, zero-dependency, and only ~247 KB in size.

---

## [0.10.0-beta.1] — 2026-08-12

### Initial Native & Parity Release by [rashn](https://github.com/rashn)
- **SendInput ABI Fix:** Sized the Win32 `INPUT` union to 40 bytes on x64 (including `MOUSEINPUT`), fixing the critical bug where SendInput was silently rejected by Windows.
- **Native x64 C++ Core (`windows/native`):** Zero-dependency native 64-bit implementation under 2 MB.
- **Capability-Based Routing:** Conversion mechanism routed by field capabilities and state, not process name allowlists.
- **Smart Selection Conversion:** Retains valid words and only flips gibberish using `ISpellChecker`.
- **Whole-Line Conversion (Issue #24):** Convert the entire typed line or selection via Shift+Home.
- **Trailing Punctuation Preservation (Issue #15):** Keeps trailing punctuation literal (`ghbdtn,` $\to$ `привет,`).
- **Input Safety:** Password field detection via UI Automation to prevent leaking or corrupting credentials.
- **Mouse & Focus Invalidation:** Mouse clicks and window focus changes safely reset buffered typing.

---

## [0.9.0] — 2026-08-10

### Initial Windows Port Beta by [rashn](https://github.com/rashn)
- First public beta of RuSwitcher for Windows.
- Historical prototype implemented in C# (.NET 8 WinForms + P/Invoke); fully superseded by native x64 C++20 in 0.10.0.
- Manual trigger (double-tap Ctrl, double-tap Shift, Pause/Break).
- System tray icon with layout indicator and Settings window.
- Basic auto-conversion and exception lists.
- Per-application layout memory.
