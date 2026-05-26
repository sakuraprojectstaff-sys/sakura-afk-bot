#include"../headers/includes.h"
#include "../headers/widgets.h"

static int afk_action_row(const char* id, std::string_view title, std::string_view left_action, std::string_view right_action)
{
    int action = -1;
    widgets->action_buttons_row(id, title, left_action, right_action, action);
    return action;
}

static bool afk_loader_button(const char* id, const ImVec2& min, const ImVec2& max, std::string_view label, bool primary)
{
    ImGui::SetCursorScreenPos(min);
    ImGui::InvisibleButton(id, max - min);

    bool hovered = ImGui::IsItemHovered();
    bool pressed = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    bool held = ImGui::IsItemActive();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float rounding = SCALE(elements->widgets.rounding);
    const float offset = held ? SCALE(1.5f) : 0.0f;

    ImVec2 a = min + ImVec2(offset, offset);
    ImVec2 b = max - ImVec2(offset, offset);

    float base_alpha = primary ? 0.72f : 0.045f;
    float hover_alpha = primary ? 0.92f : 0.075f;
    float border_alpha = hovered ? 0.38f : 0.075f;
    float text_alpha = hovered ? 1.0f : 0.78f;

    draw->rect_filled(dl, a, b, draw->get_clr(primary ? clr->main.accent : clr->main.text, hovered ? hover_alpha : base_alpha), rounding);
    draw->rect(dl, a, b, draw->get_clr(primary ? clr->main.accent : clr->main.text, border_alpha), rounding);

    if (!primary)
        draw->rect_filled(dl, a, b, draw->get_clr(clr->window.background, hovered ? 0.10f : 0.04f), rounding);

    draw->text_clipped(dl, font->get(suisse_intl_medium_data, 14), a, b, draw->get_clr(clr->main.text, text_alpha), label.data(), gui->text_end(label.data()), NULL, ImVec2(0.5f, 0.5f));
    return pressed;
}

static void afk_section_caption(const ImVec2& panel_min, const ImVec2& panel_max, float& y, std::string_view title)
{
    ImVec2 min = ImVec2(panel_min.x + SCALE(26.f), y);
    ImVec2 max = ImVec2(panel_max.x - SCALE(26.f), y + SCALE(22.f));
    draw->text_clipped(ImGui::GetWindowDrawList(), font->get(suisse_intl_medium_data, 13), min, max, draw->get_clr(clr->main.text, 0.62f), title.data(), gui->text_end(title.data()));
    y += SCALE(28.f);
}

static int afk_button_pair(const char* id, const ImVec2& panel_min, const ImVec2& panel_max, float& y, std::string_view left, std::string_view right, bool left_primary = true)
{
    const float x = panel_min.x + SCALE(26.f);
    const float gap = SCALE(10.f);
    const float h = SCALE(38.f);
    const float w = (panel_max.x - panel_min.x - SCALE(52.f) - gap) * 0.5f;

    int action = -1;
    std::string left_id = std::string(id) + "_left";
    std::string right_id = std::string(id) + "_right";

    if (afk_loader_button(left_id.c_str(), ImVec2(x, y), ImVec2(x + w, y + h), left, left_primary))
        action = 0;
    if (afk_loader_button(right_id.c_str(), ImVec2(x + w + gap, y), ImVec2(x + w + gap + w, y + h), right, false))
        action = 1;

    y += h + SCALE(12.f);
    return action;
}


static void afk_set_row_pos(const ImVec2& panel_min, float& y)
{
    ImGui::SetCursorScreenPos(ImVec2(panel_min.x + SCALE(26.f), y));
}

static void afk_next_row(const ImVec2& panel_min, float& y)
{
    y += SCALE(46.f);
    afk_set_row_pos(panel_min, y);
}

static std::string afk_lang(std::string_view en, std::string_view ru)
{
    return gui->language(en.data(), ru.data());
}

