# AFKbot Loader

AFKbot Loader — это Windows C++ / Dear ImGui / DirectX11 launcher-control panel для управления несколькими окнами Dota 2 через Sandboxie.

## Возможности

- Loader-style GUI на Dear ImGui / DirectX11
- Запуск выбранных Sandboxie-профилей
- Поиск и привязка окон Dota 2
- Автоматическое определение account_id / Dota ID
- Конвертация:
  - account_id → SteamID64
  - account_id → Steam3
- Модель party:
  - LEADER
  - BOT 02 / BOT 03 / BOT 04 / BOT 05
- Поддержка нескольких party по 5 аккаунтов
- Runtime logs в отдельной Win32-консоли
- Настройки русского / английского языка
- Telegram config
- GC helper sidecar для дальнейшей работы с Game Coordinator
- Защищённое хранение GC-аккаунтов через Windows DPAPI

## Текущий статус

Работает:

- запуск выбранных песочниц;
- проверка окон;
- определение HWND / Sandbox / PID;
- определение account_id через Steam userdata / loginusers;
- построение party model;
- подготовка GC helper;
- вкладка GC Accounts для заполнения данных аккаунтов.

В разработке:

- стабильная сборка party через GC helper;
- автоматический invite / accept;
- улучшение Ready Check;
- доработка визуала отдельных вкладок.

## Сборка

Проект рассчитан на Windows и Visual Studio 2022.
