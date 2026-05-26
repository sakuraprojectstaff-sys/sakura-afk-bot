#pragma once

// AFKbot Loader V0.2 runtime skeleton.
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
#include <wininet.h>
#include <shellapi.h>
#pragma comment(lib, "wininet.lib")

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
        std::string state{"FOUND"};
    };

    struct BotAccountInfo
    {
        int slot{};
        std::string sandbox_name;
        uint32_t dota_id{};
        uint64_t steamid64{};
        std::string steam3;
        HWND hwnd{};
        std::string state{"UNKNOWN"};
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

        void ensure_demo_accounts_from_windows()
        {
            accounts.clear();
            int slot = 1;
            for (const auto& w : windows)
            {
                BotAccountInfo acc;
                acc.slot = slot;
                acc.sandbox_name = "DotaBox" + two_digits(slot);
                acc.hwnd = w.hwnd;
                acc.state = "HWND_BOUND_ID_PENDING";
                accounts.push_back(acc);
                ++slot;
            }
        }

        static std::string two_digits(int value)
        {
            std::ostringstream ss;
            ss << std::setw(2) << std::setfill('0') << value;
            return ss.str();
        }

        bool launch_all_sandboxie()
        {
            const wchar_t* candidates[] = {
                L"C:\\Program Files\\Sandboxie-Plus\\Start.exe",
                L"C:\\Program Files\\Sandboxie\\Start.exe",
                L"C:\\Program Files (x86)\\Sandboxie-Plus\\Start.exe",
                L"C:\\Program Files (x86)\\Sandboxie\\Start.exe"
            };

            std::wstring start_exe;
            for (auto* candidate : candidates)
            {
                if (GetFileAttributesW(candidate) != INVALID_FILE_ATTRIBUTES)
                {
                    start_exe = candidate;
                    break;
                }
            }

            if (start_exe.empty())
            {
                log("Sandboxie Start.exe not found. Configure Sandboxie path later in V0.3.");
                last_launch_ok = false;
                return false;
            }

            int launched = 0;
            for (int slot = 1; slot <= 10; ++slot)
            {
                std::wstring box = L"DotaBox" + std::to_wstring(slot < 10 ? 0 : slot / 10) + std::to_wstring(slot % 10);
                if (slot >= 10) box = L"DotaBox" + std::to_wstring(slot);
                std::wstring command = L"\"" + start_exe + L"\" /box:" + box + L" steam://rungameid/570";

                STARTUPINFOW si{};
                si.cb = sizeof(si);
                PROCESS_INFORMATION pi{};
                std::vector<wchar_t> cmd(command.begin(), command.end());
                cmd.push_back(L'\0');

                if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
                {
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                    ++launched;
                    Sleep(250);
                }
            }

            last_launch_ok = launched > 0;
            log("Launch All requested via Sandboxie. Started slots: " + std::to_string(launched));
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

            std::wstring process_path = lowercase(get_process_path(out_pid));
            std::wstring title_lower = lowercase(out_title);

            return title_lower.find(L"dota") != std::wstring::npos ||
                   process_path.find(L"dota2.exe") != std::wstring::npos ||
                   process_path.find(L"dota 2") != std::wstring::npos;
        }

        void find_dota_windows()
        {
            windows.clear();

            EnumWindows([](HWND hwnd, LPARAM lparam) -> BOOL
            {
                auto* self = reinterpret_cast<RuntimeState*>(lparam);
                std::wstring title;
                DWORD pid = 0;
                RECT rect{};
                if (!is_dota_candidate_window(hwnd, title, pid, rect))
                    return TRUE;

                DotaWindowInfo info;
                info.hwnd = hwnd;
                info.pid = pid;
                info.title = narrow(title);
                info.process_path = narrow(get_process_path(pid));
                info.rect = rect;
                info.responsive = is_responsive(hwnd);
                info.slot = static_cast<int>(self->windows.size()) + 1;
                self->windows.push_back(info);
                return TRUE;
            }, reinterpret_cast<LPARAM>(this));

            ensure_demo_accounts_from_windows();
            log("Find Windows completed. Dota windows found: " + std::to_string(windows.size()));
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
                log("Open Console All skipped: no Dota windows found.");
                return;
            }

            if (dota_console_open_hint)
            {
                log("Open Console All skipped: console is already marked open. This prevents accidentally closing it with toggleconsole.");
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
            log("Open Console All sent " + console_key_name() + " to " + std::to_string(windows.size()) + " windows.");
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
                log("Console opened before sending commands: " + console_key_name());
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
            if (static_cast<size_t>(slot - 1) < accounts.size())
            {
                accounts[slot - 1].dota_id = dota_id;
                accounts[slot - 1].steamid64 = steam64;
                accounts[slot - 1].steam3 = steam3_from_dota(dota_id);
                accounts[slot - 1].state = account_role_for_slot(slot);
                accounts[slot - 1].hwnd = w.hwnd;
            }

            log("Slot " + std::to_string(slot) + " [" + account_role_for_slot(slot) + "]: parsed account_id=" + std::to_string(dota_id) + " steam64=" + std::to_string(steam64) + " steam3=" + steam3_from_dota(dota_id) + " source=" + source);
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

        std::filesystem::path steam_root_from_window(const DotaWindowInfo& w) const
        {
            std::filesystem::path exe = widen(w.process_path);
            std::filesystem::path p = exe.has_filename() ? exe.parent_path() : exe;

            for (int i = 0; i < 16 && !p.empty(); ++i)
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

        uint32_t parse_account_id_from_steam_userdata(const DotaWindowInfo& w, std::filesystem::path* source_path = nullptr) const
        {
            std::filesystem::path steam_root = steam_root_from_window(w);
            if (steam_root.empty())
                return 0;

            std::filesystem::path userdata = steam_root / L"userdata";
            std::error_code ec;
            if (!std::filesystem::exists(userdata, ec) || !std::filesystem::is_directory(userdata, ec))
            {
                return parse_account_id_from_loginusers(steam_root, source_path);
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

            return parse_account_id_from_loginusers(steam_root, source_path);
        }

        bool bind_account_from_steam_filesystem(const DotaWindowInfo& w, int slot)
        {
            std::filesystem::path source;
            uint32_t dota_id = parse_account_id_from_steam_userdata(w, &source);
            if (!dota_id)
                return false;

            std::string src = source.empty() ? "Steam userdata" : narrow(source.wstring());
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
                log("Slot " + std::to_string(slot) + ": account dump/log not found. Console may have been closed before commands, or Dota wrote dump to an unknown folder. First guess: " + narrow(guessed.wstring()));
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

            bind_account_identity(w, slot, dota_id, "Dota console dump " + narrow(dump.wstring()));
        }

        void identify_accounts_all()
        {
            if (windows.empty())
                find_dota_windows();
            if (windows.empty())
            {
                log("Account ID parse skipped: no Dota windows found.");
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

                log("Slot " + std::to_string(slot) + ": Steam userdata/loginusers fallback failed. Trying Dota console dump.");

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

            log("Account ID parse completed. account_id is used as Dota invite ID.");
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
            execute_console_command(windows.front().hwnd, "dota_start_party_ready_check");
            log("Ready-check command sent to leader window.");
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