static void afk_render_config_hint(ImDrawList* dl, const ImVec2& min, const ImVec2& max)
{
    auto& rt = afkbot::runtime();
    std::string line1 = afk_lang("Console key: ", "Кнопка консоли: ") + rt.console_key_name() + "  |  " + afk_lang("Save config: ", "Сохранение: ") + (rt.save_config_enabled ? "ON" : "OFF");
    std::string line2 = afk_lang("Runtime logs are moved to a separate console. Enable it in Settings.", "Логи вынесены в отдельную консоль. Включи её в настройках.");
    std::string line3 = afk_lang("Account ID uses dota_game_account_client_debug. Party debug only verifies party.", "ID аккаунта берётся через dota_game_account_client_debug. Party debug только проверяет пати.");
    draw->text_clipped(dl, font->get(suisse_intl_medium_data, 12), min, max, draw->get_clr(clr->main.text, 0.68f), line1.c_str());
    draw->text_clipped(dl, font->get(suisse_intl_medium_data, 12), min + SCALE(0, 18), max, draw->get_clr(clr->main.text, 0.52f), line2.c_str());
    draw->text_clipped(dl, font->get(suisse_intl_medium_data, 12), min + SCALE(0, 36), max, draw->get_clr(clr->main.text, 0.38f), line3.c_str());
}

static void afk_render_log_lines(ImDrawList* dl, const ImVec2& min, const ImVec2& max)
{
    auto& rt = afkbot::runtime();
    float y = min.y;
    int shown = 0;
    for (auto it = rt.logs.rbegin(); it != rt.logs.rend() && shown < 8; ++it, ++shown)
    {
        std::string line = "> " + *it;
        draw->text_clipped(dl, font->get(suisse_intl_medium_data, 12), ImVec2(min.x, y), max, draw->get_clr(clr->main.text, 0.55f), line.c_str());
        y += SCALE(18.f);
    }
    if (shown == 0)
        draw->text_clipped(dl, font->get(suisse_intl_medium_data, 12), min, max, draw->get_clr(clr->main.text, 0.45f), afk_lang("> Runtime log is empty. Use buttons above.", "> Лог пустой. Используй кнопки выше.").c_str());
}

static void afk_render_windows(ImDrawList* dl, const ImVec2& min, const ImVec2& max)
{
    auto& rt = afkbot::runtime();
    float y = min.y;
    if (rt.windows.empty())
    {
        draw->text_clipped(dl, font->get(suisse_intl_medium_data, 12), min, max, draw->get_clr(clr->main.text, 0.45f), afk_lang("No Dota windows found yet. Press Find Windows.", "Окна Dota ещё не найдены. Нажми Найти окна.").c_str());
        return;
    }
    int shown = 0;
    for (const auto& w : rt.windows)
    {
        if (shown++ >= 8) break;
        std::string line = "Slot " + std::to_string(w.slot) + " | HWND " + afkbot::hwnd_to_hex(w.hwnd) + " | PID " + std::to_string(w.pid) + " | " + afkbot::rect_to_string(w.rect);
        draw->text_clipped(dl, font->get(suisse_intl_medium_data, 12), ImVec2(min.x, y), max, draw->get_clr(clr->main.text, 0.55f), line.c_str());
        y += SCALE(18.f);
    }
}

static void afk_render_accounts(ImDrawList* dl, const ImVec2& min, const ImVec2& max)
{
    auto& rt = afkbot::runtime();
    float y = min.y;
    if (rt.accounts.empty())
    {
        draw->text_clipped(dl, font->get(suisse_intl_medium_data, 12), min, max, draw->get_clr(clr->main.text, 0.45f), afk_lang("Accounts are created automatically after Find Windows / Verify Party.", "Аккаунты привязываются автоматически после поиска окон / проверки пати.").c_str());
        return;
    }
    int shown = 0;
    for (const auto& a : rt.accounts)
    {
        if (shown++ >= 8) break;
        std::string line = "Bot " + std::to_string(a.slot) + " | " + a.sandbox_name + " | " + afkbot::hwnd_to_hex(a.hwnd) + " | " + a.state;
        draw->text_clipped(dl, font->get(suisse_intl_medium_data, 12), ImVec2(min.x, y), max, draw->get_clr(clr->main.text, 0.55f), line.c_str());
        y += SCALE(18.f);
    }
}

