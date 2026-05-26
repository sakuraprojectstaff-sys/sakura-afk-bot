#pragma once
#include <vector>
#include "../models/dota_window.h"
namespace afkbot { class WindowService { public: std::vector<DotaWindow> find_dota_windows(); void arrange_grid(int rows, int cols); }; }
