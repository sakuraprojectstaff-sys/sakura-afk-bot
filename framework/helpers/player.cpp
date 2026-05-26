#include "../headers/includes.h"

void c_video_player::render(std::string_view id, const ImVec2& size, std::string_view name)
{
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (size.x > 0.0f && size.x < avail.x)
        avail.x = size.x;
    if (size.y > 0.0f && size.y < avail.y)
        avail.y = size.y;

    if (avail.x < 220.0f)
        avail.x = 220.0f;
    if (avail.y < 220.0f)
        avail.y = 220.0f;

    std::string btn_id = std::string("afk_preview_") + std::string(id);
    ImGui::InvisibleButton(btn_id.c_str(), avail);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    ImVec2 center = ImVec2((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f);

    const ImU32 bg0 = IM_COL32(16, 14, 22, 255);
    const ImU32 bg1 = IM_COL32(28, 22, 40, 255);
    const ImU32 border = IM_COL32(156, 105, 255, 85);
    const ImU32 accent = IM_COL32(184, 128, 255, 255);
    const ImU32 text = IM_COL32(238, 235, 248, 255);
    const ImU32 muted = IM_COL32(165, 158, 184, 255);

    dl->AddRectFilled(p0, p1, bg0, 16.0f);
    dl->AddRectFilledMultiColor(p0, p1, bg1, bg0, bg0, bg1);
    dl->AddRect(p0, p1, border, 16.0f, 0, 1.4f);

    for (int i = 0; i < 6; ++i)
    {
        float r = 42.0f + i * 28.0f;
        dl->AddCircle(center, r, IM_COL32(130, 84, 255, 20 + i * 8), 64, 1.2f);
    }

    ImVec2 diamond[4] = {
        ImVec2(center.x, center.y - 38.0f),
        ImVec2(center.x + 38.0f, center.y),
        ImVec2(center.x, center.y + 38.0f),
        ImVec2(center.x - 38.0f, center.y)
    };
    dl->AddPolyline(diamond, 4, accent, true, 2.0f);
    dl->AddCircleFilled(center, 5.0f, accent, 24);

    std::string title = std::string(name);
    std::string line1 = "AFKbot Loader Preview";
    std::string line2 = "Safe build: FFmpeg / PortAudio disabled";
    std::string line3 = "Next: Dashboard, bots, windows, party, ready-check";

    dl->AddText(ImVec2(p0.x + 18.0f, p0.y + 18.0f), text, title.c_str());
    dl->AddText(ImVec2(p0.x + 18.0f, p0.y + 44.0f), accent, line1.c_str());
    dl->AddText(ImVec2(p0.x + 18.0f, p1.y - 64.0f), muted, line2.c_str());
    dl->AddText(ImVec2(p0.x + 18.0f, p1.y - 38.0f), muted, line3.c_str());
}