static void afk_module_controls(std::string_view name, const ImVec2& panel_min, const ImVec2& panel_max)
{
    auto& rt = afkbot::runtime();
    float y = panel_min.y + SCALE(86.f);
    int action = -1;

    if (name == "Dashboard")
    {
        afk_section_caption(panel_min, panel_max, y, afk_lang("Sandboxie", "Sandboxie"));
        action = afk_button_pair("DashSandboxPair", panel_min, panel_max, y, afk_lang("Launch all", "Запустить все"), afk_lang("Find windows", "Найти окна"));
        if (action == 0) rt.launch_all_sandboxie();
        if (action == 1) rt.find_dota_windows();

        afk_section_caption(panel_min, panel_max, y, afk_lang("Account binding", "Привязка аккаунтов"));
        action = afk_button_pair("DashWindowPair", panel_min, panel_max, y, afk_lang("Bind HWND", "Привязать HWND"), afk_lang("Identify IDs", "Определить ID"));
        if (action == 0) rt.ensure_demo_accounts_from_windows();
        if (action == 1) rt.identify_all();

        afk_section_caption(panel_min, panel_max, y, afk_lang("Party", "Пати"));
        action = afk_button_pair("DashPartyPair", panel_min, panel_max, y, afk_lang("Verify party", "Проверить пати"), afk_lang("Open console", "Открыть консоль"));
        if (action == 0) rt.verify_party_all();
        if (action == 1) rt.open_console_all();
    }
    else if (name == "Bot Accounts")
    {
        afk_section_caption(panel_min, panel_max, y, afk_lang("Accounts", "Аккаунты"));
        action = afk_button_pair("AccountsPair1", panel_min, panel_max, y, afk_lang("Bind HWND", "Привязать HWND"), afk_lang("Identify IDs", "Определить ID"));
        if (action == 0) rt.ensure_demo_accounts_from_windows();
        if (action == 1) rt.identify_all();

        afk_section_caption(panel_min, panel_max, y, afk_lang("Windows", "Окна"));
        action = afk_button_pair("AccountsPair2", panel_min, panel_max, y, afk_lang("Refresh", "Обновить"), afk_lang("Open console", "Открыть консоль"));
        if (action == 0) { rt.find_dota_windows(); rt.ensure_demo_accounts_from_windows(); }
        if (action == 1) rt.open_console_all();
    }
    else if (name == "Dota Windows")
    {
        afk_section_caption(panel_min, panel_max, y, afk_lang("Windows", "Окна"));
        action = afk_button_pair("WindowsPair1", panel_min, panel_max, y, afk_lang("Find", "Найти"), afk_lang("Arrange 2x5", "Разложить 2x5"));
        if (action == 0) rt.find_dota_windows();
        if (action == 1) rt.arrange_2x5();

        afk_section_caption(panel_min, panel_max, y, afk_lang("Console", "Консоль"));
        action = afk_button_pair("WindowsPair2", panel_min, panel_max, y, afk_lang("Open all", "Открыть все"), afk_lang("Identify IDs", "Определить ID"));
        if (action == 0) rt.open_console_all();
        if (action == 1) rt.identify_all();
    }
    else if (name == "Sandbox")
    {
        afk_section_caption(panel_min, panel_max, y, afk_lang("Sandboxie", "Sandboxie"));
        action = afk_button_pair("SandboxPair1", panel_min, panel_max, y, afk_lang("Launch all", "Запустить все"), afk_lang("Find after", "Найти после"));
        if (action == 0) rt.launch_all_sandboxie();
        if (action == 1) rt.find_dota_windows();
    }
    else if (name == "Party Builder")
    {
        afk_section_caption(panel_min, panel_max, y, afk_lang("Party debug", "Party debug"));
        action = afk_button_pair("PartyPair1", panel_min, panel_max, y, afk_lang("Verify party", "Проверить пати"), afk_lang("Open console", "Открыть консоль"));
        if (action == 0) rt.verify_party_all();
        if (action == 1) rt.open_console_all();

        afk_section_caption(panel_min, panel_max, y, afk_lang("Bindings", "Привязки"));
        action = afk_button_pair("PartyPair2", panel_min, panel_max, y, afk_lang("Identify IDs", "Определить ID"), afk_lang("Find windows", "Найти окна"));
        if (action == 0) rt.identify_all();
        if (action == 1) rt.find_dota_windows();
    }
    else if (name == "Ready Check")
    {
        afk_section_caption(panel_min, panel_max, y, afk_lang("Ready check", "Ready check"));
        action = afk_button_pair("ReadyPair1", panel_min, panel_max, y, afk_lang("Start leader", "Запуск лидер"), afk_lang("Accept all", "Принять все"));
        if (action == 0) rt.start_ready_check_leader();
        if (action == 1) rt.accept_ready_all();
    }
}




