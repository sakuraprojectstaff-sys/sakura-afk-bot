#pragma once
#include <cstdint>
#include <string>
namespace afkbot {
    constexpr uint64_t kSteamId64Base = 76561197960265728ULL;
    inline uint64_t dota_id_to_steamid64(uint32_t dota_id) { return kSteamId64Base + static_cast<uint64_t>(dota_id); }
    inline uint32_t steamid64_to_dota_id(uint64_t steamid64) { return static_cast<uint32_t>(steamid64 - kSteamId64Base); }
    inline std::string steam3_from_dota_id(uint32_t dota_id) { return "[U:1:" + std::to_string(dota_id) + "]"; }
}
