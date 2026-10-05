# Native Windows target (x64 / ARM64)

This is the official production Windows target. Built in modern C++20 using the Win32 API with static linking (`/MT`) and zero external runtime dependencies. The legacy .NET/C# prototype has been completely migrated and decommissioned.

Hard constraints:

- fully standalone `RuSwitcher.exe`; no .NET, Electron or bundled runtime;
- x64 and ARM64;
- release executable budget: **5 MiB maximum**, preferred 1–2 MiB;
- Windows system DLLs only (`user32`, `ole32`, `uiautomationcore`, etc.);
- application names never control conversion behaviour;
- protected/password fields are capability-detected before copy, deletion or injection.

Build on a Visual Studio Windows runner:

```powershell
cmake -S windows/native -B build/native -A x64
cmake --build build/native --config Release
pwsh scripts/check_windows_binary_size.ps1 build/native/Release/RuSwitcher.exe
```

The native target is fully verified for word, phrase/line (Punto Switcher parity), selection, whole-line,
clipboard restoration, caret/mouse invalidation, multiline and protected-field scenarios.

## Migration status

Verified locally on Windows ARM64 with real input events:

- standalone ARM64 and x64 release builds;
- double-Ctrl conversion of the buffered last word (`ghbdtn` to `привет`);
- repeated-trigger undo (`привет` back to `ghbdtn`);
- standard password fields are detected and left unchanged;
- Backspace is mirrored in the typed-word buffer;
- caret movement, mouse clicks, shortcuts and focus/window changes invalidate stale text;
- universal selected-text conversion, including multiline CRLF text;
- exact clipboard restoration, including custom binary formats;
- whole-current-line conversion independent of the foreground application;
- native tray UI with pause/resume, layout pair selection and launch-at-sign-in;
- embedded icon and Windows version metadata;
- the executable imports Windows system DLLs only;
- **macOS 3.5.0b Parity Features:**
  - 154 curated tech/AI brand recognition targets (`brand_words.h`);
  - Two-caps correction (`ПРивет` $\to$ `Привет`, `TWo` $\to$ `Two`);
  - Number punctuation fix (`1ю8` $\to$ `1.8`, `5б2` $\to$ `5,2`);
  - Change-case cycling (`lower` $\to$ `UPPER` $\to$ `Title` $\to$ `lower`);
  - Word-End Pipeline triggering on Space, Enter, and Tab;
  - Windows Spell Checking COM API integration (`ISpellCheckerFactory`);
  - Interactive Tray context menu toggles for all text fixes and auto-conversion;
  - Automated pure-logic native test suite (`RuSwitcherNativeTests.exe`);
- **Punto Switcher Factory Parity:**
  - Continuous keystroke buffer (`KeystrokeBuffer`) tracking multi-word phrases;
  - Hotkey conversion targets the exact typed phrase (`BufferedLine`) without destructive line erasure;
  - Dynamic backward word reconstruction on Backspace (`rebuild_current_word`);
  - 3-scope conversion: Word, Phrase, and System Line.

The native executable is ultra-compact (~247 KB for x64), has zero third-party runtime dependencies, is statically compiled (`/MT`) for x64 and ARM64. Run `scripts/check_windows_binary_size.ps1` for every release build.

