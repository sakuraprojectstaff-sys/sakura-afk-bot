#pragma once
#include <string>
#include <vector>
namespace afkbot { struct PartyState { std::string party_id; int member_count{}; std::vector<unsigned long long> member_steamids; }; }
