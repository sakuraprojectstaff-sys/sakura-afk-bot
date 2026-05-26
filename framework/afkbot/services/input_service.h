#pragma once
#include <Windows.h>
namespace afkbot { inline POINT relative_point(HWND hwnd, float rx, float ry) { RECT rc{}; GetClientRect(hwnd, &rc); return { LONG((rc.right - rc.left) * rx), LONG((rc.bottom - rc.top) * ry) }; } }
