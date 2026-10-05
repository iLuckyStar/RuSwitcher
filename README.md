# RuSwitcher for Windows (Native x64)

<p align="center">
  <img src="windows/native/assets/RuSwitcher.ico" width="128" alt="RuSwitcher Windows Icon">
</p>

<p align="center">
  <b>Легковесный, быстрый и автономный переключатель раскладки клавиатуры и автокорректор для Windows 10 / 11</b><br>
  Современная открытая альтернатива Punto Switcher без рекламы, без телеметрии и без сторонних рантаймов.
</p>

<p align="center">
  <a href="https://github.com/iLuckyStar/RuSwitcher/releases/latest"><img src="https://img.shields.io/github/v/release/iLuckyStar/RuSwitcher?style=flat-square&label=Windows%20Release" alt="Windows Release"></a>
  <img src="https://img.shields.io/badge/Platform-Windows%2010%20%2F%2011%20(x64)-0078D6?style=flat-square&logo=windows" alt="Windows 10 / 11 x64">
  <img src="https://img.shields.io/badge/Language-C%2B%2B20%20(Native)-00599C?style=flat-square&logo=c%2B%2B" alt="C++20 Native">
  <img src="https://img.shields.io/badge/Binary%20Size-~247%20KB-success?style=flat-square" alt="Size ~247 KB">
  <img src="https://img.shields.io/badge/Dependencies-Zero%20(%2FMT)-brightgreen?style=flat-square" alt="Zero Dependencies">
  <a href="LICENSE"><img src="https://img.shields.io/github/license/iLuckyStar/RuSwitcher?style=flat-square" alt="MIT License"></a>
</p>

<p align="center">
  <a href="#русский">Русский</a> · <a href="#english">English</a>
</p>

<p align="center">
  <a href="https://github.com/iLuckyStar/RuSwitcher/releases/download/win-v0.10.0-beta.2/RuSwitcher-0.10.0-beta.2-native-x64.exe"><b>⬇️ Скачать RuSwitcher.exe (x64)</b></a>
  &nbsp;·&nbsp;
  <a href="https://github.com/iLuckyStar/RuSwitcher/releases/download/win-v0.10.0-beta.2/RuSwitcher-0.10.0-beta.2-native-x64.zip"><b>📦 Скачать ZIP-архив (~139 КБ)</b></a>
  &nbsp;·&nbsp;
  <a href="https://github.com/iLuckyStar/RuSwitcher/releases">Все релизы</a>
</p>

---

