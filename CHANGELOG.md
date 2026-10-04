# RuSwitcher for Windows — Changelog & History

RuSwitcher is an open-source, lightweight, keyboard layout switcher and auto-correction utility created by **Rashid Sayfutdinov ([rashn](https://github.com/rashn))**.
- Official Website: [ruswitcher.app](https://ruswitcher.app)
- Upstream Repository: [github.com/rashn/RuSwitcher](https://github.com/rashn/RuSwitcher)
- Fork Repository: [github.com/iLuckyStar/RuSwitcher](https://github.com/iLuckyStar/RuSwitcher)

---

## [0.10.0-beta.2] — 2026-10-04

### 🚀 macOS 3.5.0b Feature Parity Update for Windows
*Fork maintained by [iLuckyStar](https://github.com/iLuckyStar) bringing the latest algorithmic breakthroughs from macOS v3.5.0b to Windows.*

#### Added
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
- **Modern .NET Compatibility:**
  - Configured `<RollForward>LatestMajor</RollForward>` allowing out-of-the-box execution on .NET 8, .NET 9, and .NET 10 runtimes.

---

## [0.10.0-beta.1] — 2026-08-12

### Initial Native & Parity Release by [rashn](https://github.com/rashn)
- **SendInput ABI Fix:** Sized the Win32 `INPUT` union to 40 bytes on x64 (including `MOUSEINPUT`), fixing the critical bug where SendInput was silently rejected by Windows.
- **Compact Native C++ Core (`windows/native`):** Experimental zero-dependency Win32 implementation under 2 MB.
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
- Implemented in C# (.NET 8 WinForms + P/Invoke).
- Manual trigger (double-tap Ctrl, double-tap Shift, Pause/Break).
- System tray icon with layout indicator and Settings window.
- Basic auto-conversion and exception lists.
- Per-application layout memory.
