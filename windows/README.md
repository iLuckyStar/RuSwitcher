# RuSwitcher for Windows

> **Original author:** Rashid Sayfutdinov ([@rashn](https://github.com/rashn)) | [ruswitcher.app](https://ruswitcher.app)  
> **Windows port & macOS 3.5.0b parity update:** [@iLuckyStar](https://github.com/iLuckyStar) | [Changelog](file:///C:/Users/Bear/.gemini/antigravity/scratch/RuSwitcher/CHANGELOG.md)

**Status: 0.10.0-beta.2 (macOS 3.5.0b feature parity).** A tray application built on the same philosophy as the macOS original — zero external dependencies, no telemetry, local dictionaries, keycode-based conversion.

## Features (parity with macOS 3.5.0b)

- **Manual trigger** — double-tap Ctrl (default), double-tap Shift, or the Pause/Break key.
  Converts the last typed word, the current selection, or the whole line into the other layout,
  and switches the keyboard. Trigger it again with nothing typed since to reverse (toggle).
- **Change-case hotkey** (issue #29, macOS 3.3.0 parity) — cycle `lower` → `UPPER` → `Title` on the last word or selection.
- **Word-end pipeline** (macOS 3.5.0b parity) — triggers on Space, Enter, and Tab.
- **Text fixes** (macOS 3.5.0b parity) — fixes two initial capitals (`ПРивет` → `Привет`, `TWo` → `Two`) and numbers with misplaced separators (`1ю8` → `1.8`).
- **Brand & IT term recognition** (issue #34) — built-in dictionary of 154 AI/tech brands (`chatgpt`, `docker`, `github`, `kubernetes`, `vscode`, etc.).
- **Whole-line conversion** (issue #24) — convert the entire current line, not just the last word,
  including a keyboard-buffer fallback for Windows Terminal and other console hosts.
- **Smart selection conversion** (issue #22) — keeps words that are already correct, flips only the
  gibberish (dictionary-driven).
- **Trailing punctuation** kept literally (issue #15) — `ghbdtn,` → `привет,`.
- **As-you-type auto conversion** (beta, off by default) — flips a word right after Space when the
  dictionary is confident it was typed in the wrong layout. Precision over recall; reversing an
  auto-conversion with the trigger teaches a "never convert" exception (learn-from-undo).
- **Exception lists** — never-convert / always-convert, editable in Settings → Exceptions.
- **Layout-switch hotkey** (issue #14) — a separate hotkey that only switches the layout.
- **Per-app layout memory** — remembers and restores each application's last-used layout.
- **Layout sound** (issue #7) and a **layout indicator** in the tray menu.
- **Launch at startup**, **auto-update check**, and a settings window.
- **Localized UI** — English and Russian (more languages to follow; falls back to English).

The conversion engine has no application allowlist or compatibility routing by executable name.
Typed words and lines use RuSwitcher's own keyboard buffer; pre-existing selections probe keyboard
copy and then native focused-control copy based on the control's actual response. Notepad, Chrome,
Edge, ChatGPT/Codex, WinForms and Windows Terminal remain the regression matrix, not special cases.

## Engine mapping

| macOS mechanism | Windows counterpart |
|---|---|
| CGEventTap | `SetWindowsHookEx(WH_KEYBOARD_LL)` |
| CGEvent keyboardSetUnicodeString | `SendInput` + `KEYEVENTF_UNICODE` |
| Carbon `UCKeyTranslate` | `ToUnicodeEx` |
| TIS layout switching | `WM_INPUTLANGCHANGEREQUEST` |
| NSSpellChecker | `ISpellChecker` (Windows 8+) |
| NSStatusItem | `Shell_NotifyIcon` |
| per-app frontmost observer | `SetWinEventHook(EVENT_SYSTEM_FOREGROUND)` |

## Build & test (Native Win32 C++20)

Официальная сборка RuSwitcher для Windows является полностью автономным нативным C++20 приложением (`/MT`) без рантайм-зависимостей.

```powershell
# Сборка нативного приложения и тестов через CMake (MSVC)
cmake -B windows/native/build -S windows/native -DCMAKE_BUILD_TYPE=Release
cmake --build windows/native/build --config Release

# Запуск тестов
.\windows\native\build\Release\RuSwitcherNativeTests.exe
```

Итоговый бинарник `RuSwitcher.exe` занимает всего ~247 КБ и не требует .NET или VC++ Redistributable.

## Distribution

- **Release track:** push a `win-vX.Y.Z` tag → the `windows-release` workflow builds, tests, publishes
  the single-file exe (x64 + arm64), compiles the Inno Setup installer, computes SHA-256, and creates
  a GitHub release (pre-release for `0.x`). Code signing runs automatically **if** the
  `WINDOWS_CERT_PFX_BASE64` + `WINDOWS_CERT_PASSWORD` secrets are set; otherwise it ships unsigned.
- **Update feed:** `windows/version.json` (separate from the repository-root `version.json`, which is
  the **macOS** feed and must stay where it is). The app checks it once a day and offers to open the
  download page.
- **Installer:** [`installer/RuSwitcher.iss`](installer/RuSwitcher.iss) (Inno Setup, per-user).
- **winget:** manifest templates in [`winget/`](winget/) — see its README for the submission steps.

The Windows version is versioned **separately** from macOS (`win-vX.Y.Z`).
