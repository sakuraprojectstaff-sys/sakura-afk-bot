#pragma once

// AFKbot Loader V0.2.34 runtime skeleton.
// Safe launcher/control-panel code only: Sandboxie launch, HWND scan,
// window arrangement, console command sending, and local runtime state.

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdint>
#include <cwctype>
#include <iterator>
#include <fstream>
#include <filesystem>
#include <atomic>
#include <thread>
#include <cstdio>
#include <array>
#include <cctype>
#include <cstdlib>
#include <wininet.h>
#include <shellapi.h>
#include <random>
#include <wincrypt.h>
#include <tlhelp32.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "crypt32.lib")

namespace afkbot
{
    inline std::string narrow(const std::wstring& value)
    {
        if (value.empty()) return {};
        int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (size <= 1) return {};
        std::string out(size - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, out.data(), size, nullptr, nullptr);
        return out;
    }

    inline std::wstring widen(const std::string& value)
    {
        if (value.empty()) return {};
        int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
        if (size <= 1) return {};
        std::wstring out(size - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, out.data(), size);
        return out;
    }

    inline std::string hwnd_to_hex(HWND hwnd)
    {
        std::ostringstream ss;
        ss << "0x" << std::uppercase << std::hex << reinterpret_cast<uintptr_t>(hwnd);
        return ss.str();
    }

    inline std::string rect_to_string(const RECT& rc)
    {
        std::ostringstream ss;
        ss << rc.left << "," << rc.top << " " << (rc.right - rc.left) << "x" << (rc.bottom - rc.top);
        return ss.str();
    }

    inline std::wstring get_process_path(DWORD pid)
    {
        std::wstring result;
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!process)
            return result;

