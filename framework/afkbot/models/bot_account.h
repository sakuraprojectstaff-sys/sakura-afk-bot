#pragma once
#include <string>
#include <cstdint>
namespace afkbot { struct BotAccount { int id{}; int owner_user_id{}; int slot{}; std::string sandbox_name; uint32_t dota_id{}; uint64_t steamid64{}; std::string steam3; bool has_dota_plus{}; std::string hwnd; std::string state = "OFFLINE"; }; }