void c_widgets::log_reg_page()
{
	gui->begin_content("log_reg_zone", SCALE(elements->log_reg_page.log_reg_width, 0), SCALE(elements->log_reg_page.padding), SCALE(0, elements->window.padding.y), window_flags_no_scroll_with_mouse | window_flags_no_scrollbar);
	{
		draw->shadow_circle(gui->window_drawlist(), gui->window_pos() + ImVec2(gui->window_size().x / 2, SCALE(elements->log_reg_page.padding.y + elements->log_reg_page.shadow_radius)), SCALE(elements->log_reg_page.shadow_radius), draw->get_clr(clr->main.accent, 0.1), SCALE(100), ImVec2(0, 0), 0, 60);
		draw->text_clipped(gui->window_drawlist(), font->get(icons_data, 40), gui->window_pos() + SCALE(0, elements->log_reg_page.padding.y + 4), gui->window_pos() + gui->window_size(), draw->get_clr(clr->main.accent), "A", NULL, NULL, ImVec2(0.5f, 0.f));
		draw->text_clipped(gui->window_drawlist(), font->get(suisse_intl_medium_data, 16), gui->window_pos() + SCALE(0, elements->log_reg_page.padding.y + elements->window.padding.y + elements->log_reg_page.shadow_radius * 2 - 4), gui->window_pos() + gui->window_size(), draw->get_clr(clr->main.text), "AFKbot Loader", NULL, NULL, ImVec2(0.5f, 0.f));
		draw->text_clipped(gui->window_drawlist(), font->get(suisse_intl_regular_data, 14), gui->window_pos() + ImVec2(0, SCALE(elements->log_reg_page.padding.y + elements->window.padding.y + elements->log_reg_page.shadow_radius * 2 + 1) + gui->text_size(font->get(suisse_intl_medium_data, 16), "AFKbot Loader").y), gui->window_pos() + gui->window_size(), draw->get_clr(clr->main.text, 0.48), "Multi-window control center.", NULL, NULL, ImVec2(0.5f, 0.f));
		
		gui->easing(elements->log_reg_page.window_height, var->gui.registration ? (elements->textfield.size.y * 4 + elements->window.padding.y + elements->widgets.spacing.y * 2) : (elements->textfield.size.y * 3 + elements->window.padding.y + elements->widgets.spacing.y * 1), 12.f, dynamic_easing);
		gui->set_pos(ImVec2((gui->window_width() - SCALE(elements->textfield.size.x)) / 2, (gui->window_height() - SCALE(elements->log_reg_page.window_height)) / 2), pos_all);
		gui->begin_content("registration_zone", SCALE(elements->textfield.size.x, 0), SCALE(0, 0), SCALE(0, elements->window.padding.y), window_flags_no_scroll_with_mouse | window_flags_no_scrollbar);
		{
			gui->begin_content("fields_zone", SCALE(elements->textfield.size.x, elements->log_reg_page.window_height - (elements->textfield.size.y + elements->window.padding.y)), SCALE(0, 0), SCALE(0, elements->widgets.spacing.y), window_flags_no_scroll_with_mouse | window_flags_no_scrollbar);
			{
				static char username[32];
				widgets->text_field("username_field", "Username", "B", username, sizeof(username));
				static char password[32];
				widgets->text_field("password_field", "Password", "C", password, sizeof(password));

				if ((var->gui.username == username && var->gui.password == password) && var->gui.registered)
					var->gui.stage_count = 1;
			}
			gui->end_content();

			widgets->reg_log_buttons();
		}
		gui->end_content();
		const float version_zone_height = 48.f;
		gui->set_pos(ImVec2((gui->window_width() - SCALE(elements->textfield.size.x)) / 2, gui->window_height() - SCALE(elements->log_reg_page.padding.y + version_zone_height)), pos_all);
		gui->begin_content("versions_zone", SCALE(elements->textfield.size.x, version_zone_height), SCALE(0, 0), SCALE(0, 0), window_flags_no_scroll_with_mouse | window_flags_no_scrollbar);
		{
			draw->text_clipped(gui->window_drawlist(), font->get(suisse_intl_semi_bold_data, 13), gui->window_pos(), gui->window_pos() + ImVec2(gui->window_width(), SCALE(20)), draw->get_clr(clr->main.text, 0.56), "AFKbot Loader V0.1", NULL, NULL, ImVec2(0.f, 0.5f));
			draw->text_clipped(gui->window_drawlist(), font->get(suisse_intl_semi_bold_data, 13), gui->window_pos(), gui->window_pos() + ImVec2(gui->window_width(), SCALE(20)), draw->get_clr(clr->main.text, 0.16), "22.05.2026", NULL, NULL, ImVec2(1.f, 0.5f));
		}
		gui->end_content();
	}
	gui->end_content();
	gui->sameline();
	gui->begin_content("decoration_zone", ImVec2(0, 0), SCALE(0, 0), SCALE(elements->widgets.spacing.x, elements->window.padding.y), window_flags_no_scroll_with_mouse | window_flags_no_scrollbar);
	{
		draw->image_rounded(gui->window_drawlist(), var->gui.decoration[0], gui->window_pos(), gui->window_pos() + ImVec2(gui->window_size().x, SCALE(elements->log_reg_page.img_height)), ImVec2(0, 0), ImVec2(1, 1), draw->get_clr({1.f, 1.f, 1.f, 1.f}), SCALE(elements->widgets.rounding));
		draw->rect(gui->window_drawlist(), gui->window_pos(), gui->window_pos() + ImVec2(gui->window_size().x, SCALE(elements->log_reg_page.img_height)), draw->get_clr(clr->main.text, 0.08), SCALE(elements->widgets.rounding));
		gui->set_pos(SCALE(0, elements->window.padding.y + elements->log_reg_page.img_height), pos_all);

		widgets->info_card("launches_1_id", "E", "Active Bots", "0", (gui->window_size().x - SCALE(elements->widgets.spacing.x * 2)) / 3);
		gui->sameline();
		widgets->info_card("total_users_1_id", "a", "Dota Windows", "0", (gui->window_size().x - SCALE(elements->widgets.spacing.x * 2)) / 3);
		gui->sameline();
		widgets->info_card("products_1_id", "G", "Ready Status", "0/0", (gui->window_size().x - SCALE(elements->widgets.spacing.x * 2)) / 3);
	}
	gui->end_content();
};

