#pragma once
#include <string>
#include <Windows.h>
namespace afkbot { struct DotaWindow { HWND hwnd{}; DWORD process_id{}; std::string title; RECT rect{}; bool responsive{}; }; }