> **О проекте:**  
> Оригинальная концепция и реализация для macOS создана **Рашидом Сайфутдиновым ([@rashn](https://github.com/rashn))** ([ruswitcher.app](https://ruswitcher.app) · [github.com/rashn/RuSwitcher](https://github.com/rashn/RuSwitcher)).  
> Нативный порт для Windows (чистый C++20, x64, алгоритмический паритет и функции Punto Switcher) разрабатывается и поддерживается **[@iLuckyStar](https://github.com/iLuckyStar)**.

---

## Русский

Набрали `ghbdtn` вместо `привет` или `ghbdtn vbh rfr ltkf`? Нажмите горячую клавишу (по умолчанию **двойной тап Ctrl** или **Shift + Pause/Break**) — и RuSwitcher мгновенно переведёт набранный текст в нужную раскладку и переключит клавиатуру.

### 🌟 Ключевые возможности

- ⚡ **Многорежимная конвертация (в стиле Punto Switcher):**
  - **Набранная фраза / строка (`Phrase`)** — непрерывный буфер нажатий (`KeystrokeBuffer`) отслеживает цепочку ввода из одного или нескольких слов (`ghbdtn vbh rfr ltkf`). По горячей клавише переключается именно то, что вы набрали последним, без затирания терминалов, адресных строк или истории консоли.
  - **Последнее набранное слово / выделенный текст (`Word`)** — классический точечный режим конвертации.
  - **Вся строка целиком (`SystemLine`)** — захват строки от начала (`Home` $\to$ `Shift+End`).
  - Быстрое переключение области конвертации доступно в меню трея (`Область конвертации ▶`) и в окне Настроек.
- ⌨️ **Гибкие горячие клавиши и триггеры:**
  - Триггер конвертации: **Двойной тап Ctrl** (по умолчанию), **Shift + Pause/Break**, **Pause/Break**, **Caps Lock**, **Двойной тап Shift**, **Двойной тап Alt**.
  - **Перехват Caps Lock:** одиночный тап переключает раскладку в стиле Mac / Punto Switcher без случайного залипания режима заглавных букв!
  - Отдельный хоткей для быстрой смены языка ввода (без модификации текста).
  - Повторное нажатие триггера отменяет конвертацию (Undo/Toggle).
- 🔤 **Смена регистра (Change-Case Hotkey):**
  - Циклическая смена регистра для последнего набранного слова или выделенного фрагмента: `строчные` $\to$ `ПРОПИСНЫЕ` $\to$ `С Заглавной`.
  - Полноценная поддержка кириллицы и латиницы через нативные функции Win32 Unicode.
- 🧠 **Авто-конвертация и Word-End Pipeline:**
  - Автоматическая коррекция раскладки при завершении слова (нажатие `Space`, `Enter` или `Tab`).
  - Умная обработка русских букв, расположенных на клавишах пунктуации (`х`, `ъ`, `ж`, `э`, `б`, `ю`, `ё`): слова вроде `[jhjij` $\to$ `хорошо`, `rf;tncz` $\to$ `кажется` надежно распознаются и конвертируются.
  - Сохранение пунктуации в конце слов (`ghbdtn,` $\to$ `привет,`).
- 🛠️ **Авто-исправление опечаток (Text Fixes):**
  - **Две заглавные буквы подряд:** автоматически исправляет слова вроде `ПРивет` $\to$ `Привет` или `TWo` $\to$ `Two`, сохраняя при этом общепринятые сокращения (`IDs`, `PCs`, `CDs`).
  - **Опечатки в разделителях чисел:** исправляет случайные запятые/точки от другой раскладки (`1ю8` $\to$ `1.8`, `5б2` $\to$ `5,2`).
- 🤖 **Встроенный словарь брендов и IT-терминов (154 слова):**
  - Термины вроде `chatgpt`, `docker`, `github`, `kubernetes`, `vscode`, `gemini`, `claude`, `python` и др. распознаются даже при наборе на русском (`срфепзе`, `вщслук`, `пуьштш`) и гарантированно конвертируются в правильные названия брендов.
- 💾 **Память раскладки для каждого приложения (Per-App Layout Memory):**
  - RuSwitcher запоминает активный язык ввода для каждого окна и процесса, автоматически восстанавливая нужную раскладку при переключении фокуса.
  - Сессии удалённого рабочего стола (`mstsc.exe`, `anydesk.exe`, `teamviewer.exe`) автоматически исключаются, не мешая вводу на удалённом хосте.
- 🛡️ **Защита буфера обмена (Bulletproof Clipboard Protection):**
  - Ваши скопированные данные никогда не теряются: механизм снимка буфера (`ClipboardSnapshot`) сохраняет все форматы (текст, HTML, RTF, картинки, файлы `CF_HDROP`) на время конвертации и восстанавливает их сразу после подстановки.
- 🔒 **Безопасность ввода (Input Safety):**
  - Автоматическое отключение перехвата в терминалах (`cmd`, `powershell`, `pwsh`, `Windows Terminal`, `bash`, `wsl`, `putty`, `kitty`, `alacritty`), полях ввода паролей и менеджерах паролей (`1Password`, `Bitwarden`, `KeePass`) через COM UI Automation.
- 🪶 **Ультра-легковесный монолит C++20:**
  - Размер исполняемого файла всего **~247 КБ** (в zip-архиве ~139 КБ).
  - Скомпилирован со статической линковкой (`/MT`, x64). Никаких зависимостей от .NET Runtime, VC++ Redistributable или Electron.
  - Мгновенный запуск, потребление ОЗУ < 5 МБ, нулевая задержка при переключении раскладок.

---

### 🚀 Быстрый старт

1. Скачайте **[`RuSwitcher-0.10.0-beta.2-native-x64.zip`](https://github.com/iLuckyStar/RuSwitcher/releases/download/win-v0.10.0-beta.2/RuSwitcher-0.10.0-beta.2-native-x64.zip)** или автономный исполняемый файл **`RuSwitcher.exe`**.
2. Распакуйте в любую удобную папку (например, `C:\Program Files\RuSwitcher` или папку пользователя).
3. Запустите `RuSwitcher.exe`. В системном трее появится иконка переключателя с индикатором активной раскладки (`⌨ РУ / EN`).
4. Нажмите правой кнопкой мыши по иконке в трее, чтобы открыть меню настроек, сменить область конвертации или включить автозапуск при входе в систему.

---

### 🛠️ Сборка из исходников

Для сборки требуется Windows 10/11 и установленный **Visual Studio 2022** (с пакетом разработки классических приложений на C++) либо **Build Tools for Visual Studio** + **CMake**:

```powershell
# Клонирование репозитория
git clone https://github.com/iLuckyStar/RuSwitcher.git
cd RuSwitcher

# Конфигурация и сборка нативного C++20 x64 приложения
cmake -B windows/native/build -S windows/native -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build windows/native/build --config Release

# Запуск набора нативных тестов (все тесты должны пройти с отметкой [PASS])
.\windows\native\build\Release\RuSwitcherNativeTests.exe

# Готовый исполняемый файл находится здесь:
# .\windows\native\build\Release\RuSwitcher.exe (~247 КБ)
```

---

## English

Typed `ghbdtn` instead of `привет` or `ghbdtn vbh rfr ltkf`? Simply tap your configured trigger (default: **Double-tap Ctrl** or **Shift + Pause/Break**) — and RuSwitcher instantly converts the text into the correct layout and switches the active keyboard.

### 🌟 Key Features

- ⚡ **Punto Switcher Style Conversion Modes:**
  - **Typed Phrase / Continuous Line (`Phrase`)** — a continuous keystroke buffer (`KeystrokeBuffer`) preserves your typed sequence across multiple words (`ghbdtn vbh rfr ltkf`). The hotkey converts exactly what was typed without wiping command terminals or erasing previous text.
  - **Last Word / Selection (`Word`)** — classic word-by-word conversion.
  - **Whole Line (`SystemLine`)** — line conversion via `Home` $\to$ `Shift+End`.
  - Switch scopes anytime from the system tray menu (`Conversion Scope ▶`) or the Settings dialog.
- ⌨️ **Configurable Hotkeys & Triggers:**
  - Conversion trigger: **Double-tap Ctrl** (default), **Shift + Pause/Break**, **Pause/Break**, **Caps Lock**, **Double-tap Shift**, **Double-tap Alt**.
  - **Caps Lock Interception:** Single tap switches layout like macOS / Punto Switcher without getting stuck in uppercase mode!
  - Independent layout switch hotkey (switches input language without altering text).
  - Repeated trigger press reverts conversion (Undo/Toggle).
- 🔤 **Change-Case Hotkey:**
  - Cycle casing for the last typed word or active text selection: `lower` $\to$ `UPPER` $\to$ `Title Case`.
  - Full Unicode support for Cyrillic and Latin characters via native Win32 Unicode APIs.
- 🧠 **Smart Auto-Conversion & Word-End Pipeline:**
  - As-you-type conversion triggered on Space, Enter, or Tab.
  - Smart handling of Russian letters mapped to punctuation keys (`[`, `]`, `;`, `'`, `,`, `.`, `` ` ``).
  - Trailing punctuation preserved literally (`ghbdtn,` $\to$ `привет,`).
- 🛠️ **Automatic Text Fixes:**
  - **Two initial capitals:** auto-fixes typos like `ПРивет` $\to$ `Привет` or `TWo` $\to$ `Two` while protecting acronyms (`IDs`, `PCs`, `CDs`).
  - **Number punctuation fix:** corrects misplaced commas or dots caused by typing numbers in the wrong layout (`1ю8` $\to$ `1.8`, `5б2` $\to$ `5,2`).
- 🤖 **Curated Tech & AI Brand Dictionary (154 entries):**
  - Dedicated recognition for tech names (`chatgpt`, `docker`, `github`, `kubernetes`, `vscode`, `gemini`, `claude`, etc.) even when typed in Cyrillic (`срфепзе`, `вщслук`, `пуьштш`).
- 💾 **Per-Application Layout Memory:**
  - Remembers the active keyboard layout per application process and restores it automatically upon focus switch.
  - Safely ignores Remote Desktop sessions (`mstsc.exe`, `anydesk.exe`, `teamviewer.exe`).
- 🛡️ **Zero-Loss Clipboard Protection:**
  - The user's clipboard is 100% safeguarded. `ClipboardSnapshot` captures and restores all clipboard formats (plain text, HTML, RTF, bitmaps, shell file drops `CF_HDROP`) after text replacement.
- 🔒 **Input Safety & Privacy:**
  - Automatically disables keystroke capture in terminals (`cmd`, `powershell`, `pwsh`, `Windows Terminal`, `wsl`, `putty`, `kitty`, etc.) and password fields / password managers (`1Password`, `Bitwarden`, `KeePass`) via UI Automation COM.
- 🪶 **Ultra-Lightweight C++20 Native Monolith:**
  - Executable size is only **~247 KB** (139 KB zipped).
  - Statically linked (`/MT`, x64). Zero dependencies on .NET, Electron, or VC++ Redistributable.
  - Instant startup, < 5 MB RAM footprint, zero-latency asynchronous layout switching.

---

### 🛠️ Building from Source

Requires Windows 10/11 and **Visual Studio 2022** (Desktop development with C++) or **MSVC Build Tools** + **CMake**:

```powershell
# Clone the repository
git clone https://github.com/iLuckyStar/RuSwitcher.git
cd RuSwitcher

# Configure and build native C++20 x64 binary
cmake -B windows/native/build -S windows/native -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build windows/native/build --config Release

# Run native pure-logic unit tests
.\windows\native\build\Release\RuSwitcherNativeTests.exe

# The compiled binary is located at:
# .\windows\native\build\Release\RuSwitcher.exe (~247 KB)
```

---

### 📄 License & Trademarks

- **Code:** Licensed under the [MIT License](LICENSE).
- **Trademarks & Attribution:** The original macOS application is developed by Rashid Sayfutdinov ([@rashn](https://github.com/rashn) / [ruswitcher.app](https://ruswitcher.app)). For trademark policies regarding name and icon usage, see [TRADEMARKS.md](TRADEMARKS.md).