        wchar_t buffer[MAX_PATH * 4]{};
        DWORD size = static_cast<DWORD>(std::size(buffer));
        if (QueryFullProcessImageNameW(process, 0, buffer, &size))
            result.assign(buffer, size);
        CloseHandle(process);
        return result;
    }

    inline std::wstring get_process_image_name_from_snapshot(DWORD pid)
    {
        std::wstring result;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
            return result;

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (Process32FirstW(snapshot, &entry))
        {
            do
            {
                if (entry.th32ProcessID == pid)
                {
                    result = entry.szExeFile;
                    break;
                }
            }
            while (Process32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return result;
    }

    inline std::wstring get_process_path_or_name(DWORD pid)
    {
        std::wstring path = get_process_path(pid);
        if (!path.empty())
            return path;
        return get_process_image_name_from_snapshot(pid);
    }

    inline std::wstring get_window_class_name(HWND hwnd)
    {
        wchar_t buffer[256]{};
        int len = GetClassNameW(hwnd, buffer, static_cast<int>(std::size(buffer)));
        if (len <= 0)
            return {};
        return std::wstring(buffer, buffer + len);
    }

    inline std::wstring lowercase(std::wstring s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
        return s;
    }

    struct DotaWindowInfo
    {
        HWND hwnd{};
        DWORD pid{};
        std::string title;
        std::string process_path;
        RECT rect{};
        bool responsive{};
        int slot{};
        int party{ 1 };
        std::string role{"BOT"};
        std::string state{"FOUND"};
    };

    struct BotAccountInfo
    {
        int slot{};
        int party{ 1 };
        std::string role{"BOT"};
        std::string sandbox_name;
        uint32_t dota_id{};
        uint64_t steamid64{};
        std::string steam3;
        HWND hwnd{};
        std::string source{"unknown"};
        std::string state{"UNKNOWN"};
    };

    struct GcAccountCredential
    {
        bool enabled{ true };
        uint32_t dota_id{};
        uint64_t steamid64{};
        std::string sandbox_name;
        std::string account_name_protected;
        std::string password_protected;
        std::string shared_secret_protected;
        std::string status{"missing"};
    };

    enum class ConsoleKey
    {
        F8 = 0,
        Backslash = 1,
        Grave = 2
    };

    struct RuntimeState
    {
        std::vector<DotaWindowInfo> windows;
        std::vector<BotAccountInfo> accounts;
        std::vector<std::string> logs;
        int ready_accept_attempts{};
        bool last_launch_ok{};
        ConsoleKey console_key{ ConsoleKey::Backslash };
        int console_vk_code{ VK_OEM_5 };
        std::string console_key_label{ "\\" };
        bool waiting_console_bind{ false };
        DWORD console_bind_started_ms{ 0 };
        bool dota_console_open_hint{ false };
        bool save_config_enabled{ true };
        bool log_console_visible{ false };
        bool log_console_ready{ false };
        bool log_console_banner_printed{ false };
        bool ui_russian{ false };
        bool remember_login{ false };
        std::string saved_username;
        std::string saved_password_protected;
        int sandbox_launch_count{ 3 };
        int sandbox_launch_wait_seconds{ 45 };
        int sandbox_launch_delay_ms{ 900 };
        std::string sandbox_launch_backend{ "steam_applaunch" };
        std::vector<std::string> sandbox_launch_selected;
        int last_window_scan_skipped_unknown{ 0 };
        int last_window_scan_skipped_outside_target{ 0 };
        int last_window_scan_raw_candidates{ 0 };
        std::string party_invite_backend{ "game_coordinator" };
        std::string party_invite_command_template;
        std::string party_accept_command_template;
        std::string gc_helper_node_exe{ "node" };
        std::string gc_helper_script_path{ "tools\\gc_helper\\gc_party_helper.js" };
        std::string gc_helper_accounts_path{ "data\\config\\gc_accounts.json" };
        std::string gc_helper_accounts_runtime_path{ "data\\config\\gc_accounts_runtime.json" };
        std::string gc_helper_plan_path{ "data\\config\\gc_party_plan.json" };
        int gc_helper_timeout_seconds{ 90 };
        bool gc_helper_detached_console{ true };
        std::vector<GcAccountCredential> gc_accounts;
        int gc_account_selected_index{ 0 };
        uint64_t gc_account_buffer_steam64{};
        char gc_account_login_buf[128]{};
        char gc_account_password_buf[128]{};
        char gc_account_secret_buf[256]{};
        bool auto_pipeline_running{ false };
        bool telegram_reports_enabled{ true };
        std::string telegram_bot_token;
        std::string telegram_chat_id;
        bool telegram_use_proxy{ false };
        std::string telegram_proxy_url;
        std::string telegram_last_status{ "Idle" };
        std::atomic_bool telegram_sending{ false };
        float ready_yes_x{ 0.28f };
        float ready_yes_y{ 0.72f };

        static constexpr uint64_t steam64_base = 76561197960265728ULL;

        RuntimeState()
        {
            load_config();
            load_gc_account_store();
            sync_log_console_visibility(false);
        }

        WORD console_default_color() const
        {
            return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }

        WORD console_tag_color(const std::string& tagged) const
        {
            if (tagged.find("[ERROR]") == 0 || tagged.find("[WARN]") == 0)
                return FOREGROUND_RED | FOREGROUND_INTENSITY;
            if (tagged.find("[CONFIG]") == 0)
                return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            if (tagged.find("[WINDOWS]") == 0)
                return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            if (tagged.find("[PARTY]") == 0)
                return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            if (tagged.find("[READY]") == 0)
                return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            if (tagged.find("[TELEGRAM]") == 0)
                return FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            if (tagged.find("[ACCOUNT]") == 0)
                return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        }

        void console_set_color(WORD color)
        {
            HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
            if (out && out != INVALID_HANDLE_VALUE)
                SetConsoleTextAttribute(out, color);
        }

        void write_console_raw_wide(const std::wstring& text)
        {
            HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
            if (out && out != INVALID_HANDLE_VALUE)
            {
                DWORD written = 0;
                WriteConsoleW(out, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
            }
        }

        void write_console_raw(const std::string& text)
        {
            write_console_raw_wide(widen(text));
        }

        std::wstring console_banner_title() const
        {
            return LR"banner(
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
      ___    ________ __ ____        __
     /   |  / ____/ //_// __ )____  / /_
    / /| | / /_  / ,<  / __  / __ \/ __/
   / ___ |/ __/ / /| |/ /_/ / /_/ / /_
  /_/  |_/_/   /_/ |_/_____/\____/\__/
)banner";
        }

        void write_console_banner()
        {
            console_set_color(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            write_console_raw_wide(console_banner_title());
            console_set_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            write_console_raw_wide(L"\n        RUNTIME LOADER CONSOLE ONLINE\n");
            console_set_color(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            write_console_raw_wide(L"━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
            console_set_color(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            write_console_raw_wide(ui_russian ? L"  [CORE]   Лоадер инициализирован\n" : L"  [CORE]   Loader initialized\n");
            console_set_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            write_console_raw_wide(ui_russian ? L"  [DEBUG]  Консоль подключена\n" : L"  [DEBUG]  Console attached\n");
            console_set_color(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            write_console_raw_wide(ui_russian ? L"  [CONFIG] Ожидание runtime-событий\n" : L"  [CONFIG] Waiting for runtime events\n");
            console_set_color(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            write_console_raw_wide(L"━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
            console_set_color(console_default_color());
        }

        void ensure_log_console()
        {
            HWND hwnd = GetConsoleWindow();
            if (!hwnd)
            {
                if (!AllocConsole())
                    return;

                FILE* stream = nullptr;
                freopen_s(&stream, "CONOUT$", "w", stdout);
                freopen_s(&stream, "CONOUT$", "w", stderr);
                freopen_s(&stream, "CONIN$", "r", stdin);
                log_console_banner_printed = false;
            }

            SetConsoleCP(CP_UTF8);
            SetConsoleOutputCP(CP_UTF8);
            SetConsoleTitleW(L"AFKbot Loader | Runtime Console");
            log_console_ready = true;

            if (!log_console_banner_printed)
            {
                write_console_banner();
                log_console_banner_printed = true;
            }
        }

        void destroy_log_console()
        {
            HWND hwnd = GetConsoleWindow();
            if (hwnd)
                FreeConsole();
            log_console_ready = false;
            log_console_banner_printed = false;
        }

        std::string localize_console_text(const std::string& text) const
        {
            if (!ui_russian)
                return text;

            if (text == "appsettings.cfg saved") return "appsettings.cfg сохранён";
            if (text.find("Config save failed") != std::string::npos) return "Не удалось сохранить appsettings.cfg";
            if (text.find("Console key changed to ") == 0) return "Кнопка консоли изменена на " + text.substr(23);
            if (text.find("Console key capture started") == 0) return "Ожидание бинда консоли. Нажми любую клавишу. ESC отменяет.";
            if (text.find("Console key capture cancelled") == 0) return "Назначение бинда консоли отменено.";
            if (text.find("Save Config toggle") == 0) return text.find("ON") != std::string::npos ? "Сохранение конфига: ON" : "Сохранение конфига: OFF";
            if (text.find("Find Windows completed. Dota windows found: ") == 0) return "Поиск окон завершён. Найдено окон Dota: " + text.substr(43);
            if (text.find("Open Console All sent ") == 0) return "Открытие консоли отправлено во все окна: " + text;
            if (text.find("Launch All requested via Sandboxie") == 0) return "Запуск через Sandboxie отправлен. " + text;
            if (text.find("Arrange 2x5 skipped") == 0) return "Раскладка 2x5 пропущена: окна Dota не найдены.";
            if (text.find("Arrange 2x5 requested") == 0) return "Запрошена раскладка 2x5.";
            if (text.find("Account ID parse skipped") == 0) return "Определение account_id пропущено: окна Dota не найдены.";
            if (text.find("Account ID parse completed") == 0) return "Определение account_id завершено.";
            if (text.find("no account_id found") != std::string::npos) return text;
            if (text.find("parsed account_id=") != std::string::npos) return text;
            if (text.find("Verify Party skipped") == 0) return "Проверка пати пропущена: окна Dota не найдены.";
            if (text.find("Verify Party completed") == 0) return "Проверка пати завершена.";
            if (text.find("no party shared object") != std::string::npos) return text;
            if (text.find("Ready-check skipped") == 0) return "Ready-check пропущен: окно лидера не найдено.";
            if (text.find("Ready-check command sent") == 0) return "Команда ready-check отправлена лидеру.";
            if (text.find("Accept Ready All clicked") == 0) return "Нажатие YES отправлено во все окна.";
            if (text.find("Telegram test message sent") == 0) return "Тестовое сообщение Telegram отправлено.";
            if (text.find("Telegram test failed") == 0) return "Тест Telegram не прошёл. " + text;
            return text;
        }

        static std::string format_console_event(const std::string& text)
        {
            if (text.empty())
                return "[LOG]";
            if (text.size() > 2 && text[0] == '[')
                return text;
            if (text.find("account_id") != std::string::npos || text.find("SteamID") != std::string::npos || text.find("steam64") != std::string::npos)
                return "[ACCOUNT] " + text;
            if (text.find("Config") != std::string::npos || text.find("config") != std::string::npos || text.find("appsettings") != std::string::npos || text.find("конфиг") != std::string::npos)
                return "[CONFIG] " + text;
            if (text.find("Telegram") != std::string::npos)
                return "[TELEGRAM] " + text;
            if (text.find("Find Windows") != std::string::npos || text.find("Window") != std::string::npos || text.find("HWND") != std::string::npos || text.find("окон") != std::string::npos)
                return "[WINDOWS] " + text;
            if (text.find("Party") != std::string::npos || text.find("party") != std::string::npos || text.find("пати") != std::string::npos)
                return "[PARTY] " + text;
            if (text.find("Ready") != std::string::npos || text.find("ready") != std::string::npos)
                return "[READY] " + text;
            return "[CORE] " + text;
        }

        void write_console_line(const std::string& text)
        {
            if (!log_console_visible)
                return;

            ensure_log_console();
            std::string formatted = format_console_event(localize_console_text(text));
            size_t tag_end = formatted.find(']');
            if (tag_end != std::string::npos)
            {
                std::string tag = formatted.substr(0, tag_end + 1);
                std::string rest = formatted.substr(tag_end + 1);
                console_set_color(console_tag_color(tag));
                write_console_raw(tag);
                console_set_color(console_default_color());
                write_console_raw(rest + "\r\n");
            }
            else
            {
                console_set_color(console_default_color());
                write_console_raw(formatted + "\r\n");
            }
        }

        void sync_log_console_visibility(bool replay_logs)
        {
            if (log_console_visible)
            {
                ensure_log_console();
                if (replay_logs)
                {
                    for (const auto& line : logs)
                        write_console_line(line);
                }
            }
            else
            {
                destroy_log_console();
            }
        }

        void set_log_console_visible(bool enabled)
        {
            if (log_console_visible == enabled)
                return;

            log_console_visible = enabled;
            sync_log_console_visibility(false);
            if (save_config_enabled)
                save_config();
        }

        void log(const std::string& text)
        {
            logs.push_back(text);
            if (logs.size() > 80)
                logs.erase(logs.begin(), logs.begin() + static_cast<long long>(logs.size() - 80));
            write_console_line(text);
        }

        uint64_t dota_to_steam64(uint32_t dota_id) const
        {
            return steam64_base + static_cast<uint64_t>(dota_id);
        }

        uint32_t steam64_to_dota(uint64_t steamid64) const
        {
            return steamid64 > steam64_base ? static_cast<uint32_t>(steamid64 - steam64_base) : 0;
        }

        std::string steam3_from_dota(uint32_t dota_id) const
        {
            return "[U:1:" + std::to_string(dota_id) + "]";
        }

        std::string key_name_from_vk(int vk) const
        {
            if (vk >= VK_F1 && vk <= VK_F24)
                return "F" + std::to_string(vk - VK_F1 + 1);

            if (vk >= 'A' && vk <= 'Z')
                return std::string(1, static_cast<char>(vk));

            if (vk >= '0' && vk <= '9')
                return std::string(1, static_cast<char>(vk));

            switch (vk)
            {
            case VK_OEM_5: return "\\";
            case VK_OEM_3: return "` / ~";
            case VK_SPACE: return "SPACE";
            case VK_TAB: return "TAB";
            case VK_ESCAPE: return "ESC";
            case VK_RETURN: return "ENTER";
            case VK_BACK: return "BACKSPACE";
            case VK_INSERT: return "INSERT";
            case VK_DELETE: return "DELETE";
            case VK_HOME: return "HOME";
            case VK_END: return "END";
            case VK_PRIOR: return "PAGE UP";
            case VK_NEXT: return "PAGE DOWN";
            case VK_LEFT: return "LEFT";
            case VK_RIGHT: return "RIGHT";
            case VK_UP: return "UP";
            case VK_DOWN: return "DOWN";
            case VK_NUMPAD0: return "NUM 0";
            case VK_NUMPAD1: return "NUM 1";
            case VK_NUMPAD2: return "NUM 2";
            case VK_NUMPAD3: return "NUM 3";
            case VK_NUMPAD4: return "NUM 4";
            case VK_NUMPAD5: return "NUM 5";
            case VK_NUMPAD6: return "NUM 6";
            case VK_NUMPAD7: return "NUM 7";
            case VK_NUMPAD8: return "NUM 8";
            case VK_NUMPAD9: return "NUM 9";
            case VK_MULTIPLY: return "NUM *";
            case VK_ADD: return "NUM +";
            case VK_SUBTRACT: return "NUM -";
            case VK_DECIMAL: return "NUM .";
            case VK_DIVIDE: return "NUM /";
            case VK_OEM_PLUS: return "+ / =";
            case VK_OEM_MINUS: return "- / _";
            case VK_OEM_4: return "[ / {";
            case VK_OEM_6: return "] / }";
            case VK_OEM_1: return "; / :";
            case VK_OEM_7: return "' / \"";
            case VK_OEM_COMMA: return ", / <";
            case VK_OEM_PERIOD: return ". / >";
            case VK_OEM_2: return "/ / ?";
            default: break;
            }

            return "VK " + std::to_string(vk);
        }

        void set_console_key(int vk, const std::string& label = {})
        {
            console_vk_code = vk;
            console_key_label = label.empty() ? key_name_from_vk(vk) : label;

            if (vk == VK_F8)
                console_key = ConsoleKey::F8;
            else if (vk == VK_OEM_3)
                console_key = ConsoleKey::Grave;
            else if (vk == VK_OEM_5)
                console_key = ConsoleKey::Backslash;

            log("Console key changed to " + console_key_name());
            if (save_config_enabled)
                save_config();
        }

        std::string console_key_name() const
        {
            return console_key_label.empty() ? key_name_from_vk(console_vk_code) : console_key_label;
        }

        WORD console_vk() const
        {
            return static_cast<WORD>(console_vk_code);
        }

        void cycle_console_key()
        {
            if (console_vk_code == VK_F8)
                set_console_key(VK_OEM_5, "\\");
            else if (console_vk_code == VK_OEM_5)
                set_console_key(VK_OEM_3, "` / ~");
            else
                set_console_key(VK_F8, "F8");
        }

        void begin_console_key_capture()
        {
            waiting_console_bind = true;
            console_bind_started_ms = GetTickCount();
            log("Console key capture started. Press any keyboard key. ESC cancels.");
        }

        bool is_forbidden_bind_key(int vk) const
        {
            switch (vk)
            {
            case VK_LBUTTON:
            case VK_RBUTTON:
            case VK_MBUTTON:
            case VK_XBUTTON1:
            case VK_XBUTTON2:
            case VK_SHIFT:
            case VK_CONTROL:
            case VK_MENU:
            case VK_LSHIFT:
            case VK_RSHIFT:
            case VK_LCONTROL:
            case VK_RCONTROL:
            case VK_LMENU:
            case VK_RMENU:
            case VK_LWIN:
            case VK_RWIN:
                return true;
            default:
                return false;
            }
        }

        bool poll_console_key_capture()
        {
            if (!waiting_console_bind)
                return false;

            if (GetTickCount() - console_bind_started_ms < 250)
                return false;

            if (GetAsyncKeyState(VK_ESCAPE) & 1)
            {
                waiting_console_bind = false;
                log("Console key capture cancelled.");
                return false;
            }

            for (int vk = 0x08; vk <= 0xFE; ++vk)
            {
                if (is_forbidden_bind_key(vk) || vk == VK_ESCAPE)
                    continue;

                if (GetAsyncKeyState(vk) & 1)
                {
                    waiting_console_bind = false;
                    set_console_key(vk);
                    return true;
                }
            }

            return false;
        }

        void set_save_config_enabled(bool enabled)
        {
            save_config_enabled = enabled;
            log(std::string("Save Config toggle: ") + (save_config_enabled ? "ON" : "OFF"));
            if (save_config_enabled)
                save_config();
        }

        std::wstring module_dir() const
        {
            wchar_t path[MAX_PATH * 4]{};
            GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
            std::wstring full = path;
            size_t pos = full.find_last_of(L"\\/");
            if (pos == std::wstring::npos)
                return L".";
            return full.substr(0, pos);
        }

        std::wstring config_dir() const
        {
            return module_dir() + L"\\data\\config";
        }

        std::wstring config_path() const
        {
            return config_dir() + L"\\appsettings.cfg";
        }

        void ensure_config_dirs() const
        {
            std::wstring data = module_dir() + L"\\data";
            std::wstring config = config_dir();
            CreateDirectoryW(data.c_str(), nullptr);
            CreateDirectoryW(config.c_str(), nullptr);
        }

        static std::string config_escape(const std::string& value)
        {
            std::string out;
            out.reserve(value.size());
            for (char c : value)
            {
                if (c == '\\') out += "\\\\";
                else if (c == '\n') out += "\\n";
                else if (c == '\r') out += "\\r";
                else out += c;
            }
            return out;
        }

        static std::string config_unescape(const std::string& value)
        {
            std::string out;
            out.reserve(value.size());
            for (size_t i = 0; i < value.size(); ++i)
            {
                if (value[i] == '\\' && i + 1 < value.size())
                {
                    char n = value[++i];
                    if (n == 'n') out += '\n';
                    else if (n == 'r') out += '\r';
                    else out += n;
                }
                else
                {
                    out += value[i];
                }
            }
            return out;
        }

        static std::string config_list_trim(const std::string& value)
        {
            size_t first = 0;
            while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
                ++first;
            size_t last = value.size();
            while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
                --last;
            return value.substr(first, last - first);
        }

        static std::vector<std::string> config_split_list(const std::string& value)
        {
            std::vector<std::string> out;
            std::string item;
            bool esc = false;
            for (char c : value)
            {
                if (esc)
                {
                    item += c;
                    esc = false;
                    continue;
                }
                if (c == '\\')
                {
                    esc = true;
                    continue;
                }
                if (c == '|')
                {
                    item = config_list_trim(item);
                    if (!item.empty() && std::find(out.begin(), out.end(), item) == out.end())
                        out.push_back(item);
                    item.clear();
                    continue;
                }
                item += c;
            }
            item = config_list_trim(item);
            if (!item.empty() && std::find(out.begin(), out.end(), item) == out.end())
                out.push_back(item);
            return out;
        }

        static std::string config_join_list(const std::vector<std::string>& values)
        {
            std::string out;
            for (const auto& value : values)
            {
                if (value.empty())
                    continue;
                if (!out.empty())
                    out += '|';
                for (char c : value)
                {
                    if (c == '\\' || c == '|')
                        out += '\\';
                    out += c;
                }
            }
            return out;
        }

        static std::string trim_copy(const std::string& value)
        {
            size_t first = 0;
            while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
                ++first;

            size_t last = value.size();
            while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
                --last;

            return value.substr(first, last - first);
        }

        static std::string bytes_to_hex(const BYTE* data, DWORD size)
        {
            std::ostringstream ss;
            ss << std::hex << std::uppercase << std::setfill('0');
            for (DWORD i = 0; i < size; ++i)
                ss << std::setw(2) << static_cast<int>(data[i]);
            return ss.str();
        }

        static std::vector<BYTE> hex_to_bytes(const std::string& hex)
        {
            std::vector<BYTE> out;
            if (hex.size() < 2 || (hex.size() % 2) != 0)
                return out;
            out.reserve(hex.size() / 2);
            for (size_t i = 0; i + 1 < hex.size(); i += 2)
            {
                unsigned int value = 0;
                std::stringstream ss;
                ss << std::hex << hex.substr(i, 2);
                if (!(ss >> value))
                    return {};
                out.push_back(static_cast<BYTE>(value & 0xFF));
            }
            return out;
        }

        static std::string protect_string_dpapi(const std::string& plain)
        {
            if (plain.empty())
                return {};

            DATA_BLOB in{};
            in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(plain.data()));
            in.cbData = static_cast<DWORD>(plain.size());

            DATA_BLOB out{};
            if (!CryptProtectData(&in, L"AFKbot Loader saved login", nullptr, nullptr, nullptr, 0, &out))
                return {};

            std::string hex = bytes_to_hex(out.pbData, out.cbData);
            LocalFree(out.pbData);
            return hex;
        }

        static std::string unprotect_string_dpapi(const std::string& hex)
        {
            auto bytes = hex_to_bytes(hex);
            if (bytes.empty())
                return {};

            DATA_BLOB in{};
            in.pbData = bytes.data();
            in.cbData = static_cast<DWORD>(bytes.size());

            DATA_BLOB out{};
            if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &out))
                return {};

            std::string plain(reinterpret_cast<char*>(out.pbData), reinterpret_cast<char*>(out.pbData) + out.cbData);
            LocalFree(out.pbData);
            return plain;
        }

        std::string saved_password_plain() const
        {
            return unprotect_string_dpapi(saved_password_protected);
        }

        void update_saved_login(bool remember, const std::string& username, const std::string& password)
        {
            remember_login = remember;
            if (remember)
            {
                saved_username = username;
                saved_password_protected = protect_string_dpapi(password);
                log("[CONFIG] Login remember enabled. Password stored through Windows DPAPI.");
            }
            else
            {
                saved_username.clear();
                saved_password_protected.clear();
                log("[CONFIG] Login remember disabled. Saved login cleared.");
            }
            if (save_config_enabled)
                save_config();
        }

        std::wstring gc_account_store_path() const
        {
            return config_dir() + L"\\gc_accounts_secure.cfg";
        }

        static bool gc_parse_u64(const std::string& s, uint64_t& out)
        {
            try
            {
                size_t idx = 0;
                unsigned long long v = std::stoull(s, &idx, 10);
                if (idx == 0) return false;
                out = static_cast<uint64_t>(v);
                return true;
            }
            catch (...) { return false; }
        }

        static bool gc_parse_u32(const std::string& s, uint32_t& out)
        {
            try
            {
                size_t idx = 0;
                unsigned long v = std::stoul(s, &idx, 10);
                if (idx == 0) return false;
                out = static_cast<uint32_t>(v);
                return true;
            }
            catch (...) { return false; }
        }

        static std::vector<std::string> split_pipe_line(const std::string& line)
        {
            std::vector<std::string> out;
            std::string part;
            for (char c : line)
            {
                if (c == '|')
                {
                    out.push_back(part);
                    part.clear();
                }
                else part += c;
            }
            out.push_back(part);
            return out;
        }

        int find_gc_account_index_by_steam64(uint64_t steam64) const
        {
            if (!steam64) return -1;
            for (int i = 0; i < static_cast<int>(gc_accounts.size()); ++i)
                if (gc_accounts[static_cast<size_t>(i)].steamid64 == steam64)
                    return i;
            return -1;
        }

        std::string gc_plain_account_name(const GcAccountCredential& e) const
        {
            return unprotect_string_dpapi(e.account_name_protected);
        }

        std::string gc_plain_password(const GcAccountCredential& e) const
        {
            return unprotect_string_dpapi(e.password_protected);
        }

        std::string gc_plain_shared_secret(const GcAccountCredential& e) const
        {
            return unprotect_string_dpapi(e.shared_secret_protected);
        }

        bool gc_account_has_login(const GcAccountCredential& e) const
        {
            return !gc_plain_account_name(e).empty();
        }

        bool gc_account_has_password(const GcAccountCredential& e) const
        {
            return !gc_plain_password(e).empty();
        }

        bool gc_account_complete(const GcAccountCredential& e) const
        {
            return e.enabled && e.steamid64 && gc_account_has_login(e) && gc_account_has_password(e);
        }

        std::string gc_account_status_text(const GcAccountCredential& e) const
        {
            if (!e.enabled) return "disabled";
            if (!e.steamid64) return "no steam64";
            if (!gc_account_has_login(e)) return "login missing";
            if (!gc_account_has_password(e)) return "password missing";
            if (gc_plain_shared_secret(e).empty()) return "ready, guard manual";
            return "ready";
        }

        void refresh_gc_account_edit_buffers()
        {
            if (gc_accounts.empty())
            {
                gc_account_selected_index = 0;
                gc_account_buffer_steam64 = 0;
                gc_account_login_buf[0] = 0;
                gc_account_password_buf[0] = 0;
                gc_account_secret_buf[0] = 0;
                return;
            }
            if (gc_account_selected_index < 0) gc_account_selected_index = 0;
            if (gc_account_selected_index >= static_cast<int>(gc_accounts.size())) gc_account_selected_index = static_cast<int>(gc_accounts.size()) - 1;
            const auto& e = gc_accounts[static_cast<size_t>(gc_account_selected_index)];
            gc_account_buffer_steam64 = e.steamid64;
            strncpy_s(gc_account_login_buf, sizeof(gc_account_login_buf), gc_plain_account_name(e).c_str(), _TRUNCATE);
            strncpy_s(gc_account_password_buf, sizeof(gc_account_password_buf), gc_plain_password(e).c_str(), _TRUNCATE);
            strncpy_s(gc_account_secret_buf, sizeof(gc_account_secret_buf), gc_plain_shared_secret(e).c_str(), _TRUNCATE);
        }

        void load_gc_account_store()
        {
            gc_accounts.clear();
            std::ifstream in(std::filesystem::path(gc_account_store_path()), std::ios::binary);
            if (!in) return;
            std::string line;
            while (std::getline(in, line))
            {
                if (line.empty() || line[0] == '#') continue;
                auto parts = split_pipe_line(line);
                if (parts.size() < 7) continue;
                GcAccountCredential e;
                uint64_t steam64 = 0;
                uint32_t dota = 0;
                gc_parse_u64(parts[0], steam64);
                gc_parse_u32(parts[1], dota);
                e.steamid64 = steam64;
                e.dota_id = dota;
                e.sandbox_name = config_unescape(parts[2]);
                e.enabled = parts[3] != "0";
                e.account_name_protected = config_unescape(parts[4]);
                e.password_protected = config_unescape(parts[5]);
                e.shared_secret_protected = config_unescape(parts[6]);
                e.status = gc_account_status_text(e);
                if (e.steamid64) gc_accounts.push_back(e);
            }
            refresh_gc_account_edit_buffers();
        }

        void save_gc_account_store()
        {
            ensure_config_dirs();
            std::ofstream out(std::filesystem::path(gc_account_store_path()), std::ios::binary | std::ios::trunc);
            if (!out)
            {
                log("[GC] Secure account save failed: cannot open data/config/gc_accounts_secure.cfg");
                return;
            }
            out << "# AFKbot GC accounts. Login/password/shared_secret are Windows DPAPI protected. Do not upload this file.\n";
            for (const auto& e : gc_accounts)
            {
                out << e.steamid64 << "|" << e.dota_id << "|" << config_escape(e.sandbox_name) << "|" << (e.enabled ? 1 : 0) << "|" << config_escape(e.account_name_protected) << "|" << config_escape(e.password_protected) << "|" << config_escape(e.shared_secret_protected) << "\n";
            }
            log("[GC] Secure accounts saved: data/config/gc_accounts_secure.cfg entries=" + std::to_string(gc_accounts.size()));
        }

        void ensure_gc_accounts_from_detected()
        {
            if (accounts.empty()) ensure_demo_accounts_from_windows();
            for (const auto& a : accounts)
            {
                if (!a.steamid64) continue;
                int idx = find_gc_account_index_by_steam64(a.steamid64);
                if (idx < 0)
                {
                    GcAccountCredential e;
                    e.enabled = true;
                    e.steamid64 = a.steamid64;
                    e.dota_id = a.dota_id;
                    e.sandbox_name = a.sandbox_name;
                    e.status = gc_account_status_text(e);
                    gc_accounts.push_back(e);
                }
                else
                {
                    auto& e = gc_accounts[static_cast<size_t>(idx)];
                    e.dota_id = a.dota_id;
                    e.sandbox_name = a.sandbox_name;
                    e.status = gc_account_status_text(e);
                }
            }
            if (gc_account_selected_index >= static_cast<int>(gc_accounts.size())) gc_account_selected_index = gc_accounts.empty() ? 0 : static_cast<int>(gc_accounts.size()) - 1;
            if (!gc_accounts.empty() && gc_account_buffer_steam64 != gc_accounts[static_cast<size_t>(gc_account_selected_index)].steamid64) refresh_gc_account_edit_buffers();
        }

        GcAccountCredential* selected_gc_account()
        {
            ensure_gc_accounts_from_detected();
            if (gc_accounts.empty()) return nullptr;
            if (gc_account_selected_index < 0) gc_account_selected_index = 0;
            if (gc_account_selected_index >= static_cast<int>(gc_accounts.size())) gc_account_selected_index = static_cast<int>(gc_accounts.size()) - 1;
            if (gc_account_buffer_steam64 != gc_accounts[static_cast<size_t>(gc_account_selected_index)].steamid64) refresh_gc_account_edit_buffers();
            return &gc_accounts[static_cast<size_t>(gc_account_selected_index)];
        }

        const GcAccountCredential* selected_gc_account() const
        {
            if (gc_accounts.empty()) return nullptr;
            int idx = gc_account_selected_index;
            if (idx < 0) idx = 0;
            if (idx >= static_cast<int>(gc_accounts.size())) idx = static_cast<int>(gc_accounts.size()) - 1;
            return &gc_accounts[static_cast<size_t>(idx)];
        }

        void select_gc_account_delta(int delta)
        {
            ensure_gc_accounts_from_detected();
            if (gc_accounts.empty()) return;
            apply_gc_account_edit_buffers(false);
            gc_account_selected_index += delta;
            if (gc_account_selected_index < 0) gc_account_selected_index = static_cast<int>(gc_accounts.size()) - 1;
            if (gc_account_selected_index >= static_cast<int>(gc_accounts.size())) gc_account_selected_index = 0;
            refresh_gc_account_edit_buffers();
        }

        void apply_gc_account_edit_buffers(bool save)
        {
            if (gc_accounts.empty()) return;
            if (gc_account_selected_index < 0 || gc_account_selected_index >= static_cast<int>(gc_accounts.size())) return;
            auto& e = gc_accounts[static_cast<size_t>(gc_account_selected_index)];
            e.account_name_protected = protect_string_dpapi(gc_account_login_buf);
            e.password_protected = protect_string_dpapi(gc_account_password_buf);
            e.shared_secret_protected = protect_string_dpapi(gc_account_secret_buf);
            e.status = gc_account_status_text(e);
            if (save) save_gc_account_store();
        }

        void clear_selected_gc_account_secret_data()
        {
            GcAccountCredential* e = selected_gc_account();
            if (!e) return;
            uint64_t steam64 = e->steamid64;
            e->account_name_protected.clear();
            e->password_protected.clear();
            e->shared_secret_protected.clear();
            e->status = gc_account_status_text(*e);
            refresh_gc_account_edit_buffers();
            save_gc_account_store();
            log("[GC] Cleared credentials for steam64=" + std::to_string(steam64));
        }

        int gc_account_total_count()
        {
            ensure_gc_accounts_from_detected();
            return static_cast<int>(gc_accounts.size());
        }

        int gc_account_ready_count()
        {
            ensure_gc_accounts_from_detected();
            int ready = 0;
            for (const auto& e : gc_accounts) if (gc_account_complete(e)) ++ready;
            return ready;
        }

        static std::string powershell_single_quote(const std::string& value)
        {
            std::string out = "'";
            for (char c : value)
            {
                if (c == '\'') out += "''";
                else out += c;
            }
            out += "'";
            return out;
        }

        static std::string url_encode(const std::string& value)
        {
            std::ostringstream escaped;
            escaped << std::hex << std::uppercase;
            for (unsigned char c : value)
            {
                if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
                {
                    escaped << static_cast<char>(c);
                }
                else
                {
                    escaped << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
                }
            }
            return escaped.str();
        }

        static std::string windows_cmd_quote(const std::string& value)
        {
            std::string out = "\"";
            for (char c : value)
            {
                if (c == '"') out += "\\\"";
                else out += c;
            }
            out += "\"";
            return out;
        }

        static std::string capture_command_output(const std::string& command, int* exit_code = nullptr)
        {
            std::string output;
#ifdef _WIN32
            FILE* pipe = _popen(command.c_str(), "r");
#else
            FILE* pipe = popen(command.c_str(), "r");
#endif
            if (!pipe)
            {
                if (exit_code) *exit_code = -1;
                return output;
            }

            std::array<char, 512> buffer{};
            while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
                output += buffer.data();

#ifdef _WIN32
            int rc = _pclose(pipe);
#else
            int rc = pclose(pipe);
#endif
            if (exit_code) *exit_code = rc;
            return output;
        }

        bool send_telegram_via_wininet_post(const std::string& token, const std::string& chat_id, const std::string& message, std::string& response, DWORD& last_error)
        {
            last_error = 0;
            response.clear();

            std::string proxy = telegram_use_proxy ? trim_copy(telegram_proxy_url) : std::string();
            HINTERNET internet = InternetOpenA("AFKbotLoader/0.2", proxy.empty() ? INTERNET_OPEN_TYPE_PRECONFIG : INTERNET_OPEN_TYPE_PROXY, proxy.empty() ? nullptr : proxy.c_str(), nullptr, 0);
            if (!internet)
            {
                last_error = GetLastError();
                return false;
            }

            DWORD timeout_ms = 8000;
            InternetSetOptionA(internet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout_ms, sizeof(timeout_ms));
            InternetSetOptionA(internet, INTERNET_OPTION_SEND_TIMEOUT, &timeout_ms, sizeof(timeout_ms));
            InternetSetOptionA(internet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout_ms, sizeof(timeout_ms));

            HINTERNET connect = InternetConnectA(internet, "api.telegram.org", INTERNET_DEFAULT_HTTPS_PORT,
                nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
            if (!connect)
            {
                last_error = GetLastError();
                InternetCloseHandle(internet);
                return false;
            }

            std::string path = "/bot" + token + "/sendMessage";
            const char* accept_types[] = { "application/json", "*/*", nullptr };
            HINTERNET request = HttpOpenRequestA(connect, "POST", path.c_str(), nullptr, nullptr, accept_types,
                INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI, 0);
            if (!request)
            {
                last_error = GetLastError();
                InternetCloseHandle(connect);
                InternetCloseHandle(internet);
                return false;
            }

            InternetSetOptionA(request, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout_ms, sizeof(timeout_ms));
            InternetSetOptionA(request, INTERNET_OPTION_SEND_TIMEOUT, &timeout_ms, sizeof(timeout_ms));
            InternetSetOptionA(request, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout_ms, sizeof(timeout_ms));

            DWORD secure_flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
            InternetSetOptionA(request, INTERNET_OPTION_SECURITY_FLAGS, &secure_flags, sizeof(secure_flags));

            std::string body = "chat_id=" + url_encode(chat_id) + "&text=" + url_encode(message);
            std::string headers = "Content-Type: application/x-www-form-urlencoded\r\n";
            BOOL sent = HttpSendRequestA(request, headers.c_str(), static_cast<DWORD>(headers.size()),
                body.data(), static_cast<DWORD>(body.size()));
            if (!sent)
            {
                last_error = GetLastError();
                InternetCloseHandle(request);
                InternetCloseHandle(connect);
                InternetCloseHandle(internet);
                return false;
            }

            DWORD status_code = 0;
            DWORD status_size = sizeof(status_code);
            HttpQueryInfoA(request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status_code, &status_size, nullptr);

            char buffer[512]{};
            DWORD read = 0;
            while (InternetReadFile(request, buffer, sizeof(buffer) - 1, &read) && read > 0)
            {
                buffer[read] = 0;
                response += buffer;
                if (response.size() > 4096)
                    break;
            }

            InternetCloseHandle(request);
            InternetCloseHandle(connect);
            InternetCloseHandle(internet);

            if (status_code != 200)
            {
                last_error = status_code;
                return false;
            }
            return true;
        }

        bool send_telegram_via_curl(const std::string& token, const std::string& chat_id, const std::string& message, std::string& response, int& exit_code)
        {
            std::string url = "https://api.telegram.org/bot" + token + "/sendMessage";
            std::string proxy = telegram_use_proxy ? trim_copy(telegram_proxy_url) : std::string();

            auto build_cmd = [&](const std::string& extra) {
                std::string command = "curl.exe -4 -k --ssl-no-revoke -sS --location --connect-timeout 10 --max-time 25 ";
                if (!proxy.empty())
                    command += " --proxy " + windows_cmd_quote(proxy);
                command += " " + extra + " -X POST " + windows_cmd_quote(url) +
                    " -d " + windows_cmd_quote("chat_id=" + chat_id) +
                    " --data-urlencode " + windows_cmd_quote("text=" + message) + " 2>&1";
                return command;
            };

            response = capture_command_output(build_cmd(""), &exit_code);
            if (response.find("\"ok\":true") != std::string::npos)
                return true;

            std::string first_response = response;
            int first_exit = exit_code;

            response = capture_command_output(build_cmd("--resolve api.telegram.org:443:149.154.167.220"), &exit_code);
            if (response.find("\"ok\":true") != std::string::npos)
                return true;

            response = "first_exit=" + std::to_string(first_exit) + "; first=" + first_response + "\nresolve_exit=" + std::to_string(exit_code) + "; resolve=" + response;
            return false;
        }

        bool test_telegram_getme_via_curl(const std::string& token, std::string& response, int& exit_code)
        {
            std::string url = "https://api.telegram.org/bot" + token + "/getMe";
            std::string proxy = telegram_use_proxy ? trim_copy(telegram_proxy_url) : std::string();
            std::string command = "curl.exe -4 -k --ssl-no-revoke -sS --location --connect-timeout 10 --max-time 20 ";
            if (!proxy.empty())
                command += " --proxy " + windows_cmd_quote(proxy);
            command += " " + windows_cmd_quote(url) + " 2>&1";
            response = capture_command_output(command, &exit_code);
            return response.find("\"ok\":true") != std::string::npos;
        }

        void open_telegram_getme_in_browser()
        {
            std::string token = trim_copy(telegram_bot_token);
            if (token.empty())
            {
                telegram_last_status = "Token missing";
                return;
            }
            std::string url = "https://api.telegram.org/bot" + token + "/getMe";
            ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            telegram_last_status = "Opened getMe";
            log("Opened Telegram getMe URL in browser.");
        }

        bool send_telegram_via_powershell(const std::string& token, const std::string& chat_id, const std::string& message, std::string& response, int& exit_code)
        {
            ensure_config_dirs();
            std::wstring script_path = config_dir() + L"\\telegram_send_test.ps1";

            std::ofstream ps(std::filesystem::path(script_path), std::ios::binary | std::ios::trunc);
            if (!ps)
            {
                response = "Cannot create PowerShell script";
                exit_code = -1;
                return false;
            }

            std::string uri = "https://api.telegram.org/bot" + token + "/sendMessage";
            ps << "$ErrorActionPreference = 'Stop'\n";
            ps << "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12\n";
            ps << "$ProgressPreference = 'SilentlyContinue'\n";
            ps << "$uri = " << powershell_single_quote(uri) << "\n";
            ps << "$body = @{ chat_id = " << powershell_single_quote(chat_id) << "; text = " << powershell_single_quote(message) << " }\n";
            if (telegram_use_proxy && !trim_copy(telegram_proxy_url).empty())
                ps << "$proxy = " << powershell_single_quote(trim_copy(telegram_proxy_url)) << "\n";
            ps << "$params = @{ Uri = $uri; Method = 'Post'; Body = $body; TimeoutSec = 20 }\n";
            if (telegram_use_proxy && !trim_copy(telegram_proxy_url).empty())
                ps << "$params.Proxy = $proxy\n";
            ps << "$r = Invoke-RestMethod @params\n";
            ps << "$r | ConvertTo-Json -Compress\n";
            ps.close();

            std::string command = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File " + windows_cmd_quote(narrow(script_path)) + " 2>&1";
            response = capture_command_output(command, &exit_code);
            return response.find("\\\"ok\\\":true") != std::string::npos || response.find("\"ok\":true") != std::string::npos;
        }

        bool send_telegram_message_blocking(const std::string& raw_token, const std::string& raw_chat_id, const std::string& message)
        {
            std::string token = trim_copy(raw_token);
            std::string chat_id = trim_copy(raw_chat_id);

            if (token.empty() || chat_id.empty())
            {
                telegram_last_status = "Token/chat missing";
                log("Telegram test skipped: bot token or chat id is empty.");
                return false;
            }

            std::string getme_response;
            int getme_exit = 0;
            telegram_last_status = "Checking token...";
            bool getme_ok = test_telegram_getme_via_curl(token, getme_response, getme_exit);
            if (!getme_ok)
                log("Telegram getMe failed. exit=" + std::to_string(getme_exit) + "; response=" + getme_response);

            std::string curl_response;
            int curl_exit = 0;
            telegram_last_status = "Sending curl...";
            bool curl_ok = send_telegram_via_curl(token, chat_id, message, curl_response, curl_exit);
            if (curl_ok)
            {
                telegram_last_status = "Sent curl";
                log("Telegram test message sent via curl fallback.");
                return true;
            }
            log("Telegram curl failed. curl_exit=" + std::to_string(curl_exit) + "; response=" + curl_response);

            std::string ps_response;
            int ps_exit = 0;
            telegram_last_status = "Sending PS...";
            bool ps_ok = send_telegram_via_powershell(token, chat_id, message, ps_response, ps_exit);
            if (ps_ok)
            {
                telegram_last_status = "Sent PS";
                log("Telegram test message sent via PowerShell.");
                return true;
            }
            log("Telegram PowerShell failed. ps_exit=" + std::to_string(ps_exit) + "; response=" + ps_response);

            std::string wininet_response;
            DWORD wininet_error = 0;
            telegram_last_status = "Sending WinINet...";
            bool wininet_ok = send_telegram_via_wininet_post(token, chat_id, message, wininet_response, wininet_error);
            if (wininet_ok && wininet_response.find("\"ok\":true") != std::string::npos)
            {
                telegram_last_status = "Sent";
                log("Telegram test message sent via WinINet POST.");
                return true;
            }
            log("Telegram WinINet failed: " + std::to_string(wininet_error) + "; response: " + wininet_response);

            std::string combined = getme_response + " " + curl_response + " " + ps_response + " " + wininet_response;
            if (combined.find("Unauthorized") != std::string::npos || combined.find("401") != std::string::npos)
                telegram_last_status = "Bad bot token";
            else if (combined.find("chat not found") != std::string::npos || combined.find("400") != std::string::npos)
                telegram_last_status = "Bad chat ID";
            else if (wininet_error == 12002 || combined.find("timed out") != std::string::npos || combined.find("Timeout") != std::string::npos)
                telegram_last_status = "Network timeout";
            else if (curl_exit != 0)
                telegram_last_status = "Curl failed " + std::to_string(curl_exit);
            else if (ps_exit != 0)
                telegram_last_status = "PS failed " + std::to_string(ps_exit);
            else
                telegram_last_status = "Telegram API error";

            log("Telegram test failed. curl_exit=" + std::to_string(curl_exit) + "; ps_exit=" + std::to_string(ps_exit) + "; wininet=" + std::to_string(wininet_error));
            return false;
        }

        void send_telegram_test_message()
        {
            if (telegram_sending.exchange(true))
            {
                telegram_last_status = "Already sending";
                return;
            }

            if (save_config_enabled)
                save_config();

            telegram_last_status = "Sending...";
            std::string token = trim_copy(telegram_bot_token);
            std::string chat_id = trim_copy(telegram_chat_id);
            telegram_bot_token = token;
            telegram_chat_id = chat_id;

            std::thread([this, token, chat_id]()
            {
                send_telegram_message_blocking(token, chat_id, "AFKbot Loader test message. Telegram reports are connected.");
                telegram_sending.store(false);
            }).detach();
        }

        void save_config()
        {
            ensure_config_dirs();
            std::ofstream out(std::filesystem::path(config_path()), std::ios::binary | std::ios::trunc);
            if (!out)
            {
                log("Config save failed: cannot open appsettings.cfg");
                return;
            }

            out << "console_key_vk=" << console_vk_code << "\n";
            out << "console_key_name=" << console_key_name() << "\n";
            out << "save_config=" << (save_config_enabled ? 1 : 0) << "\n";
            out << "remember_login=" << (remember_login ? 1 : 0) << "\n";
            out << "saved_username=" << config_escape(saved_username) << "\n";
            out << "saved_password_protected=" << config_escape(saved_password_protected) << "\n";
            out << "sandbox_launch_count=" << sandbox_launch_count << "\n";
            out << "sandbox_launch_wait_seconds=" << sandbox_launch_wait_seconds << "\n";
            out << "sandbox_launch_delay_ms=" << sandbox_launch_delay_ms << "\n";
            out << "sandbox_launch_backend=" << config_escape(sandbox_launch_backend) << "\n";
            out << "sandbox_launch_selected=" << config_escape(config_join_list(sandbox_launch_selected)) << "\n";
            out << "party_invite_backend=" << config_escape(party_invite_backend) << "\n";
            out << "party_invite_command_template=" << config_escape(party_invite_command_template) << "\n";
            out << "party_accept_command_template=" << config_escape(party_accept_command_template) << "\n";
            out << "gc_helper_node_exe=" << config_escape(gc_helper_node_exe) << "\n";
            out << "gc_helper_script_path=" << config_escape(gc_helper_script_path) << "\n";
            out << "gc_helper_accounts_path=" << config_escape(gc_helper_accounts_path) << "\n";
            out << "gc_helper_accounts_runtime_path=" << config_escape(gc_helper_accounts_runtime_path) << "\n";
            out << "gc_helper_plan_path=" << config_escape(gc_helper_plan_path) << "\n";
            out << "gc_helper_timeout_seconds=" << gc_helper_timeout_seconds << "\n";
            out << "gc_helper_detached_console=" << (gc_helper_detached_console ? 1 : 0) << "\n";
            out << "telegram_reports=" << (telegram_reports_enabled ? 1 : 0) << "\n";
            out << "telegram_bot_token=" << config_escape(telegram_bot_token) << "\n";
            out << "telegram_chat_id=" << config_escape(telegram_chat_id) << "\n";
            out << "telegram_use_proxy=" << (telegram_use_proxy ? 1 : 0) << "\n";
            out << "telegram_proxy_url=" << config_escape(telegram_proxy_url) << "\n";
            out << "ready_yes_x=" << ready_yes_x << "\n";
            out << "ready_yes_y=" << ready_yes_y << "\n";
            log("appsettings.cfg saved");
        }

        void load_config()
        {
            std::ifstream in(std::filesystem::path(config_path()), std::ios::binary);
            if (!in)
                return;

            std::string line;
            while (std::getline(in, line))
            {
                size_t eq = line.find('=');
                if (eq == std::string::npos)
                    continue;
                std::string key = line.substr(0, eq);
                std::string value = line.substr(eq + 1);
                if (key == "console_key_vk")
                {
                    try
                    {
                        int vk = std::stoi(value);
                        if (vk >= 0x08 && vk <= 0xFE)
                        {
                            console_vk_code = vk;
                            console_key_label = key_name_from_vk(vk);
                        }
                    }
                    catch (...) {}
                }
                else if (key == "console_key_name")
                {
                    if (!value.empty())
                        console_key_label = value;
                }
                else if (key == "console_key")
                {
                    if (value == "f8")
                    {
                        console_vk_code = VK_F8;
                        console_key_label = "F8";
                    }
                    else if (value == "grave")
                    {
                        console_vk_code = VK_OEM_3;
                        console_key_label = "` / ~";
                    }
                    else
                    {
                        console_vk_code = VK_OEM_5;
                        console_key_label = "\\";
                    }
                }
                else if (key == "save_config")
                {
                    save_config_enabled = value != "0";
                }
                else if (key == "log_console_visible")
                {
                    log_console_visible = value != "0";
                }
                else if (key == "remember_login")
                {
                    remember_login = value != "0";
                }
                else if (key == "saved_username")
                {
                    saved_username = config_unescape(value);
                }
                else if (key == "saved_password_protected")
                {
                    saved_password_protected = config_unescape(value);
                }
                else if (key == "sandbox_launch_count")
                {
                    try { sandbox_launch_count = (std::max)(1, (std::min)(50, std::stoi(value))); } catch (...) {}
                }
                else if (key == "sandbox_launch_wait_seconds")
                {
                    try { sandbox_launch_wait_seconds = (std::max)(5, (std::min)(180, std::stoi(value))); } catch (...) {}
                }
                else if (key == "sandbox_launch_delay_ms")
                {
                    try { sandbox_launch_delay_ms = (std::max)(0, (std::min)(10000, std::stoi(value))); } catch (...) {}
                }
                else if (key == "sandbox_launch_backend")
                {
                    sandbox_launch_backend = config_unescape(value);
                    if (sandbox_launch_backend.empty()) sandbox_launch_backend = "steam_applaunch";
                    if (sandbox_launch_backend == "cmd_uri" || sandbox_launch_backend == "explorer_uri")
                        sandbox_launch_backend = "steam_applaunch";
                }
                else if (key == "sandbox_launch_selected")
                {
                    sandbox_launch_selected = config_split_list(config_unescape(value));
                    sandbox_launch_count = sandbox_launch_selected.empty() ? sandbox_launch_count : static_cast<int>(sandbox_launch_selected.size());
                }
                else if (key == "party_invite_backend")
                {
                    party_invite_backend = config_unescape(value);
                    if (party_invite_backend.empty()) party_invite_backend = "game_coordinator";
                }
                else if (key == "party_invite_command_template")
                {
                    party_invite_command_template = config_unescape(value);
                }
                else if (key == "party_accept_command_template")
                {
                    party_accept_command_template = config_unescape(value);
                }
                else if (key == "gc_helper_node_exe")
                {
                    gc_helper_node_exe = config_unescape(value);
                    if (gc_helper_node_exe.empty()) gc_helper_node_exe = "node";
                }
                else if (key == "gc_helper_script_path")
                {
                    gc_helper_script_path = config_unescape(value);
                    if (gc_helper_script_path.empty()) gc_helper_script_path = "tools\\gc_helper\\gc_party_helper.js";
                }
                else if (key == "gc_helper_accounts_path")
                {
                    gc_helper_accounts_path = config_unescape(value);
                    if (gc_helper_accounts_path.empty()) gc_helper_accounts_path = "data\\config\\gc_accounts.json";
                }
                else if (key == "gc_helper_accounts_runtime_path")
                {
                    gc_helper_accounts_runtime_path = config_unescape(value);
                    if (gc_helper_accounts_runtime_path.empty()) gc_helper_accounts_runtime_path = "data\\config\\gc_accounts_runtime.json";
                }
                else if (key == "gc_helper_plan_path")
                {
                    gc_helper_plan_path = config_unescape(value);
                    if (gc_helper_plan_path.empty()) gc_helper_plan_path = "data\\config\\gc_party_plan.json";
                }
                else if (key == "gc_helper_timeout_seconds")
                {
                    try { gc_helper_timeout_seconds = (std::max)(20, (std::min)(600, std::stoi(value))); } catch (...) {}
                }
                else if (key == "gc_helper_detached_console")
                {
                    gc_helper_detached_console = value != "0";
                }
                else if (key == "telegram_reports")
                {
                    telegram_reports_enabled = value != "0";
                }
                else if (key == "telegram_bot_token")
                {
                    telegram_bot_token = config_unescape(value);
                }
                else if (key == "telegram_chat_id")
                {
                    telegram_chat_id = config_unescape(value);
                }
                else if (key == "telegram_use_proxy")
                {
                    telegram_use_proxy = value != "0";
                }
                else if (key == "telegram_proxy_url")
                {
                    telegram_proxy_url = config_unescape(value);
                }
                else if (key == "ready_yes_x")
                {
                    try { ready_yes_x = std::stof(value); } catch (...) {}
                }
                else if (key == "ready_yes_y")
                {
                    try { ready_yes_y = std::stof(value); } catch (...) {}
                }
            }
        }

        std::string sandbox_name_from_process_path(const std::string& process_path) const
        {
            std::wstring path = widen(process_path);
            std::replace(path.begin(), path.end(), L'/', L'\\');
            std::wstring low = lowercase(path);

            size_t sandbox_pos = low.find(L"\\sandbox\\");
            if (sandbox_pos == std::wstring::npos)
                sandbox_pos = low.find(L"/sandbox/");
            if (sandbox_pos == std::wstring::npos)
                return {};

            size_t box_start = low.find(L'\\', sandbox_pos + 10);
            if (box_start == std::wstring::npos)
                return {};
            ++box_start;

            size_t box_end = low.find(L'\\', box_start);
            if (box_end == std::wstring::npos || box_end <= box_start)
                return {};

            return narrow(path.substr(box_start, box_end - box_start));
        }

        std::string sandbox_name_for_window(const DotaWindowInfo& w, int fallback_slot) const
        {
            std::string box = sandbox_name_from_process_path(w.process_path);
            if (!box.empty())
                return box;
            return "DotaBox" + two_digits(fallback_slot);
        }

        void ensure_demo_accounts_from_windows()
        {
            std::vector<BotAccountInfo> old = accounts;
            std::vector<BotAccountInfo> next;
            int slot = 1;
            for (const auto& w : windows)
            {
                BotAccountInfo acc;
                acc.slot = slot;
                acc.party = w.party > 0 ? w.party : ((slot - 1) / 5 + 1);
                acc.role = !w.role.empty() ? w.role : account_role_for_slot(slot);
                acc.sandbox_name = sandbox_name_for_window(w, slot);
                acc.hwnd = w.hwnd;
                acc.state = "WAIT_ID";

                auto old_it = std::find_if(old.begin(), old.end(), [&](const BotAccountInfo& item)
                {
                    return item.hwnd == w.hwnd || (!item.sandbox_name.empty() && item.sandbox_name == acc.sandbox_name);
                });

                if (old_it != old.end())
                {
                    acc.dota_id = old_it->dota_id;
                    acc.steamid64 = old_it->steamid64;
                    acc.steam3 = old_it->steam3;
                    acc.source = old_it->source;
                    if (!old_it->state.empty() && old_it->state != "UNKNOWN")
                        acc.state = old_it->state;
                }

                next.push_back(acc);
                ++slot;
            }
            accounts = std::move(next);
        }

        static std::string two_digits(int value)
        {
            std::ostringstream ss;
            ss << std::setw(2) << std::setfill('0') << value;
            return ss.str();
        }

        std::filesystem::path sandbox_profiles_path() const
        {
            return std::filesystem::path(config_dir()) / L"sandbox_profiles.json";
        }

        static std::string json_string_value(const std::string& text, const std::string& key)
        {
            std::string needle = "\"" + key + "\"";
            size_t pos = text.find(needle);
            if (pos == std::string::npos)
                return {};
            pos = text.find(':', pos + needle.size());
            if (pos == std::string::npos)
                return {};
            pos = text.find('"', pos);
            if (pos == std::string::npos)
                return {};
            ++pos;

            std::string out;
            bool esc = false;
            for (; pos < text.size(); ++pos)
            {
                char c = text[pos];
                if (esc)
                {
                    if (c == 'n') out += '\n';
                    else if (c == 'r') out += '\r';
                    else out += c;
                    esc = false;
                    continue;
                }
                if (c == '\\')
                {
                    esc = true;
                    continue;
                }
                if (c == '"')
                    break;
                out += c;
            }
            return out;
        }

        static int json_int_value(const std::string& text, const std::string& key, int fallback = 0)
        {
            std::string needle = "\"" + key + "\"";
            size_t pos = text.find(needle);
            if (pos == std::string::npos)
                return fallback;
            pos = text.find(':', pos + needle.size());
            if (pos == std::string::npos)
                return fallback;
            ++pos;
            while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])))
                ++pos;
            size_t start = pos;
            if (pos < text.size() && (text[pos] == '-' || text[pos] == '+'))
                ++pos;
            while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos])))
                ++pos;
            if (pos == start)
                return fallback;
            try { return std::stoi(text.substr(start, pos - start)); }
            catch (...) { return fallback; }
        }

        std::wstring sandboxie_start_exe_from_profiles() const
        {
            std::string text = read_text_file_utf8(sandbox_profiles_path());
            std::string configured = json_string_value(text, "sandboxie_start_exe");
            if (!configured.empty())
            {
                std::wstring wide = widen(configured);
                if (GetFileAttributesW(wide.c_str()) != INVALID_FILE_ATTRIBUTES)
                    return wide;
            }
            return {};
        }

        std::string dota_launch_uri_from_profiles() const
        {
            std::string text = read_text_file_utf8(sandbox_profiles_path());
            std::string configured = json_string_value(text, "dota_launch_uri");
            return configured.empty() ? std::string("steam://rungameid/570") : configured;
        }

        static bool sandbox_backend_is_valid(const std::string& id)
        {
            return id == "cmd_uri" || id == "explorer_uri" || id == "steam_applaunch" || id == "steam_uri" || id == "steam_then_uri" || id == "direct_dota";
        }

        std::string normalized_sandbox_launch_backend() const
        {
            if (sandbox_backend_is_valid(sandbox_launch_backend))
                return sandbox_launch_backend;

            std::string text = read_text_file_utf8(sandbox_profiles_path());
            std::string configured = json_string_value(text, "sandbox_launch_backend");
            return sandbox_backend_is_valid(configured) ? configured : std::string("steam_applaunch");
        }

        std::string sandbox_launch_backend_label() const
        {
            std::string id = normalized_sandbox_launch_backend();
            if (id == "cmd_uri") return "cmd URI";
            if (id == "explorer_uri") return "explorer URI";
            if (id == "steam_applaunch") return "steam -applaunch";
            if (id == "steam_uri") return "steam URI arg";
            if (id == "steam_then_uri") return "steam first + URI";
            if (id == "direct_dota") return "direct dota2.exe";
            return "cmd URI";
        }

        void cycle_sandbox_launch_backend(int delta)
        {
            std::vector<std::string> ids = { "cmd_uri", "explorer_uri", "steam_applaunch", "steam_uri", "steam_then_uri", "direct_dota" };
            std::string current = normalized_sandbox_launch_backend();
            int index = 0;
            for (int i = 0; i < static_cast<int>(ids.size()); ++i)
            {
                if (ids[static_cast<size_t>(i)] == current)
                {
                    index = i;
                    break;
                }
            }

            index += delta;
            while (index < 0)
                index += static_cast<int>(ids.size());
            index %= static_cast<int>(ids.size());

            sandbox_launch_backend = ids[static_cast<size_t>(index)];
            log("[SANDBOX] Launch backend set to " + sandbox_launch_backend_label() + " (" + sandbox_launch_backend + ")");
            if (save_config_enabled)
                save_config();
        }

        void set_sandbox_launch_backend(const std::string& id)
        {
            if (!sandbox_backend_is_valid(id))
            {
                log("[SANDBOX] Invalid launch backend ignored: " + id);
                return;
            }

            sandbox_launch_backend = id;
            log("[SANDBOX] Launch backend set to " + sandbox_launch_backend_label() + " (" + sandbox_launch_backend + ")");
            if (save_config_enabled)
                save_config();
        }

        std::wstring steam_exe_from_profiles() const
        {
            std::string text = read_text_file_utf8(sandbox_profiles_path());
            std::string configured = json_string_value(text, "steam_exe");
            if (!configured.empty())
            {
                std::wstring wide = widen(configured);
                if (GetFileAttributesW(wide.c_str()) != INVALID_FILE_ATTRIBUTES)
                    return wide;
            }

            std::wstring dota2_path = dota2_exe_from_profiles();
            if (!dota2_path.empty())
            {
                std::wstring marker = L"\\steamapps\\common\\";
                std::wstring low = dota2_path;
                std::transform(low.begin(), low.end(), low.begin(), [](wchar_t c) { return (c >= L'A' && c <= L'Z') ? static_cast<wchar_t>(c + 32) : c; });
                size_t pos = low.find(marker);
                if (pos != std::wstring::npos)
                {
                    std::wstring steam_root = dota2_path.substr(0, pos);
                    std::wstring steam_from_dota = steam_root + L"\\steam.exe";
                    if (GetFileAttributesW(steam_from_dota.c_str()) != INVALID_FILE_ATTRIBUTES)
                        return steam_from_dota;
                }
            }

            const wchar_t* candidates[] = {
                L"C:\\Program Files (x86)\\Steam\\steam.exe",
                L"C:\\Program Files\\Steam\\steam.exe",
                L"D:\\Program Files (x86)\\Steam\\steam.exe",
                L"D:\\Program Files\\Steam\\steam.exe",
                L"E:\\Program Files (x86)\\Steam\\steam.exe",
                L"E:\\Program Files\\Steam\\steam.exe",
                L"F:\\Program Files (x86)\\Steam\\steam.exe",
                L"F:\\Program Files\\Steam\\steam.exe"
            };

            for (auto* candidate : candidates)
            {
                if (GetFileAttributesW(candidate) != INVALID_FILE_ATTRIBUTES)
                    return candidate;
            }
            return {};
        }

        static std::wstring windows_system_cmd_exe()
        {
            wchar_t dir[MAX_PATH * 4]{};
            UINT len = GetSystemDirectoryW(dir, static_cast<UINT>(std::size(dir)));
            if (len > 0 && len < std::size(dir))
            {
                std::wstring path(dir);
                path += L"\\cmd.exe";
                if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES)
                    return path;
            }
            return L"C:\\Windows\\System32\\cmd.exe";
        }

        static std::wstring windows_explorer_exe()
        {
            wchar_t dir[MAX_PATH * 4]{};
            UINT len = GetWindowsDirectoryW(dir, static_cast<UINT>(std::size(dir)));
            if (len > 0 && len < std::size(dir))
            {
                std::wstring path(dir);
                path += L"\\explorer.exe";
                if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES)
                    return path;
            }
            return L"C:\\Windows\\explorer.exe";
        }

        std::wstring dota2_exe_from_profiles() const
        {
            std::string text = read_text_file_utf8(sandbox_profiles_path());
            std::string configured = json_string_value(text, "dota2_exe");
            if (!configured.empty())
            {
                std::wstring wide = widen(configured);
                if (GetFileAttributesW(wide.c_str()) != INVALID_FILE_ATTRIBUTES)
                    return wide;
            }

            std::vector<std::wstring> roots = {
                L"C:\\Program Files (x86)\\Steam",
                L"C:\\Program Files\\Steam",
                L"D:\\Program Files (x86)\\Steam",
                L"D:\\Program Files\\Steam",
                L"E:\\Program Files (x86)\\Steam",
                L"E:\\Program Files\\Steam",
                L"F:\\Program Files (x86)\\Steam",
                L"F:\\Program Files\\Steam"
            };

            for (const auto& root : roots)
            {
                std::wstring candidate = root + L"\\steamapps\\common\\dota 2 beta\\game\\bin\\win64\\dota2.exe";
                if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES)
                    return candidate;
            }
            return {};
        }

        bool sandbox_window_already_running(const std::string& sandbox_name) const
        {
            for (const auto& w : windows)
            {
                if (sandbox_name_for_window(w, w.slot) == sandbox_name)
                    return true;
            }
            return false;
        }

        std::vector<std::string> sandbox_names_from_profiles() const
        {
            std::vector<std::string> names;
            std::string text = read_text_file_utf8(sandbox_profiles_path());
            const std::string needle = "\"sandbox_name\"";
            size_t pos = 0;
            while ((pos = text.find(needle, pos)) != std::string::npos)
            {
                size_t object_start = text.rfind('{', pos);
                size_t object_end = text.find('}', pos);
                std::string object_text;
                if (object_start != std::string::npos && object_end != std::string::npos && object_end > object_start)
                    object_text = text.substr(object_start, object_end - object_start + 1);

                std::string object_low = object_text;
                std::transform(object_low.begin(), object_low.end(), object_low.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (object_low.find("\"enabled\"") != std::string::npos && object_low.find("false") != std::string::npos)
                {
                    pos = object_end == std::string::npos ? pos + needle.size() : object_end + 1;
                    continue;
                }

                size_t colon = text.find(':', pos + needle.size());
                size_t quote = colon == std::string::npos ? std::string::npos : text.find('"', colon);
                if (quote == std::string::npos)
                    break;
                ++quote;
                std::string name;
                bool esc = false;
                for (size_t i = quote; i < text.size(); ++i)
                {
                    char c = text[i];
                    if (esc)
                    {
                        name += c;
                        esc = false;
                        continue;
                    }
                    if (c == '\\')
                    {
                        esc = true;
                        continue;
                    }
                    if (c == '"')
                    {
                        pos = i + 1;
                        break;
                    }
                    name += c;
                }
                if (!name.empty() && std::find(names.begin(), names.end(), name) == names.end())
                    names.push_back(name);
            }

            if (names.empty())
            {
                names.push_back("Dota2Ferma");
                names.push_back("Dota2FermaSakura");
                for (int i = 2; i <= 10; ++i)
                    names.push_back("Dota2FermaSakura" + std::to_string(i));
            }
            return names;
        }

        int sandbox_launch_count_from_profiles() const
        {
            if (sandbox_launch_count > 0)
                return sandbox_launch_count;
            std::string text = read_text_file_utf8(sandbox_profiles_path());
            int configured = json_int_value(text, "launch_count", 0);
            return configured > 0 ? configured : 3;
        }

        int normalized_sandbox_launch_count(int profile_count) const
        {
            if (profile_count <= 0)
                return 0;
            int requested = sandbox_launch_count_from_profiles();
            if (requested <= 0)
                requested = 3;
            return (std::max)(1, (std::min)(requested, profile_count));
        }

        std::vector<std::string> normalized_sandbox_launch_selection() const
        {
            std::vector<std::string> profiles = sandbox_names_from_profiles();
            std::vector<std::string> selected;

            for (const auto& profile : profiles)
            {
                if (std::find(sandbox_launch_selected.begin(), sandbox_launch_selected.end(), profile) != sandbox_launch_selected.end())
                    selected.push_back(profile);
            }

            if (!selected.empty())
                return selected;

            int count = normalized_sandbox_launch_count(static_cast<int>(profiles.size()));
            for (int i = 0; i < count && i < static_cast<int>(profiles.size()); ++i)
                selected.push_back(profiles[static_cast<size_t>(i)]);
            return selected;
        }

        std::vector<std::string> active_sandbox_names_for_launch() const
        {
            return normalized_sandbox_launch_selection();
        }

        bool is_sandbox_launch_profile_selected(const std::string& name) const
        {
            std::vector<std::string> selected = normalized_sandbox_launch_selection();
            return std::find(selected.begin(), selected.end(), name) != selected.end();
        }

        void normalize_saved_sandbox_selection()
        {
            std::vector<std::string> profiles = sandbox_names_from_profiles();
            std::vector<std::string> normalized;
            for (const auto& profile : profiles)
            {
                if (std::find(sandbox_launch_selected.begin(), sandbox_launch_selected.end(), profile) != sandbox_launch_selected.end())
                    normalized.push_back(profile);
            }
            sandbox_launch_selected = normalized;
            sandbox_launch_count = static_cast<int>(sandbox_launch_selected.size());
        }

        void set_sandbox_launch_first_count(int count, const std::string& reason)
        {
            std::vector<std::string> profiles = sandbox_names_from_profiles();
            int max_count = static_cast<int>(profiles.size());
            if (max_count <= 0)
                max_count = 1;
            sandbox_launch_count = (std::max)(1, (std::min)(max_count, count));
            sandbox_launch_selected.clear();
            for (int i = 0; i < sandbox_launch_count && i < static_cast<int>(profiles.size()); ++i)
                sandbox_launch_selected.push_back(profiles[static_cast<size_t>(i)]);
            log("[SANDBOX] Launch selection set to " + std::to_string(sandbox_launch_selected.size()) + "/" + std::to_string(max_count) + (reason.empty() ? std::string() : (" via " + reason)));
            if (save_config_enabled)
                save_config();
        }

        void adjust_sandbox_launch_count(int delta)
        {
            int current = static_cast<int>(active_sandbox_names_for_launch().size());
            set_sandbox_launch_first_count(current + delta, "count buttons");
        }

        void set_sandbox_launch_count(int count)
        {
            set_sandbox_launch_first_count(count, "preset");
        }

        void set_sandbox_launch_all_profiles()
        {
            std::vector<std::string> profiles = sandbox_names_from_profiles();
            sandbox_launch_selected = profiles;
            sandbox_launch_count = static_cast<int>(sandbox_launch_selected.size());
            log("[SANDBOX] Launch selection set to all profiles: " + std::to_string(sandbox_launch_count));
            if (save_config_enabled)
                save_config();
        }

        void toggle_sandbox_launch_profile(const std::string& name)
        {
            std::vector<std::string> selected = normalized_sandbox_launch_selection();
            auto it = std::find(selected.begin(), selected.end(), name);
            bool enabled = it == selected.end();
            if (enabled)
            {
                selected.push_back(name);
            }
            else
            {
                if (selected.size() <= 1)
                {
                    log("[SANDBOX] At least one sandbox profile must stay selected.");
                    return;
                }
                selected.erase(it);
            }

            std::vector<std::string> profiles = sandbox_names_from_profiles();
            std::vector<std::string> ordered;
            for (const auto& profile : profiles)
            {
                if (std::find(selected.begin(), selected.end(), profile) != selected.end())
                    ordered.push_back(profile);
            }
            sandbox_launch_selected = ordered;
            sandbox_launch_count = static_cast<int>(sandbox_launch_selected.size());
            log("[SANDBOX] " + std::string(enabled ? "Selected " : "Unselected ") + name + ". Selected=" + std::to_string(sandbox_launch_count) + "/" + std::to_string(profiles.size()));
            if (save_config_enabled)
                save_config();
        }

        static bool string_list_contains(const std::vector<std::string>& values, const std::string& value)
        {
            return std::find(values.begin(), values.end(), value) != values.end();
        }

        int scan_dota_windows_internal(const std::vector<std::string>& allowed_boxes = {}, bool profile_only = true)
        {
            windows.clear();
            last_window_scan_skipped_unknown = 0;
            last_window_scan_skipped_outside_target = 0;
            last_window_scan_raw_candidates = 0;

            struct ScanContext
            {
                RuntimeState* self{};
                std::vector<std::string> allowed_boxes;
                std::vector<std::string> profile_boxes;
                bool profile_only{};
            } ctx;

            ctx.self = this;
            ctx.allowed_boxes = allowed_boxes;
            ctx.profile_boxes = sandbox_names_from_profiles();
            ctx.profile_only = profile_only;

            EnumWindows([](HWND hwnd, LPARAM lparam) -> BOOL
            {
                auto* ctx = reinterpret_cast<ScanContext*>(lparam);
                auto* self = ctx->self;
                std::wstring title;
                DWORD pid = 0;
                RECT rect{};
                if (!is_dota_candidate_window(hwnd, title, pid, rect))
                    return TRUE;

                ++self->last_window_scan_raw_candidates;

                std::string process_path = narrow(get_process_path_or_name(pid));
                std::string detected_box = self->sandbox_name_from_process_path(process_path);

                if (!ctx->allowed_boxes.empty())
                {
                    if (detected_box.empty() || !RuntimeState::string_list_contains(ctx->allowed_boxes, detected_box))
                    {
                        ++self->last_window_scan_skipped_outside_target;
                        return TRUE;
                    }
                }
                else if (ctx->profile_only)
                {
                    if (detected_box.empty() || !RuntimeState::string_list_contains(ctx->profile_boxes, detected_box))
                    {
                        ++self->last_window_scan_skipped_unknown;
                        return TRUE;
                    }
                }

                DotaWindowInfo info;
                info.hwnd = hwnd;
                info.pid = pid;
                info.title = narrow(title);
                info.process_path = process_path;
                info.rect = rect;
                info.responsive = is_responsive(hwnd);
                info.slot = static_cast<int>(self->windows.size()) + 1;
                self->windows.push_back(info);
                return TRUE;
            }, reinterpret_cast<LPARAM>(&ctx));

            sort_windows_for_slots();
            assign_random_party_roles();
            ensure_demo_accounts_from_windows();
            return static_cast<int>(windows.size());
        }

        int count_running_dota_processes() const
        {
            int count = 0;
            HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapshot == INVALID_HANDLE_VALUE)
                return 0;

            PROCESSENTRY32W entry{};
            entry.dwSize = sizeof(entry);
            if (Process32FirstW(snapshot, &entry))
            {
                do
                {
                    std::wstring exe = lowercase(std::wstring(entry.szExeFile));
                    if (exe == L"dota2.exe")
                        ++count;
                }
                while (Process32NextW(snapshot, &entry));
            }

            CloseHandle(snapshot);
            return count;
        }

        int wait_for_dota_windows_after_launch(int expected_count)
        {
            if (expected_count <= 0)
                return scan_dota_windows_internal();

            DWORD started = GetTickCount();
            DWORD timeout_ms = static_cast<DWORD>((std::max)(5, sandbox_launch_wait_seconds)) * 1000UL;
            int last_found = 0;

            while (GetTickCount() - started < timeout_ms)
            {
                last_found = scan_dota_windows_internal();
                if (last_found >= expected_count)
                    break;
                Sleep(1500);
            }

            int process_found = count_running_dota_processes();
            log("[WINDOWS] Post-launch scan: hwnd_found=" + std::to_string(last_found) + "/" + std::to_string(expected_count) + ", process_found=" + std::to_string(process_found) + " Dota processes.");
            if (last_found == 0 && process_found > 0)
                log("[WINDOWS] Dota process exists, but top-level HWND was not detected yet. Press Verify windows after the game finishes loading.");
            for (const auto& w : windows)
                log("[WINDOWS] Slot " + std::to_string(w.slot) + " [" + party_role_label_for_window(w, w.slot) + "]: hwnd=" + hwnd_to_hex(w.hwnd) + " sandbox=" + sandbox_name_for_window(w, w.slot) + " pid=" + std::to_string(w.pid));
            return last_found;
        }

        int verify_sandbox_launch_windows()
        {
            std::vector<std::string> boxes = active_sandbox_names_for_launch();
            int expected = static_cast<int>(boxes.size());
            int found = scan_dota_windows_internal(boxes, true);
            int process_found = count_running_dota_processes();
            log("[WINDOWS] Verify scan: active_hwnd=" + std::to_string(found) + "/" + std::to_string(expected) + ", process_found=" + std::to_string(process_found) + " Dota processes, skipped_extra_hwnd=" + std::to_string(last_window_scan_skipped_outside_target) + ".");
            if (found == 0 && process_found > 0)
                log("[WINDOWS] Dota is running, but target sandbox HWND was not detected yet. Use Find Windows after the main menu is visible.");
            if (last_window_scan_skipped_outside_target > 0)
                log("[WINDOWS] Ignored " + std::to_string(last_window_scan_skipped_outside_target) + " Dota HWND outside selected launch profiles, so stale/extra windows cannot become leader.");
            for (const auto& w : windows)
                log("[WINDOWS] Slot " + std::to_string(w.slot) + " [" + party_role_label_for_window(w, w.slot) + "]: hwnd=" + hwnd_to_hex(w.hwnd) + " sandbox=" + sandbox_name_for_window(w, w.slot) + " pid=" + std::to_string(w.pid));
            last_launch_ok = expected > 0 ? (found >= expected) : (found > 0);
            return found;
        }

        bool start_sandbox_command_line(const std::wstring& command, const std::string& label, int slot, const std::string& box_name, DWORD extra_delay_ms = 0)
        {
            log("[SANDBOX] Launch Slot " + std::to_string(slot) + ": sandbox=" + box_name + " backend=" + label + " command=" + narrow(command));

            STARTUPINFOW si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            std::vector<wchar_t> cmd(command.begin(), command.end());
            cmd.push_back(L'\0');

            if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
            {
                log("[SANDBOX] Command started Slot " + std::to_string(slot) + ": sandbox=" + box_name + " backend=" + label + " launcher_pid=" + std::to_string(pi.dwProcessId));
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                DWORD delay = extra_delay_ms > 0 ? extra_delay_ms : static_cast<DWORD>(sandbox_launch_delay_ms);
                if (delay > 0)
                    Sleep(delay);
                return true;
            }

            log("[SANDBOX] Failed Slot " + std::to_string(slot) + ": sandbox=" + box_name + " backend=" + label + " error=" + std::to_string(GetLastError()));
            return false;
        }

        std::vector<std::pair<std::string, std::wstring>> sandbox_commands_for_backend(
            const std::string& backend,
            const std::wstring& start_exe,
            const std::wstring& box,
            const std::wstring& uri,
            const std::wstring& cmd_exe,
            const std::wstring& explorer_exe,
            const std::wstring& steam_exe,
            const std::wstring& dota2_exe) const
        {
            std::vector<std::pair<std::string, std::wstring>> commands;
            auto sbie_prefix = [&]() -> std::wstring
            {
                return L"\"" + start_exe + L"\" /box:" + box + L" ";
            };

            if (backend == "explorer_uri")
            {
                commands.push_back({ "explorer_uri", sbie_prefix() + L"\"" + explorer_exe + L"\" \"" + uri + L"\"" });
            }
            else if (backend == "steam_applaunch")
            {
                if (!steam_exe.empty())
                    commands.push_back({ "steam_applaunch", sbie_prefix() + L"\"" + steam_exe + L"\" -applaunch 570" });
            }
            else if (backend == "steam_uri")
            {
                if (!steam_exe.empty())
                    commands.push_back({ "steam_uri", sbie_prefix() + L"\"" + steam_exe + L"\" \"" + uri + L"\"" });
            }
            else if (backend == "steam_then_uri")
            {
                if (!steam_exe.empty())
                    commands.push_back({ "steam_start", sbie_prefix() + L"\"" + steam_exe + L"\"" });
                commands.push_back({ "cmd_uri", sbie_prefix() + L"\"" + cmd_exe + L"\" /d /c start \"\" \"" + uri + L"\"" });
            }
            else if (backend == "direct_dota")
            {
                if (!dota2_exe.empty())
                    commands.push_back({ "direct_dota", sbie_prefix() + L"\"" + dota2_exe + L"\"" });
            }
            else
            {
                commands.push_back({ "cmd_uri", sbie_prefix() + L"\"" + cmd_exe + L"\" /d /c start \"\" \"" + uri + L"\"" });
            }

            return commands;
        }

        bool launch_all_sandboxie()
        {
            const wchar_t* candidates[] = {
                L"C:\\Program Files\\Sandboxie-Plus\\Start.exe",
                L"C:\\Program Files\\Sandboxie\\Start.exe",
                L"C:\\Program Files (x86)\\Sandboxie-Plus\\Start.exe",
                L"C:\\Program Files (x86)\\Sandboxie\\Start.exe"
            };

            std::wstring start_exe = sandboxie_start_exe_from_profiles();
            for (auto* candidate : candidates)
            {
                if (!start_exe.empty())
                    break;
                if (GetFileAttributesW(candidate) != INVALID_FILE_ATTRIBUTES)
                    start_exe = candidate;
            }

            if (start_exe.empty())
            {
                std::string checked = "";
                for (auto* candidate : candidates)
                {
                    if (!checked.empty()) checked += " | ";
                    checked += narrow(candidate);
                }
                log("[SANDBOX] Sandboxie Start.exe not found. checked=" + checked + " config=" + narrow(sandbox_profiles_path().wstring()));
                last_launch_ok = false;
                return false;
            }

            int existing_before = scan_dota_windows_internal();
            std::vector<std::string> all_boxes = sandbox_names_from_profiles();
            std::vector<std::string> boxes = active_sandbox_names_for_launch();
            std::string launch_uri = dota_launch_uri_from_profiles();
            std::string backend = normalized_sandbox_launch_backend();
            std::wstring cmd_exe = windows_system_cmd_exe();
            std::wstring explorer_exe = windows_explorer_exe();
            std::wstring steam_exe = steam_exe_from_profiles();
            std::wstring dota2_exe = dota2_exe_from_profiles();
            std::wstring uri = widen(launch_uri);
            int command_started = 0;
            int skipped_running = 0;
            int slot = 1;

            log("[SANDBOX] Launch requested: target=" + std::to_string(boxes.size()) + " profiles=" + std::to_string(all_boxes.size()) + " existing=" + std::to_string(existing_before));
            log("[SANDBOX] Launch backend: " + sandbox_launch_backend_label() + " (" + backend + ")");
            log("[SANDBOX] Paths: start=" + narrow(start_exe) + " steam=" + (steam_exe.empty() ? std::string("not_found") : narrow(steam_exe)) + " dota2=" + (dota2_exe.empty() ? std::string("not_found") : narrow(dota2_exe)));

            for (const auto& box_name : boxes)
            {
                if (sandbox_window_already_running(box_name))
                {
                    log("[SANDBOX] Skip Slot " + std::to_string(slot) + ": sandbox=" + box_name + " already has a running Dota window.");
                    ++skipped_running;
                    ++slot;
                    continue;
                }

                std::wstring box = widen(box_name);
                auto commands = sandbox_commands_for_backend(backend, start_exe, box, uri, cmd_exe, explorer_exe, steam_exe, dota2_exe);
                if (commands.empty())
                {
                    log("[SANDBOX] Failed Slot " + std::to_string(slot) + ": sandbox=" + box_name + " backend=" + backend + " cannot build command. Check steam_exe/dota2_exe paths.");
                    ++slot;
                    continue;
                }

                int command_index = 0;
                for (const auto& item : commands)
                {
                    DWORD extra_delay = 0;
                    if (backend == "steam_then_uri" && command_index == 0)
                        extra_delay = 4500;
                    if (start_sandbox_command_line(item.second, item.first, slot, box_name, extra_delay))
                        ++command_started;
                    ++command_index;
                }
                ++slot;
            }

            int found_now = scan_dota_windows_internal(boxes, true);
            int process_found_now = count_running_dota_processes();
            log("[WINDOWS] Immediate scan after launch commands: active_hwnd=" + std::to_string(found_now) + "/" + std::to_string(boxes.size()) + ", process_found=" + std::to_string(process_found_now) + " Dota processes, skipped_extra_hwnd=" + std::to_string(last_window_scan_skipped_outside_target) + ".");
            log("[SANDBOX] Launch commands sent: commands=" + std::to_string(command_started) + ", skipped=" + std::to_string(skipped_running) + ". Wait for Dota, then press Verify windows / Find windows. Extra/stale Dota windows are ignored for this launch.");
            last_launch_ok = command_started > 0 || skipped_running > 0;
            return last_launch_ok;
        }

        static bool is_dota_candidate_window(HWND hwnd, std::wstring& out_title, DWORD& out_pid, RECT& out_rect)
        {
            if (!IsWindow(hwnd) || !IsWindowVisible(hwnd))
                return false;

            wchar_t title_buffer[512]{};
            GetWindowTextW(hwnd, title_buffer, static_cast<int>(std::size(title_buffer)));
            out_title = title_buffer;

            GetWindowThreadProcessId(hwnd, &out_pid);
            if (!out_pid)
                return false;

            RECT rc{};
            if (!GetWindowRect(hwnd, &rc))
                return false;
            if ((rc.right - rc.left) < 160 || (rc.bottom - rc.top) < 120)
                return false;
            out_rect = rc;

            std::wstring process_path = lowercase(get_process_path_or_name(out_pid));
            std::wstring process_name = lowercase(get_process_image_name_from_snapshot(out_pid));
            std::wstring class_name = lowercase(get_window_class_name(hwnd));
            std::wstring title_lower = lowercase(out_title);

            bool title_match = title_lower.find(L"dota") != std::wstring::npos;
            bool process_match = process_path.find(L"dota2.exe") != std::wstring::npos ||
                                 process_path.find(L"dota 2") != std::wstring::npos ||
                                 process_name == L"dota2.exe";
            bool class_match = class_name.find(L"sdl") != std::wstring::npos ||
                               class_name.find(L"dota") != std::wstring::npos ||
                               class_name.find(L"valve") != std::wstring::npos;

            return title_match || process_match || (class_match && process_name == L"dota2.exe");
        }

        static int trailing_number_score(const std::string& value)
        {
            int end = static_cast<int>(value.size()) - 1;
            while (end >= 0 && !std::isdigit(static_cast<unsigned char>(value[end])))
                --end;
            if (end < 0)
                return 999999;

            int start = end;
            while (start >= 0 && std::isdigit(static_cast<unsigned char>(value[start])))
                --start;
            ++start;

            try { return std::stoi(value.substr(start, end - start + 1)); }
            catch (...) { return 999999; }
        }

        void sort_windows_for_slots()
        {
            std::sort(windows.begin(), windows.end(), [&](const DotaWindowInfo& a, const DotaWindowInfo& b)
            {
                std::string sa = sandbox_name_from_process_path(a.process_path);
                std::string sb = sandbox_name_from_process_path(b.process_path);

                int na = trailing_number_score(sa);
                int nb = trailing_number_score(sb);
                if (na != nb)
                    return na < nb;
                if (sa != sb)
                    return sa < sb;
                if (a.pid != b.pid)
                    return a.pid < b.pid;
                return reinterpret_cast<uintptr_t>(a.hwnd) < reinterpret_cast<uintptr_t>(b.hwnd);
            });

            for (size_t i = 0; i < windows.size(); ++i)
                windows[i].slot = static_cast<int>(i) + 1;
        }

        void find_dota_windows()
        {
            int found = scan_dota_windows_internal({}, true);
            int process_found = count_running_dota_processes();
            log("[WINDOWS] Find Windows completed. Dota windows found: " + std::to_string(found) + ", processes=" + std::to_string(process_found) + ", skipped_unknown=" + std::to_string(last_window_scan_skipped_unknown) + ", parties=" + std::to_string(party_count_from_windows()));
            if (found == 0 && process_found > 0)
                log("[WINDOWS] Dota process is running, but no configured Sandboxie HWND was detected yet. Window may still be loading or outside configured profiles.");
            if (last_window_scan_skipped_unknown > 0)
                log("[WINDOWS] Ignored " + std::to_string(last_window_scan_skipped_unknown) + " Dota HWND without configured sandbox profile to prevent fake DotaBox slots/leaders.");
            for (const auto& w : windows)
                log("[WINDOWS] Slot " + std::to_string(w.slot) + " [" + party_role_label_for_window(w, w.slot) + "]: hwnd=" + hwnd_to_hex(w.hwnd) + " sandbox=" + sandbox_name_for_window(w, w.slot) + " pid=" + std::to_string(w.pid));
        }

        static bool is_responsive(HWND hwnd)
        {
            DWORD_PTR result = 0;
            return SendMessageTimeoutW(hwnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 250, &result) != 0;
        }

        void arrange_2x5()
        {
            if (windows.empty())
                find_dota_windows();
            if (windows.empty())
            {
                log("Arrange 2x5 skipped: no Dota windows found.");
                return;
            }

            RECT work{};
            SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
            const int cols = 5;
            const int rows = 2;
            const int gap = 4;
            const int total_w = work.right - work.left;
            const int total_h = work.bottom - work.top;
            const int cell_w = (total_w - gap * (cols - 1)) / cols;
            const int cell_h = (total_h - gap * (rows - 1)) / rows;

            for (size_t i = 0; i < windows.size() && i < 10; ++i)
            {
                int col = static_cast<int>(i) % cols;
                int row = static_cast<int>(i) / cols;
                int x = work.left + col * (cell_w + gap);
                int y = work.top + row * (cell_h + gap);
                MoveWindow(windows[i].hwnd, x, y, cell_w, cell_h, TRUE);
            }
            log("Arrange 2x5 requested for " + std::to_string((std::min)(windows.size(), size_t(10))) + " windows.");
            find_dota_windows();
        }

        static void tap_key(WORD vk)
        {
            INPUT inputs[2]{};
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = vk;
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = vk;
            inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(2, inputs, sizeof(INPUT));
        }

        static void send_unicode_text(const std::wstring& text)
        {
            for (wchar_t ch : text)
            {
                INPUT inputs[2]{};
                inputs[0].type = INPUT_KEYBOARD;
                inputs[0].ki.wScan = ch;
                inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
                inputs[1].type = INPUT_KEYBOARD;
                inputs[1].ki.wScan = ch;
                inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
                SendInput(2, inputs, sizeof(INPUT));
            }
        }

        void open_console_all()
        {
            if (windows.empty())
                find_dota_windows();

            if (windows.empty())
            {
                log("[CORE] Open Console All skipped: no Dota windows found.");
                return;
            }

            if (dota_console_open_hint)
            {
                log("[CORE] Open Console All skipped: console is already marked open. This prevents accidentally closing it with toggleconsole.");
                return;
            }

            for (const auto& w : windows)
            {
                SetForegroundWindow(w.hwnd);
                Sleep(160);
                tap_key(console_vk());
                Sleep(180);
            }
            dota_console_open_hint = true;
            log("[CORE] Open Console All sent " + console_key_name() + " to " + std::to_string(windows.size()) + " windows.");
        }

        void ensure_dota_console_open(HWND hwnd)
        {
            SetForegroundWindow(hwnd);
            Sleep(160);
            if (!dota_console_open_hint)
            {
                tap_key(console_vk());
                Sleep(240);
                dota_console_open_hint = true;
                log("[CORE] Console opened before sending commands: " + console_key_name());
            }
        }

        void send_console_command_text(const std::string& command)
        {
            send_unicode_text(widen(command));
            tap_key(VK_RETURN);
            Sleep(160);
        }

        void execute_console_command(HWND hwnd, const std::string& command)
        {
            if (!IsWindow(hwnd))
                return;
            ensure_dota_console_open(hwnd);
            send_console_command_text(command);
        }

        void execute_console_command_batch(HWND hwnd, const std::vector<std::string>& commands)
        {
            if (!IsWindow(hwnd))
                return;
            ensure_dota_console_open(hwnd);
            for (const auto& command : commands)
                send_console_command_text(command);
        }

        static uint64_t parse_number_after_key(const std::string& text, const std::string& key)
        {
            size_t pos = text.find(key);
            if (pos == std::string::npos)
                return 0;
            pos += key.size();
            while (pos < text.size() && !std::isdigit(static_cast<unsigned char>(text[pos])))
                ++pos;
            size_t start = pos;
            while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos])))
                ++pos;
            if (pos == start)
                return 0;
            try { return std::stoull(text.substr(start, pos - start)); }
            catch (...) { return 0; }
        }

        static uint32_t parse_account_id(const std::string& text)
        {
            uint64_t id = parse_number_after_key(text, "account_id:");
            if (!id)
                return 0;
            return static_cast<uint32_t>(id);
        }

        std::string account_role_for_slot(int slot) const
        {
            if (slot <= 1)
                return "LEADER";
            std::ostringstream ss;
            ss << "BOT " << std::setw(2) << std::setfill('0') << slot;
            return ss.str();
        }

        std::string role_label_for_window(const DotaWindowInfo& w, int fallback_slot) const
        {
            if (!w.role.empty())
                return w.role;
            return account_role_for_slot(fallback_slot);
        }

        std::string party_role_label_for_window(const DotaWindowInfo& w, int fallback_slot) const
        {
            int party = w.party > 0 ? w.party : ((fallback_slot - 1) / 5 + 1);
            return "P" + std::to_string(party) + " " + role_label_for_window(w, fallback_slot);
        }

        void assign_random_party_roles()
        {
            if (windows.empty())
                return;

            std::random_device rd;
            std::mt19937 rng(rd());

            const int party_size = 5;
            for (size_t start = 0; start < windows.size(); start += party_size)
            {
                size_t end = (std::min)(start + static_cast<size_t>(party_size), windows.size());
                int party = static_cast<int>(start / party_size) + 1;
                std::uniform_int_distribution<size_t> pick(start, end - 1);
                size_t leader_index = pick(rng);

                int bot_number = 2;
                for (size_t i = start; i < end; ++i)
                {
                    windows[i].party = party;
                    if (i == leader_index)
                    {
                        windows[i].role = "LEADER";
                    }
                    else
                    {
                        std::ostringstream ss;
                        ss << "BOT " << std::setw(2) << std::setfill('0') << bot_number++;
                        windows[i].role = ss.str();
                    }
                }
            }
        }

        int party_count_from_windows() const
        {
            if (windows.empty())
                return 0;
            return static_cast<int>((windows.size() + 4) / 5);
        }

        void build_party_model()
        {
            if (windows.empty())
                find_dota_windows();
            if (windows.empty())
            {
                log("[PARTY] Build skipped: no Dota windows found.");
                return;
            }

            ensure_demo_accounts_from_windows();

            int parties = party_count_from_windows();
            log("[PARTY] Build completed: windows=" + std::to_string(windows.size()) + " parties=" + std::to_string(parties) + " party_size=5 leaders=preserved_from_windows");
            for (int p = 1; p <= parties; ++p)
            {
                std::string leader;
                std::vector<std::string> bots;
                for (const auto& a : accounts)
                {
                    if (a.party != p)
                        continue;
                    std::string id = a.dota_id ? std::to_string(a.dota_id) : a.sandbox_name;
                    if (a.role == "LEADER")
                        leader = id;
                    else
                        bots.push_back(id);
                }

                std::string line = "[PARTY] Party " + std::to_string(p) + " leader=" + (leader.empty() ? "unknown" : leader) + " bots=";
                for (size_t i = 0; i < bots.size(); ++i)
                {
                    if (i) line += ",";
                    line += bots[i];
                }
                log(line);
            }
        }

        std::string party_ids_text() const
        {
            std::ostringstream out;
            int last_party = 0;
            std::vector<BotAccountInfo> sorted = accounts;
            std::sort(sorted.begin(), sorted.end(), [](const BotAccountInfo& a, const BotAccountInfo& b)
            {
                if (a.party != b.party) return a.party < b.party;
                if (a.role == "LEADER" && b.role != "LEADER") return true;
                if (a.role != "LEADER" && b.role == "LEADER") return false;
                return a.slot < b.slot;
            });

            for (const auto& a : sorted)
            {
                if (!a.dota_id)
                    continue;
                if (a.party != last_party)
                {
                    if (last_party != 0)
                        out << "\r\n";
                    out << "Party " << a.party << "\r\n";
                    last_party = a.party;
                }
                out << a.dota_id << "\r\n";
            }
            return out.str();
        }

        bool set_clipboard_text(const std::string& text)
        {
            if (!OpenClipboard(nullptr))
                return false;
            EmptyClipboard();

            std::wstring wide = widen(text);
            SIZE_T bytes = (wide.size() + 1) * sizeof(wchar_t);
            HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
            if (!mem)
            {
                CloseClipboard();
                return false;
            }

            void* dst = GlobalLock(mem);
            if (!dst)
            {
                GlobalFree(mem);
                CloseClipboard();
                return false;
            }
            memcpy(dst, wide.c_str(), bytes);
            GlobalUnlock(mem);

            if (!SetClipboardData(CF_UNICODETEXT, mem))
            {
                GlobalFree(mem);
                CloseClipboard();
                return false;
            }

            CloseClipboard();
            return true;
        }

        void copy_party_ids_to_clipboard()
        {
            std::string text = party_ids_text();
            if (text.empty())
            {
                log("[PARTY] Copy IDs skipped: no identified account IDs yet.");
                return;
            }

            if (set_clipboard_text(text))
                log("[PARTY] Copied party Dota IDs to clipboard.");
            else
                log("[PARTY] Copy IDs failed: clipboard unavailable.");
        }

        static void replace_all_inplace(std::string& value, const std::string& from, const std::string& to)
        {
            if (from.empty())
                return;
            size_t pos = 0;
            while ((pos = value.find(from, pos)) != std::string::npos)
            {
                value.replace(pos, from.size(), to);
                pos += to.size();
            }
        }

        std::string normalized_party_invite_backend() const
        {
            std::string id = party_invite_backend;
            std::transform(id.begin(), id.end(), id.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (id == "console") id = "console_template";
            if (id == "gc") id = "game_coordinator";
            if (id == "ui_automation") id = "game_coordinator";
            if (id == "manual" || id == "plan" || id == "dry_run" || id == "console_template" || id == "game_coordinator")
                return id;
            return "game_coordinator";
        }

        std::string party_invite_backend_label() const
        {
            std::string id = normalized_party_invite_backend();
            if (id == "console_template") return "console template";
            if (id == "game_coordinator") return "GC helper";
            if (id == "dry_run") return "dry run";
            if (id == "manual") return "manual";
            return "plan only";
        }

        void cycle_party_invite_backend(int delta)
        {
            std::vector<std::string> ids = { "game_coordinator", "dry_run", "plan", "console_template" };
            std::string current = normalized_party_invite_backend();
            int index = 0;
            for (int i = 0; i < static_cast<int>(ids.size()); ++i)
            {
                if (ids[static_cast<size_t>(i)] == current)
                {
                    index = i;
                    break;
                }
            }

            index += delta;
            while (index < 0)
                index += static_cast<int>(ids.size());
            index %= static_cast<int>(ids.size());

            party_invite_backend = ids[static_cast<size_t>(index)];
            log("[PARTY] Invite backend set to " + party_invite_backend_label() + " (" + party_invite_backend + ")");
            if (save_config_enabled)
                save_config();
        }

        void apply_account_template_values(std::string& command, const char* prefix, const BotAccountInfo& account) const
        {
            std::string p = prefix ? prefix : "";
            replace_all_inplace(command, "{" + p + "slot}", std::to_string(account.slot));
            replace_all_inplace(command, "{" + p + "sandbox}", account.sandbox_name);
            replace_all_inplace(command, "{" + p + "account_id}", std::to_string(account.dota_id));
            replace_all_inplace(command, "{" + p + "dota_id}", std::to_string(account.dota_id));
            replace_all_inplace(command, "{" + p + "steam64}", std::to_string(account.steamid64));
            replace_all_inplace(command, "{" + p + "steam3}", account.steam3);
        }

        std::string invite_command_from_template(const BotAccountInfo& leader, const BotAccountInfo& bot) const
        {
            std::string command = party_invite_command_template;
            apply_account_template_values(command, "leader_", leader);
            apply_account_template_values(command, "bot_", bot);
            apply_account_template_values(command, "", bot);
            return command;
        }

        std::string accept_command_from_template(const BotAccountInfo& leader, const BotAccountInfo& bot) const
        {
            std::string command = party_accept_command_template;
            apply_account_template_values(command, "leader_", leader);
            apply_account_template_values(command, "bot_", bot);
            apply_account_template_values(command, "", bot);
            return command;
        }

        std::string party_invite_plan_text()
        {
            if (windows.empty())
                find_dota_windows();
            if (accounts.empty())
                ensure_demo_accounts_from_windows();
            if (accounts.empty())
                return {};

            std::ostringstream out;
            int parties = party_count_from_windows();
            for (int p = 1; p <= parties; ++p)
            {
                const BotAccountInfo* leader = nullptr;
                std::vector<const BotAccountInfo*> bots;
                for (const auto& a : accounts)
                {
                    if (a.party != p)
                        continue;
                    if (a.role == "LEADER") leader = &a;
                    else bots.push_back(&a);
                }

                out << "Party " << p << "\r\n";
                if (leader)
                    out << "Leader: " << leader->dota_id << " | slot " << leader->slot << " | " << leader->sandbox_name << "\r\n";
                else
                    out << "Leader: unknown\r\n";

                out << "Bots:\r\n";
                for (const BotAccountInfo* bot : bots)
                {
                    if (!bot) continue;
                    out << "- " << bot->dota_id << " | slot " << bot->slot << " | " << bot->sandbox_name << "\r\n";
                }
                out << "\r\n";
            }
            return out.str();
        }
        void copy_invite_plan_to_clipboard()
        {
            std::string text = party_invite_plan_text();
            if (text.empty())
            {
                log("[PARTY] Copy invite plan skipped: no party plan yet.");
                return;
            }
            if (set_clipboard_text(text))
                log("[PARTY] Copied invite plan to clipboard.");
            else
                log("[PARTY] Copy invite plan failed: clipboard unavailable.");
        }


        static std::string json_escape(const std::string& value)
        {
            std::string out;
            out.reserve(value.size() + 16);
            for (unsigned char c : value)
            {
                switch (c)
                {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 0x20)
                    {
                        char buf[8]{};
                        sprintf_s(buf, "\\u%04x", static_cast<unsigned int>(c));
                        out += buf;
                    }
                    else
                    {
                        out += static_cast<char>(c);
                    }
                    break;
                }
            }
            return out;
        }

        std::filesystem::path module_path_dir() const
        {
            wchar_t path[MAX_PATH * 4]{};
            GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
            std::filesystem::path p(path);
            return p.parent_path();
        }

        std::filesystem::path resolve_repo_relative_path(const std::string& raw) const
        {
            std::filesystem::path input = widen(raw);
            if (input.is_absolute())
                return input;

            std::vector<std::filesystem::path> roots;
            std::filesystem::path mod = module_path_dir();
            roots.push_back(std::filesystem::current_path());
            roots.push_back(mod);
            std::filesystem::path walk = mod;
            for (int i = 0; i < 8 && !walk.empty(); ++i)
            {
                roots.push_back(walk);
                if (!walk.has_parent_path() || walk.parent_path() == walk)
                    break;
                walk = walk.parent_path();
            }

            for (const auto& root : roots)
            {
                std::filesystem::path candidate = root / input;
                std::error_code ec;
                if (std::filesystem::exists(candidate, ec))
                    return candidate;
            }

            if (!roots.empty())
                return roots.back() / input;
            return input;
        }

        std::filesystem::path resolve_repo_write_path(const std::string& raw) const
        {
            std::filesystem::path input = widen(raw);
            if (input.is_absolute())
                return input;

            std::filesystem::path mod = module_path_dir();
            std::filesystem::path walk = mod;
            for (int i = 0; i < 8 && !walk.empty(); ++i)
            {
                std::filesystem::path tools = walk / L"tools" / L"gc_helper";
                std::error_code ec;
                if (std::filesystem::exists(tools, ec))
                    return walk / input;
                if (!walk.has_parent_path() || walk.parent_path() == walk)
                    break;
                walk = walk.parent_path();
            }
            return mod / input;
        }

        bool write_gc_accounts_runtime_json(const std::filesystem::path& path)
        {
            ensure_gc_accounts_from_detected();
            if (accounts.empty()) ensure_demo_accounts_from_windows();
            if (accounts.empty())
            {
                log("[GC] Runtime accounts write skipped: no identified accounts.");
                return false;
            }
            int missing = 0;
            for (const auto& a : accounts)
            {
                if (!a.steamid64) continue;
                int idx = find_gc_account_index_by_steam64(a.steamid64);
                if (idx < 0 || !gc_account_complete(gc_accounts[static_cast<size_t>(idx)]))
                {
                    ++missing;
                    log("[GC] Missing credentials: slot=" + std::to_string(a.slot) + " sandbox=" + a.sandbox_name + " steam64=" + std::to_string(a.steamid64));
                }
            }
            if (missing > 0)
            {
                log("[GC] Fill GC Accounts tab first. Missing credentials=" + std::to_string(missing));
                return false;
            }
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            if (!out.is_open())
            {
                log("[GC] Runtime accounts write failed: " + narrow(path.wstring()));
                return false;
            }
            out << "{\n";
            out << "  \"schema\": 2,\n";
            out << "  \"source\": \"AFKbotLoader DPAPI runtime export\",\n";
            out << "  \"deleteAfterRead\": true,\n";
            out << "  \"options\": {\"inviteDelayMs\": 1500, \"acceptWindowMs\": 90000, \"connectTimeoutMs\": " << (gc_helper_timeout_seconds * 1000) << "},\n";
            out << "  \"accounts\": [\n";
            bool first = true;
            for (const auto& e : gc_accounts)
            {
                if (!gc_account_complete(e)) continue;
                if (!first) out << ",\n";
                first = false;
                out << "    {";
                out << "\"label\": \"" << json_escape(e.sandbox_name) << "\", ";
                out << "\"sandbox\": \"" << json_escape(e.sandbox_name) << "\", ";
                out << "\"steam64\": \"" << e.steamid64 << "\", ";
                out << "\"account_id\": " << e.dota_id << ", ";
                out << "\"accountName\": \"" << json_escape(gc_plain_account_name(e)) << "\", ";
                out << "\"password\": \"" << json_escape(gc_plain_password(e)) << "\"";
                std::string secret = gc_plain_shared_secret(e);
                if (!secret.empty()) out << ", \"sharedSecret\": \"" << json_escape(secret) << "\"";
                out << "}";
            }
            out << "\n  ]\n";
            out << "}\n";
            log("[GC] Runtime accounts exported for helper: " + narrow(path.wstring()));
            return true;
        }

        bool write_gc_party_plan_json(const std::filesystem::path& path)
        {
            if (accounts.empty())
                ensure_demo_accounts_from_windows();
            if (accounts.empty())
            {
                log("[GC] Plan write skipped: no accounts.");
                return false;
            }

            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);

            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            if (!out.is_open())
            {
                log("[GC] Plan write failed: " + narrow(path.wstring()));
                return false;
            }

            out << "{\n";
            out << "  \"schema\": 1,\n";
            out << "  \"source\": \"AFKbotLoader\",\n";
            out << "  \"parties\": [\n";

            int parties = party_count_from_windows();
            bool first_party = true;
            for (int p = 1; p <= parties; ++p)
            {
                const BotAccountInfo* leader = nullptr;
                std::vector<const BotAccountInfo*> bots;
                for (const auto& a : accounts)
                {
                    if (a.party != p)
                        continue;
                    if (a.role == "LEADER") leader = &a;
                    else bots.push_back(&a);
                }
                if (!leader || bots.empty())
                    continue;

                if (!first_party) out << ",\n";
                first_party = false;
                out << "    {\n";
                out << "      \"party\": " << p << ",\n";
                out << "      \"leader\": {\"slot\": " << leader->slot << ", \"sandbox\": \"" << json_escape(leader->sandbox_name) << "\", \"dota_id\": " << leader->dota_id << ", \"steam64\": \"" << leader->steamid64 << "\", \"steam3\": \"" << json_escape(leader->steam3) << "\"},\n";
                out << "      \"bots\": [";
                bool first_bot = true;
                for (const BotAccountInfo* bot : bots)
                {
                    if (!bot || !bot->dota_id || !bot->steamid64)
                        continue;
                    if (!first_bot) out << ", ";
                    first_bot = false;
                    out << "{\"slot\": " << bot->slot << ", \"sandbox\": \"" << json_escape(bot->sandbox_name) << "\", \"dota_id\": " << bot->dota_id << ", \"steam64\": \"" << bot->steamid64 << "\", \"steam3\": \"" << json_escape(bot->steam3) << "\"}";
                }
                out << "]\n";
                out << "    }";
            }

            out << "\n  ]\n";
            out << "}\n";
            log("[GC] Wrote party plan: " + narrow(path.wstring()));
            return true;
        }

        bool run_gc_helper(const std::string& action)
        {
            build_party_model();

            std::filesystem::path script = resolve_repo_relative_path(gc_helper_script_path);
            std::filesystem::path accounts_path = resolve_repo_write_path(gc_helper_accounts_runtime_path);
            std::filesystem::path plan_path = resolve_repo_write_path(gc_helper_plan_path);

            if (!std::filesystem::exists(script))
            {
                log("[GC] Helper script not found: " + narrow(script.wstring()));
                log("[GC] Expected tools/gc_helper/gc_party_helper.js. Patch files may not be copied completely.");
                return false;
            }
            if (!write_gc_accounts_runtime_json(accounts_path))
                return false;
            if (!write_gc_party_plan_json(plan_path))
                return false;

            std::wstring cmd = L"cmd.exe /d /c ";
            cmd += widen(windows_cmd_quote(gc_helper_node_exe));
            cmd += L" ";
            cmd += widen(windows_cmd_quote(narrow(script.wstring())));
            cmd += L" --action ";
            cmd += widen(action);
            cmd += L" --accounts ";
            cmd += widen(windows_cmd_quote(narrow(accounts_path.wstring())));
            cmd += L" --plan ";
            cmd += widen(windows_cmd_quote(narrow(plan_path.wstring())));
            cmd += L" --delete-accounts-after-read 1";

            STARTUPINFOW si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            std::wstring mutable_cmd = cmd;
            DWORD flags = gc_helper_detached_console ? CREATE_NEW_CONSOLE : 0;
            log("[GC] Starting helper action=" + action + " cmd=" + narrow(cmd));
            if (!CreateProcessW(nullptr, mutable_cmd.data(), nullptr, nullptr, FALSE, flags, nullptr, nullptr, &si, &pi))
            {
                DWORD err = GetLastError();
                log("[GC] Helper start failed. error=" + std::to_string(err) + " node=" + gc_helper_node_exe);
                return false;
            }

            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            log("[GC] Helper launched pid=" + std::to_string(pi.dwProcessId) + " action=" + action + ". Watch helper console/log for Steam Guard or GC errors.");
            return true;
        }

        void invite_party_placeholder()
        {
            if (windows.empty())
                find_dota_windows();
            if (accounts.empty())
                ensure_demo_accounts_from_windows();
            if (accounts.empty())
            {
                log("[PARTY] Invite skipped: no accounts/windows found.");
                return;
            }

            build_party_model();
            int parties = party_count_from_windows();
            int planned_invites = 0;
            int sent_invites = 0;
            std::string backend = normalized_party_invite_backend();
            bool can_send_console = !party_invite_command_template.empty() && backend == "console_template";
            bool can_send_gc = backend == "game_coordinator";

            log("[PARTY] Invite backend=" + party_invite_backend_label() + (can_send_console ? " invite_template=on" : " invite_template=off"));
            if (backend == "ui_automation")
            {
                log("[PARTY] UI automation backend removed: using GC helper instead.");
                backend = "game_coordinator";
                can_send_gc = true;
            }
            if (can_send_gc)
            {
                if (run_gc_helper("invite"))
                    log("[GC] Invite request handed to GC helper. This is the real backend; no window clicking is used.");
                return;
            }

            for (int p = 1; p <= parties; ++p)
            {
                const BotAccountInfo* leader = nullptr;
                std::vector<const BotAccountInfo*> bots;
                for (const auto& a : accounts)
                {
                    if (a.party != p)
                        continue;
                    if (a.role == "LEADER")
                        leader = &a;
                    else
                        bots.push_back(&a);
                }

                if (!leader || !leader->hwnd)
                {
                    log("[PARTY] Party " + std::to_string(p) + " invite skipped: leader window missing.");
                    continue;
                }

                log("[PARTY] Party " + std::to_string(p) + " leader slot=" + std::to_string(leader->slot) + " account_id=" + std::to_string(leader->dota_id) + " bots=" + std::to_string(bots.size()));

                for (const BotAccountInfo* bot : bots)
                {
                    if (!bot || !bot->dota_id)
                    {
                        log("[PARTY] Party " + std::to_string(p) + " invite skipped: bot ID missing.");
                        continue;
                    }

                    ++planned_invites;
                    log("[PARTY] Party " + std::to_string(p) + " invite plan: leader=" + std::to_string(leader->dota_id) + " -> bot=" + std::to_string(bot->dota_id) + " bot_slot=" + std::to_string(bot->slot));

                    if (can_send_console)
                    {
                        std::string command = invite_command_from_template(*leader, *bot);
                        if (!command.empty())
                        {
                            execute_console_command(leader->hwnd, command);
                            ++sent_invites;
                            log("[INVITE] Sent to leader slot=" + std::to_string(leader->slot) + " bot=" + std::to_string(bot->dota_id) + " command=" + command);
                            Sleep(350);
                        }
                    }
                }
            }

            if (!can_send_console)
                log("[PARTY] Invite dry-run completed: planned=" + std::to_string(planned_invites) + ". To send commands set party_invite_backend=console_template and party_invite_command_template=... in appsettings.cfg.");
            else
                log("[PARTY] Invite console commands completed: sent=" + std::to_string(sent_invites) + "/" + std::to_string(planned_invites));
        }

        void accept_invites_placeholder()
        {
            if (windows.empty())
                find_dota_windows();
            if (accounts.empty())
                ensure_demo_accounts_from_windows();

            std::string backend = normalized_party_invite_backend();
            bool can_send_console = !party_accept_command_template.empty() && backend == "console_template";
            if (backend == "ui_automation")
                backend = "game_coordinator";
            if (backend == "game_coordinator")
            {
                if (run_gc_helper("accept"))
                    log("[GC] Accept request handed to GC helper. This is the real backend; no window clicking is used.");
                return;
            }
            int planned = 0;
            int sent = 0;
            int parties = party_count_from_windows();

            for (int p = 1; p <= parties; ++p)
            {
                const BotAccountInfo* leader = nullptr;
                for (const auto& a : accounts)
                {
                    if (a.party == p && a.role == "LEADER")
                    {
                        leader = &a;
                        break;
                    }
                }

                for (const auto& a : accounts)
                {
                    if (a.party != p || a.role == "LEADER" || !a.hwnd)
                        continue;
                    ++planned;
                    if (can_send_console && leader)
                    {
                        std::string command = accept_command_from_template(*leader, a);
                        if (!command.empty())
                        {
                            execute_console_command(a.hwnd, command);
                            ++sent;
                            log("[ACCEPT] Sent to bot slot=" + std::to_string(a.slot) + " account_id=" + std::to_string(a.dota_id) + " command=" + command);
                            Sleep(300);
                        }
                    }
                    else
                    {
                        log("[PARTY] Accept plan: bot_slot=" + std::to_string(a.slot) + " account_id=" + std::to_string(a.dota_id) + (leader ? (" leader=" + std::to_string(leader->dota_id)) : " leader=unknown"));
                    }
                }
            }

            if (!can_send_console)
                log("[PARTY] Accept dry-run completed: planned=" + std::to_string(planned) + ". Set party_accept_command_template=... to send console accept commands.");
            else
                log("[PARTY] Accept console commands sent: " + std::to_string(sent) + "/" + std::to_string(planned));
        }

        void run_auto_pipeline(bool include_party_invites)
        {
            if (auto_pipeline_running)
            {
                log("[AUTO] Pipeline skipped: already running.");
                return;
            }

            auto_pipeline_running = true;
            std::vector<std::string> boxes = active_sandbox_names_for_launch();
            int expected = static_cast<int>(boxes.size());
            log("[AUTO] Starting pipeline: selected=" + std::to_string(expected) + (include_party_invites ? " with_party_invites=on" : " with_party_invites=off"));

            launch_all_sandboxie();

            int found = 0;
            const int max_attempts = 8;
            for (int attempt = 1; attempt <= max_attempts; ++attempt)
            {
                Sleep(attempt == 1 ? 8000 : 5000);
                found = verify_sandbox_launch_windows();
                log("[AUTO] Verify attempt " + std::to_string(attempt) + "/" + std::to_string(max_attempts) + ": found=" + std::to_string(found) + "/" + std::to_string(expected));
                if (expected > 0 && found >= expected)
                    break;
            }

            if (found <= 0)
            {
                log("[AUTO] No target Dota windows found. Pipeline stopped before account identification.");
                auto_pipeline_running = false;
                return;
            }

            identify_all();
            build_party_model();

            if (include_party_invites)
            {
                invite_party_placeholder();
                accept_invites_placeholder();
            }

            log("[AUTO] Pipeline completed: windows=" + std::to_string(windows.size()) + " accounts=" + std::to_string(accounts.size()) + " parties=" + std::to_string(party_count_from_windows()));
            auto_pipeline_running = false;
        }

        static uint32_t parse_first_steam3_id(const std::string& text)
        {
            size_t pos = text.find("[U:1:");
            if (pos == std::string::npos)
                return 0;
            pos += 5;
            size_t start = pos;
            while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos])))
                ++pos;
            if (pos == start)
                return 0;
            try { return static_cast<uint32_t>(std::stoul(text.substr(start, pos - start))); }
            catch (...) { return 0; }
        }

        static bool party_debug_has_no_party(const std::string& text)
        {
            return text.find("Failed to find party shared object") != std::string::npos ||
                   text.find("No Invite SO cache") != std::string::npos;
        }

        std::vector<std::filesystem::path> condump_candidate_paths(const DotaWindowInfo& w, const std::string& file_name) const
        {
            std::vector<std::filesystem::path> out;
            auto push_unique = [&](const std::filesystem::path& p)
            {
                if (p.empty())
                    return;
                for (const auto& existing : out)
                {
                    if (_wcsicmp(existing.wstring().c_str(), p.wstring().c_str()) == 0)
                        return;
                }
                out.push_back(p);
            };

            std::filesystem::path exe = widen(w.process_path);
            std::filesystem::path dir = exe.has_filename() ? exe.parent_path() : exe;
            std::filesystem::path game_dir;

            std::filesystem::path p = dir;
            for (int i = 0; i < 10 && !p.empty(); ++i)
            {
                if (_wcsicmp(p.filename().wstring().c_str(), L"game") == 0)
                {
                    game_dir = p;
                    break;
                }
                p = p.parent_path();
            }

            std::wstring wide_name = widen(file_name);

            if (!game_dir.empty())
            {
                push_unique(game_dir / L"dota" / wide_name);
                push_unique(game_dir / L"dota" / L"cfg" / wide_name);
                push_unique(game_dir / L"bin" / L"win64" / wide_name);
                push_unique(game_dir / wide_name);
            }

            p = dir;
            for (int i = 0; i < 10 && !p.empty(); ++i)
            {
                push_unique(p / wide_name);
                push_unique(p / L"cfg" / wide_name);
                push_unique(p / L"dota" / wide_name);
                push_unique(p / L"dota" / L"cfg" / wide_name);
                push_unique(p / L"game" / L"dota" / wide_name);
                push_unique(p / L"game" / L"dota" / L"cfg" / wide_name);
                p = p.parent_path();
            }

            return out;
        }

        std::filesystem::path guess_dota_condump_path(const DotaWindowInfo& w, const std::string& file_name) const
        {
            auto candidates = condump_candidate_paths(w, file_name);
            if (!candidates.empty())
                return candidates.front();
            return widen(file_name);
        }

        std::filesystem::path guess_dota_condump_path(const DotaWindowInfo& w, int slot) const
        {
            return guess_dota_condump_path(w, "afkbot_slot" + two_digits(slot) + ".txt");
        }

        static std::string read_text_file_utf8(const std::filesystem::path& path)
        {
            std::ifstream in(path, std::ios::binary);
            if (!in)
                return {};
            std::ostringstream ss;
            ss << in.rdbuf();
            return ss.str();
        }

        std::filesystem::path find_existing_condump_path(const DotaWindowInfo& w, const std::string& file_name) const
        {
            std::error_code ec;
            for (const auto& path : condump_candidate_paths(w, file_name))
            {
                if (std::filesystem::exists(path, ec) && std::filesystem::file_size(path, ec) > 0)
                    return path;
            }
            return {};
        }

        std::filesystem::path find_existing_condump_path_any(const DotaWindowInfo& w, const std::vector<std::string>& file_names) const
        {
            for (const auto& name : file_names)
            {
                auto path = find_existing_condump_path(w, name);
                if (!path.empty())
                    return path;
            }
            return {};
        }

        void remove_all_condump_candidates_any(const DotaWindowInfo& w, const std::vector<std::string>& file_names) const
        {
            for (const auto& name : file_names)
                remove_all_condump_candidates(w, name);
        }

        void remove_all_condump_candidates(const DotaWindowInfo& w, const std::string& file_name) const
        {
            std::error_code ec;
            for (const auto& path : condump_candidate_paths(w, file_name))
                std::filesystem::remove(path, ec);
        }

        std::vector<std::string> account_dump_names_for_slot(int slot) const
        {
            std::string base = "afkbot_account_slot" + two_digits(slot);
            return {
                base + ".txt",
                base,
                base + ".log",
                base + ".txt.txt",
                "cfg/" + base + ".txt",
                "cfg/" + base,
                "cfg/" + base + ".log",
                "cfg/" + base + ".txt.txt"
            };
        }

        void bind_account_identity(const DotaWindowInfo& w, int slot, uint32_t dota_id, const std::string& source)
        {
            if (!dota_id)
                return;

            uint64_t steam64 = dota_to_steam64(dota_id);
            std::string steam3 = steam3_from_dota(dota_id);
            std::string role = role_label_for_window(w, slot);
            int party = w.party > 0 ? w.party : ((slot - 1) / 5 + 1);
            std::string sandbox = sandbox_name_for_window(w, slot);

            if (static_cast<size_t>(slot - 1) < accounts.size())
            {
                accounts[slot - 1].slot = slot;
                accounts[slot - 1].party = party;
                accounts[slot - 1].role = role;
                accounts[slot - 1].sandbox_name = sandbox;
                accounts[slot - 1].dota_id = dota_id;
                accounts[slot - 1].steamid64 = steam64;
                accounts[slot - 1].steam3 = steam3;
                accounts[slot - 1].source = source;
                accounts[slot - 1].state = "READY";
                accounts[slot - 1].hwnd = w.hwnd;
            }

            log("[ACCOUNT] Slot " + std::to_string(slot) + " [P" + std::to_string(party) + " " + role + "]: account_id=" + std::to_string(dota_id) + " steam64=" + std::to_string(steam64) + " steam3=" + steam3 + " source=" + source);
        }

        static bool is_digits_only(const std::wstring& value)
        {
            if (value.empty())
                return false;
            for (wchar_t c : value)
            {
                if (!iswdigit(c))
                    return false;
            }
            return true;
        }

        static bool path_equal_ci(const std::filesystem::path& a, const std::filesystem::path& b)
        {
            std::wstring aw = a.wstring();
            std::wstring bw = b.wstring();
            std::replace(aw.begin(), aw.end(), L'/', L'\\');
            std::replace(bw.begin(), bw.end(), L'/', L'\\');
            aw = lowercase(aw);
            bw = lowercase(bw);
            while (!aw.empty() && (aw.back() == L'\\' || aw.back() == L'/')) aw.pop_back();
            while (!bw.empty() && (bw.back() == L'\\' || bw.back() == L'/')) bw.pop_back();
            return aw == bw;
        }

        static void push_unique_path(std::vector<std::filesystem::path>& out, const std::filesystem::path& path)
        {
            if (path.empty())
                return;
            for (const auto& existing : out)
            {
                if (path_equal_ci(existing, path))
                    return;
            }
            out.push_back(path);
        }

        std::filesystem::path sandbox_box_root_from_window(const DotaWindowInfo& w) const
        {
            std::wstring path = widen(w.process_path);
            std::replace(path.begin(), path.end(), L'/', L'\\');
            std::wstring low = lowercase(path);

            const std::wstring marker = L"\\sandbox\\";
            size_t sandbox_pos = low.find(marker);
            if (sandbox_pos == std::wstring::npos)
                return {};

            size_t user_start = sandbox_pos + marker.size();
            size_t user_end = low.find(L'\\', user_start);
            if (user_end == std::wstring::npos)
                return {};

            size_t box_start = user_end + 1;
            size_t box_end = low.find(L'\\', box_start);
            if (box_end == std::wstring::npos || box_end <= box_start)
                return {};

            return std::filesystem::path(path.substr(0, box_end));
        }

        std::filesystem::path steam_root_from_window(const DotaWindowInfo& w) const
        {
            std::filesystem::path exe = widen(w.process_path);
            std::filesystem::path p = exe.has_filename() ? exe.parent_path() : exe;

            for (int i = 0; i < 20 && !p.empty(); ++i)
            {
                std::wstring name = p.filename().wstring();
                if (_wcsicmp(name.c_str(), L"Steam") == 0)
                    return p;
                if (_wcsicmp(name.c_str(), L"steamapps") == 0)
                    return p.parent_path();
                p = p.parent_path();
            }

            return {};
        }

        bool steam_root_has_identity_files(const std::filesystem::path& steam_root) const
        {
            std::error_code ec;
            return std::filesystem::exists(steam_root / L"userdata", ec) ||
                   std::filesystem::exists(steam_root / L"config" / L"loginusers.vdf", ec);
        }

        void scan_steam_roots_under(const std::filesystem::path& base, std::vector<std::filesystem::path>& out, int max_depth = 8) const
        {
            std::error_code ec;
            if (base.empty() || !std::filesystem::exists(base, ec) || !std::filesystem::is_directory(base, ec))
                return;

            std::filesystem::recursive_directory_iterator it(base, std::filesystem::directory_options::skip_permission_denied, ec);
            std::filesystem::recursive_directory_iterator end;
            for (; it != end && !ec; it.increment(ec))
            {
                if (ec)
                    break;

                if (it.depth() > max_depth)
                {
                    it.disable_recursion_pending();
                    continue;
                }

                if (!it->is_directory(ec))
                    continue;

                std::wstring name = it->path().filename().wstring();
                if (_wcsicmp(name.c_str(), L"Steam") == 0)
                {
                    push_unique_path(out, it->path());
                    it.disable_recursion_pending();
                    continue;
                }

                if (_wcsicmp(name.c_str(), L"userdata") == 0)
                {
                    push_unique_path(out, it->path().parent_path());
                    it.disable_recursion_pending();
                    continue;
                }

                if (_wcsicmp(name.c_str(), L"config") == 0)
                {
                    if (std::filesystem::exists(it->path() / L"loginusers.vdf", ec))
                        push_unique_path(out, it->path().parent_path());
                    it.disable_recursion_pending();
                }
            }
        }

        std::filesystem::path host_path_from_sandbox_path(const std::filesystem::path& sandbox_path) const
        {
            std::wstring path = sandbox_path.wstring();
            if (path.empty())
                return {};

            std::replace(path.begin(), path.end(), L'/', L'\\');
            std::wstring low = lowercase(path);

            const std::wstring marker = L"\\drive\\";
            size_t pos = low.find(marker);
            if (pos == std::wstring::npos)
                return {};

            size_t drive_start = pos + marker.size();
            size_t drive_end = low.find(L'\\', drive_start);
            if (drive_end == std::wstring::npos || drive_end <= drive_start)
                return {};

            std::wstring drive_name = path.substr(drive_start, drive_end - drive_start);
            if (drive_name.size() != 1 || !iswalpha(drive_name[0]))
                return {};

            wchar_t drive_letter = static_cast<wchar_t>(std::towupper(drive_name[0]));
            std::wstring rest = path.substr(drive_end);
            std::wstring host;
            host.push_back(drive_letter);
            host += L":";
            host += rest;
            return std::filesystem::path(host);
        }

        std::filesystem::path host_drive_from_sandbox_drive(const std::filesystem::path& sandbox_drive) const
        {
            std::wstring name = sandbox_drive.filename().wstring();
            if (name.size() != 1 || !iswalpha(name[0]))
                return {};

            wchar_t drive_letter = static_cast<wchar_t>(std::towupper(name[0]));
            std::wstring host;
            host.push_back(drive_letter);
            host += L":\\";
            return std::filesystem::path(host);
        }

        void push_standard_steam_roots_for_drive(const std::filesystem::path& drive, std::vector<std::filesystem::path>& roots) const
        {
            if (drive.empty())
                return;

            push_unique_path(roots, drive / L"Program Files (x86)" / L"Steam");
            push_unique_path(roots, drive / L"Program Files" / L"Steam");
        }

        std::filesystem::path read_steam_root_from_registry(HKEY hive, const wchar_t* key_path, const wchar_t* value_name) const
        {
            HKEY key{};
            if (RegOpenKeyExW(hive, key_path, 0, KEY_READ | KEY_WOW64_32KEY, &key) != ERROR_SUCCESS &&
                RegOpenKeyExW(hive, key_path, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS &&
                RegOpenKeyExW(hive, key_path, 0, KEY_READ, &key) != ERROR_SUCCESS)
            {
                return {};
            }

            wchar_t buffer[MAX_PATH * 4]{};
            DWORD type = 0;
            DWORD bytes = sizeof(buffer);
            LONG result = RegQueryValueExW(key, value_name, nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bytes);
            RegCloseKey(key);

            if (result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || buffer[0] == 0)
                return {};

            if (type == REG_EXPAND_SZ)
            {
                wchar_t expanded[MAX_PATH * 4]{};
                if (ExpandEnvironmentStringsW(buffer, expanded, static_cast<DWORD>(std::size(expanded))) > 0)
                    return std::filesystem::path(expanded);
            }

            return std::filesystem::path(buffer);
        }

        void push_registry_steam_roots(std::vector<std::filesystem::path>& roots) const
        {
            const wchar_t* key_paths[] = {
                L"Software\\Valve\\Steam",
                L"SOFTWARE\\Valve\\Steam",
                L"SOFTWARE\\WOW6432Node\\Valve\\Steam"
            };

            const wchar_t* value_names[] = {
                L"SteamPath",
                L"InstallPath"
            };

            for (const wchar_t* key_path : key_paths)
            {
                for (const wchar_t* value_name : value_names)
                {
                    push_unique_path(roots, read_steam_root_from_registry(HKEY_CURRENT_USER, key_path, value_name));
                    push_unique_path(roots, read_steam_root_from_registry(HKEY_LOCAL_MACHINE, key_path, value_name));
                }
            }
        }

        void push_logical_drive_steam_roots(std::vector<std::filesystem::path>& roots) const
        {
            DWORD drives = GetLogicalDrives();
            for (int i = 0; i < 26; ++i)
            {
                if ((drives & (1u << i)) == 0)
                    continue;

                wchar_t root[] = { static_cast<wchar_t>(L'A' + i), L':', L'\\', L'\0' };
                push_standard_steam_roots_for_drive(std::filesystem::path(root), roots);
            }
        }

        std::string steam_root_candidate_summary(const DotaWindowInfo& w, size_t max_items = 10) const
        {
            auto roots = steam_root_candidates_from_window(w);
            if (roots.empty())
                return "none";

            std::ostringstream ss;
            size_t count = 0;
            for (const auto& root : roots)
            {
                if (count)
                    ss << " | ";
                ss << narrow(root.wstring());
                ++count;
                if (count >= max_items)
                    break;
            }
            if (roots.size() > max_items)
                ss << " | +" << (roots.size() - max_items) << " more";
            return ss.str();
        }

        std::vector<std::filesystem::path> steam_root_candidates_from_window(const DotaWindowInfo& w) const
        {
            std::vector<std::filesystem::path> sandbox_roots;
            std::vector<std::filesystem::path> host_roots;

            std::filesystem::path direct_root = steam_root_from_window(w);
            std::filesystem::path box_root = sandbox_box_root_from_window(w);

            push_unique_path(sandbox_roots, direct_root);

            if (!box_root.empty())
            {
                std::filesystem::path drive_root = box_root / L"drive";
                std::error_code ec;

                if (std::filesystem::exists(drive_root, ec) && std::filesystem::is_directory(drive_root, ec))
                {
                    for (const auto& drive_entry : std::filesystem::directory_iterator(drive_root, ec))
                    {
                        if (ec)
                            break;
                        if (!drive_entry.is_directory(ec))
                            continue;

                        const std::filesystem::path sandbox_drive = drive_entry.path();
                        push_standard_steam_roots_for_drive(sandbox_drive, sandbox_roots);
                        scan_steam_roots_under(sandbox_drive, sandbox_roots, 6);

                        std::filesystem::path host_drive = host_drive_from_sandbox_drive(sandbox_drive);
                        push_standard_steam_roots_for_drive(host_drive, host_roots);
                    }
                }

                scan_steam_roots_under(box_root, sandbox_roots, 9);
            }

            push_unique_path(host_roots, host_path_from_sandbox_path(direct_root));
            push_registry_steam_roots(host_roots);
            push_logical_drive_steam_roots(host_roots);

            std::vector<std::filesystem::path> roots;
            for (const auto& root : sandbox_roots)
                push_unique_path(roots, root);
            for (const auto& root : host_roots)
                push_unique_path(roots, root);

            std::vector<std::filesystem::path> existing_first;
            for (const auto& root : roots)
            {
                if (steam_root_has_identity_files(root))
                    push_unique_path(existing_first, root);
            }
            for (const auto& root : roots)
                push_unique_path(existing_first, root);

            return existing_first;
        }

        uint32_t parse_account_id_from_loginusers(const std::filesystem::path& steam_root, std::filesystem::path* source_path = nullptr) const
        {
            std::filesystem::path loginusers = steam_root / L"config" / L"loginusers.vdf";
            std::string text = read_text_file_utf8(loginusers);
            if (text.empty())
                return 0;

            struct Candidate
            {
                uint64_t steam64{};
                bool most_recent{};
            };

            std::vector<Candidate> candidates;
            size_t pos = 0;
            while ((pos = text.find("7656119", pos)) != std::string::npos)
            {
                size_t start = pos;
                size_t end = start;
                while (end < text.size() && std::isdigit(static_cast<unsigned char>(text[end])))
                    ++end;

                if (end - start >= 16)
                {
                    try
                    {
                        uint64_t steam64 = std::stoull(text.substr(start, end - start));
                        std::string block = text.substr(start, (std::min<size_t>)(700, text.size() - start));
                        bool recent = block.find("MostRecent") != std::string::npos && block.find("\"1\"") != std::string::npos;
                        candidates.push_back({ steam64, recent });
                    }
                    catch (...) {}
                }
                pos = end;
            }

            for (const auto& c : candidates)
            {
                if (c.most_recent)
                {
                    if (source_path) *source_path = loginusers;
                    return steam64_to_dota(c.steam64);
                }
            }

            if (!candidates.empty())
            {
                if (source_path) *source_path = loginusers;
                return steam64_to_dota(candidates.front().steam64);
            }

            return 0;
        }


        std::vector<uint32_t> userdata_account_ids_from_root(const std::filesystem::path& steam_root) const
        {
            std::vector<uint32_t> ids;
            std::filesystem::path userdata = steam_root / L"userdata";
            std::error_code ec;
            if (!std::filesystem::exists(userdata, ec) || !std::filesystem::is_directory(userdata, ec))
                return ids;

            for (const auto& entry : std::filesystem::directory_iterator(userdata, ec))
            {
                if (ec)
                    break;
                if (!entry.is_directory(ec))
                    continue;

                std::wstring folder = entry.path().filename().wstring();
                if (!is_digits_only(folder))
                    continue;

                uint64_t raw = 0;
                try { raw = std::stoull(folder); }
                catch (...) { continue; }
                if (raw == 0 || raw > 0xFFFFFFFFULL)
                    continue;

                uint32_t id = static_cast<uint32_t>(raw);
                if (std::find(ids.begin(), ids.end(), id) == ids.end())
                    ids.push_back(id);
            }

            return ids;
        }

        bool userdata_account_folder_exists(const std::filesystem::path& steam_root, uint32_t dota_id) const
        {
            if (!dota_id)
                return false;
            std::error_code ec;
            return std::filesystem::exists(steam_root / L"userdata" / std::to_wstring(dota_id), ec);
        }

        uint32_t parse_account_id_from_steam_userdata_root(const std::filesystem::path& steam_root, std::filesystem::path* source_path = nullptr) const
        {
            std::filesystem::path userdata = steam_root / L"userdata";
            std::error_code ec;
            if (!std::filesystem::exists(userdata, ec) || !std::filesystem::is_directory(userdata, ec))
                return 0;

            std::vector<uint32_t> userdata_ids = userdata_account_ids_from_root(steam_root);
            if (userdata_ids.size() == 1)
            {
                if (source_path) *source_path = userdata / std::to_wstring(userdata_ids.front());
                return userdata_ids.front();
            }

            if (userdata_ids.size() > 1)
            {
                std::filesystem::path loginusers_source;
                uint32_t loginusers_id = parse_account_id_from_loginusers(steam_root, &loginusers_source);
                if (loginusers_id && userdata_account_folder_exists(steam_root, loginusers_id))
                {
                    if (source_path) *source_path = loginusers_source.empty() ? (userdata / std::to_wstring(loginusers_id)) : loginusers_source;
                    return loginusers_id;
                }
            }

            uint32_t best_id = 0;
            int best_score = -1;
            std::filesystem::file_time_type best_time{};
            bool has_time = false;
            std::filesystem::path best_path;

            for (const auto& entry : std::filesystem::directory_iterator(userdata, ec))
            {
                if (ec)
                    break;
                if (!entry.is_directory(ec))
                    continue;

                std::wstring folder = entry.path().filename().wstring();
                if (!is_digits_only(folder))
                    continue;

                uint64_t raw = 0;
                try { raw = std::stoull(folder); }
                catch (...) { continue; }
                if (raw == 0 || raw > 0xFFFFFFFFULL)
                    continue;

                int score = 10;
                std::filesystem::path dota570 = entry.path() / L"570";
                std::filesystem::path dotaRemote = dota570 / L"remote";
                std::filesystem::path dotaCfg = dota570 / L"local";
                if (std::filesystem::exists(dota570, ec)) score += 1000;
                if (std::filesystem::exists(dotaRemote, ec)) score += 400;
                if (std::filesystem::exists(dotaCfg, ec)) score += 100;

                auto tpath = std::filesystem::exists(dota570, ec) ? dota570 : entry.path();
                auto time = std::filesystem::last_write_time(tpath, ec);
                bool better = false;
                if (score > best_score)
                    better = true;
                else if (score == best_score && (!has_time || (!ec && time > best_time)))
                    better = true;

                if (better)
                {
                    best_id = static_cast<uint32_t>(raw);
                    best_score = score;
                    best_path = entry.path();
                    if (!ec)
                    {
                        best_time = time;
                        has_time = true;
                    }
                }
            }

            if (best_id)
            {
                if (source_path) *source_path = best_path;
                return best_id;
            }

            return 0;
        }

        uint32_t parse_account_id_from_steam_userdata(const DotaWindowInfo& w, std::filesystem::path* source_path = nullptr) const
        {
            auto roots = steam_root_candidates_from_window(w);

            for (const auto& root : roots)
            {
                std::filesystem::path source;
                uint32_t id = parse_account_id_from_steam_userdata_root(root, &source);
                if (id)
                {
                    if (source_path) *source_path = source.empty() ? root : source;
                    return id;
                }
            }

            for (const auto& root : roots)
            {
                std::filesystem::path source;
                uint32_t id = parse_account_id_from_loginusers(root, &source);
                if (id)
                {
                    if (source_path) *source_path = source.empty() ? root : source;
                    return id;
                }
            }

            return 0;
        }

        bool bind_account_from_steam_filesystem(const DotaWindowInfo& w, int slot)
        {
            std::filesystem::path source;
            uint32_t dota_id = parse_account_id_from_steam_userdata(w, &source);
            if (!dota_id)
                return false;

            std::string src = "userdata";
            if (!source.empty())
            {
                std::string source_text = narrow(source.wstring());
                std::string source_low = source_text;
                std::transform(source_low.begin(), source_low.end(), source_low.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (source_low.find("loginusers.vdf") != std::string::npos)
                    src = "loginusers";
                else if (source_low.find("userdata") != std::string::npos)
                    src = "userdata";
                else
                    src = source_text;
            }
            bind_account_identity(w, slot, dota_id, src);
            return true;
        }

        void parse_account_debug_dump_for_window(const DotaWindowInfo& w, int slot)
        {
            auto names = account_dump_names_for_slot(slot);
            std::filesystem::path dump;
            std::string text;

            for (int attempt = 0; attempt < 24; ++attempt)
            {
                dump = find_existing_condump_path_any(w, names);
                if (!dump.empty())
                {
                    text = read_text_file_utf8(dump);
                    if (!text.empty())
                        break;
                }
                Sleep(250);
            }

            if (text.empty())
            {
                auto guessed = guess_dota_condump_path(w, names.front());
                log("[ACCOUNT] Slot " + std::to_string(slot) + " [" + account_role_for_slot(slot) + "]: console dump/log not found. First guess=" + narrow(guessed.wstring()));
                return;
            }

            uint32_t dota_id = parse_account_id(text);
            if (!dota_id)
            {
                log("Slot " + std::to_string(slot) + ": dump/log found but account_id is missing: " + narrow(dump.wstring()));
                if (slot - 1 >= 0 && static_cast<size_t>(slot - 1) < accounts.size())
                    accounts[slot - 1].state = "ID_NOT_FOUND";
                return;
            }

            bind_account_identity(w, slot, dota_id, "console");
        }

        void identify_accounts_all()
        {
            if (windows.empty())
                find_dota_windows();
            if (windows.empty())
            {
                log("[ACCOUNT] Account ID parse skipped: no Dota windows found.");
                return;
            }

            ensure_demo_accounts_from_windows();

            struct PendingConsoleParse
            {
                DotaWindowInfo window;
                int slot{};
            };
            std::vector<PendingConsoleParse> pending_console;

            int slot = 1;
            for (const auto& w : windows)
            {
                if (bind_account_from_steam_filesystem(w, slot))
                {
                    ++slot;
                    continue;
                }

                log("[ACCOUNT] Slot " + std::to_string(slot) + " [" + account_role_for_slot(slot) + "]: source=userdata/loginusers failed, checked=" + steam_root_candidate_summary(w) + ", trying source=console");

                std::string base = "afkbot_account_slot" + two_digits(slot);
                auto names = account_dump_names_for_slot(slot);
                remove_all_condump_candidates_any(w, names);

                execute_console_command_batch(w.hwnd, {
                    "clear",
                    "developer 1",
                    "con_logfile cfg/" + base + ".log",
                    "dota_game_account_client_debug",
                    "condump " + base,
                    "condump cfg/" + base,
                    "con_logfile \"\""
                });
                pending_console.push_back({ w, slot });
                ++slot;
            }

            if (!pending_console.empty())
            {
                Sleep(1800);
                for (const auto& item : pending_console)
                    parse_account_debug_dump_for_window(item.window, item.slot);
            }

            int resolved = 0;
            for (const auto& a : accounts)
                if (a.dota_id)
                    ++resolved;
            int total = static_cast<int>(accounts.size());
            if (resolved == total)
                log("[ACCOUNT] Identification completed: " + std::to_string(resolved) + "/" + std::to_string(total) + " accounts resolved.");
            else
                log("[ACCOUNT] Identification completed: " + std::to_string(resolved) + "/" + std::to_string(total) + " accounts resolved, " + std::to_string(total - resolved) + " failed.");
            log("[ACCOUNT] Account ID parse completed. account_id is used as Dota invite ID.");
        }

        void parse_party_debug_dump_for_window(const DotaWindowInfo& w, int slot)
        {
            std::filesystem::path dump = guess_dota_condump_path(w, slot);
            std::string text = read_text_file_utf8(dump);
            if (text.empty())
            {
                log("Slot " + std::to_string(slot) + ": condump not found yet: " + narrow(dump.wstring()));
                return;
            }

            if (party_debug_has_no_party(text))
            {
                log("Slot " + std::to_string(slot) + ": no party shared object. This means the account is not in a party yet.");
                if (slot - 1 >= 0 && static_cast<size_t>(slot - 1) < accounts.size())
                    accounts[slot - 1].state = "NO_PARTY";
                return;
            }

            uint32_t dota_id = parse_first_steam3_id(text);
            uint64_t leader64 = parse_number_after_key(text, "leader_id:");
            uint64_t member64 = parse_number_after_key(text, "member_ids:");
            uint64_t steam64 = leader64 ? leader64 : member64;
            if (!dota_id && steam64)
                dota_id = steam64_to_dota(steam64);
            if (!steam64 && dota_id)
                steam64 = dota_to_steam64(dota_id);

            if (dota_id || steam64)
            {
                if (static_cast<size_t>(slot - 1) < accounts.size())
                {
                    accounts[slot - 1].dota_id = dota_id;
                    accounts[slot - 1].steamid64 = steam64;
                    accounts[slot - 1].steam3 = dota_id ? steam3_from_dota(dota_id) : std::string();
                    accounts[slot - 1].state = "PARTY_PARSED";
                }
                log("Slot " + std::to_string(slot) + ": parsed party identity dota_id=" + std::to_string(dota_id) + " steam64=" + std::to_string(steam64));
            }
            else
            {
                log("Slot " + std::to_string(slot) + ": party dump read, but no Steam3/member id found.");
            }
        }

        void verify_party_all()
        {
            if (windows.empty())
                find_dota_windows();
            if (windows.empty())
            {
                log("Verify Party skipped: no Dota windows found.");
                return;
            }

            int slot = 1;
            for (const auto& w : windows)
            {
                execute_console_command_batch(w.hwnd, {
                    "dota_party_debug",
                    "condump afkbot_slot" + two_digits(slot) + ".txt"
                });
                ++slot;
            }

            ensure_demo_accounts_from_windows();
            Sleep(250);

            slot = 1;
            for (const auto& w : windows)
            {
                parse_party_debug_dump_for_window(w, slot);
                ++slot;
            }

            log("Verify Party completed. If console says 'Failed to find party shared object', create/join party first.");
        }

        void identify_all()
        {
            identify_accounts_all();
        }

        void start_ready_check_leader()
        {
            if (windows.empty())
                find_dota_windows();
            if (windows.empty())
            {
                log("Ready-check skipped: no leader window.");
                return;
            }
            auto it = std::find_if(windows.begin(), windows.end(), [](const DotaWindowInfo& w) { return w.role == "LEADER"; });
            if (it == windows.end())
                it = windows.begin();
            execute_console_command(it->hwnd, "dota_start_party_ready_check");
            log("Ready-check command sent to leader window: slot=" + std::to_string(it->slot) + " party=" + std::to_string(it->party));
        }

        void click_relative(HWND hwnd, float rx, float ry)
        {
            RECT rc{};
            if (!GetClientRect(hwnd, &rc))
                return;
            int x = static_cast<int>((rc.right - rc.left) * rx);
            int y = static_cast<int>((rc.bottom - rc.top) * ry);
            LPARAM lp = MAKELPARAM(x, y);
            PostMessageW(hwnd, WM_MOUSEMOVE, 0, lp);
            PostMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp);
            PostMessageW(hwnd, WM_LBUTTONUP, 0, lp);
        }

        void accept_ready_all()
        {
            if (windows.empty())
                find_dota_windows();
            for (const auto& w : windows)
                click_relative(w.hwnd, ready_yes_x, ready_yes_y);
            ++ready_accept_attempts;
            log("Accept Ready All clicked relative YES anchor " + std::to_string(ready_yes_x) + " / " + std::to_string(ready_yes_y) + " on " + std::to_string(windows.size()) + " windows.");
        }
    };

    inline RuntimeState& runtime()
    {
        static RuntimeState rt;
        return rt;
    }
}