void c_widgets::product_page(c_video_player& player, int img_id, std::string_view name, std::string_view desc, std::string_view launches, std::string_view updated, std::string_view status, std::string_view online)
{
	gui->begin_content("product_desc_content", ImVec2(gui->content_max().x - SCALE(elements->player.size.x + elements->window.padding.x), gui->content_avail().y), SCALE(0, 0), SCALE(elements->widgets.spacing), window_flags_no_scrollbar | window_flags_no_scroll_with_mouse);
	{
		ImGuiContext& g = *GImGui;
		widgets->back_button();

		const ImRect game_rect(ImVec2(g.LastItemData.Rect.Min.x, g.LastItemData.Rect.Max.y + SCALE(elements->widgets.spacing.y)), ImVec2(g.LastItemData.Rect.Min.x + gui->content_avail().x, g.LastItemData.Rect.Max.y + SCALE(elements->widgets.spacing.y + elements->product_page.game_zone_height)));
		const ImRect desc_rect(ImVec2(game_rect.Min.x, game_rect.Max.y + SCALE(elements->product_page.back_button_padding)), gui->window_pos() + gui->window_size() - SCALE(0, elements->widgets.spacing.y * 2 + elements->info_card.height * 2));

		const ImVec2 module_icon_min = ImVec2(game_rect.Min.x, game_rect.GetCenter().y - SCALE(elements->product_page.img_size.y / 2));
		const ImVec2 module_icon_max = ImVec2(game_rect.Min.x + SCALE(elements->product_page.img_size.x), game_rect.GetCenter().y + SCALE(elements->product_page.img_size.y / 2));
		draw->rect_filled(gui->window_drawlist(), module_icon_min, module_icon_max, draw->get_clr(clr->main.accent, 0.18f), SCALE(elements->version_card.rounding));
		draw->rect(gui->window_drawlist(), module_icon_min, module_icon_max, draw->get_clr(clr->main.accent, 0.46f), SCALE(elements->version_card.rounding));
		draw->text_clipped(gui->window_drawlist(), font->get(icons_data, 28), module_icon_min, module_icon_max, draw->get_clr(clr->main.accent), "A", NULL, NULL, ImVec2(0.5f, 0.5f));
	
		draw->text_clipped(gui->window_drawlist(), font->get(suisse_intl_medium_data, 19), game_rect.Min + SCALE(elements->product_page.img_size.x + elements->product_page.back_button_padding, 0), game_rect.Max, draw->get_clr(clr->main.text), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.f, 0.5f));
	
		std::vector<std::string> lines = gui->wrap_text(font->get(suisse_intl_medium_data, 13), gui->content_avail().x, std::string(desc));

		ImVec2 desc_pos = desc_rect.Min;
		for (const auto& line : lines)
		{
			draw->text_clipped(gui->window_drawlist(), font->get(suisse_intl_medium_data, 13), desc_pos, desc_rect.Max, draw->get_clr(clr->main.text, 0.48), line.c_str());
			desc_pos.y += gui->text_size(font->get(suisse_intl_medium_data, 13), "A").y;
		}

		gui->set_screen_pos(ImVec2(desc_rect.Min.x, desc_rect.Max.y + SCALE(elements->widgets.spacing.y)), pos_all);
		gui->begin_group();
		{
			widgets->info_card("launches_2_id", "E", gui->language("Actions", "Действия"), "12.679", (gui->window_size().x - SCALE(elements->widgets.spacing.x)) / 2);
			gui->sameline();
			widgets->info_card("updated_2_id", "W", gui->language("Updated", "Обновлено"), "22.05.2026", (gui->window_size().x - SCALE(elements->widgets.spacing.x)) / 2);
		}
		gui->end_group();
		gui->begin_group();
		{
			widgets->info_card("status_2_id", "b", gui->language("Status", "Статус"), gui->language("Ready", "Готов"), (gui->window_size().x - SCALE(elements->widgets.spacing.x)) / 2);
			gui->sameline();
			widgets->info_card("online_2_id", "a", gui->language("Windows", "Окна"), "146", (gui->window_size().x - SCALE(elements->widgets.spacing.x)) / 2);
		}
		gui->end_group();
	}
	gui->end_content();
	gui->sameline();
	gui->begin_content("player_content", ImVec2(SCALE(elements->player.size.x), gui->content_avail().y), SCALE(0, 0), SCALE(elements->widgets.spacing), window_flags_no_scrollbar | window_flags_no_scroll_with_mouse);
	{
		const ImVec2 panel_min = gui->window_pos();
		const ImVec2 panel_max = gui->window_pos() + gui->window_size();
		const float rounding = SCALE(elements->widgets.rounding);

		draw->rect_filled(gui->window_drawlist(), panel_min, panel_max, draw->get_clr(clr->loading.win_bg), rounding);
		draw->rect(gui->window_drawlist(), panel_min, panel_max, draw->get_clr(clr->main.accent, 0.42f), rounding);

		const ImVec2 center = ImVec2((panel_min.x + panel_max.x) * 0.5f, (panel_min.y + panel_max.y) * 0.47f);
		for (int i = 0; i < 5; ++i)
		{
			draw->circle(gui->window_drawlist(), center, SCALE(48.f + i * 44.f), draw->get_clr(clr->main.accent, 0.10f), 96, SCALE(1.0f));
		}

		ImVec2 diamond[4] = {
			ImVec2(center.x, center.y - SCALE(40.f)),
			ImVec2(center.x + SCALE(40.f), center.y),
			ImVec2(center.x, center.y + SCALE(40.f)),
			ImVec2(center.x - SCALE(40.f), center.y)
		};
		draw->line(gui->window_drawlist(), diamond[0], diamond[1], draw->get_clr(clr->main.accent, 0.90f), SCALE(2.f));
		draw->line(gui->window_drawlist(), diamond[1], diamond[2], draw->get_clr(clr->main.accent, 0.90f), SCALE(2.f));
		draw->line(gui->window_drawlist(), diamond[2], diamond[3], draw->get_clr(clr->main.accent, 0.90f), SCALE(2.f));
		draw->line(gui->window_drawlist(), diamond[3], diamond[0], draw->get_clr(clr->main.accent, 0.90f), SCALE(2.f));
		draw->circle_filled(gui->window_drawlist(), center, SCALE(6.f), draw->get_clr(clr->main.accent));

		const ImVec2 text_min = panel_min + SCALE(26.f, 26.f);
		const ImVec2 text_max = panel_max - SCALE(26.f, 26.f);
		draw->text_clipped(gui->window_drawlist(), font->get(suisse_intl_medium_data, 22), text_min, text_max, draw->get_clr(clr->main.text), name.data(), gui->text_end(name.data()));
		afk_module_controls(name, panel_min, panel_max);
	}
	gui->end_content();

};