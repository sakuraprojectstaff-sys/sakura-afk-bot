#include "headers/includes.h"
#include "headers/widgets.h"

void c_gui::render()
{
	gui->initialize();
	afkbot::runtime().ui_russian = (var->gui.lang_count == 1);

	gui->push_var(style_var_window_shadow_size, SCALE(0, 0));
	gui->set_next_window_pos(ImVec2(0, 0));
	gui->set_next_window_size(SCALE(var->window.size));
	gui->begin("menu", nullptr, var->window.flags);
	{
		gui->set_style();
		gui->draw_decorations();

		gui->easing(var->gui.stage_alpha, var->gui.active_stage == var->gui.stage_count ? 1.f : 0.f, 6.f, static_easing);
		if (var->gui.stage_alpha == 0.f)
			var->gui.active_stage = var->gui.stage_count;

		gui->easing(var->gui.content_alpha, var->gui.active_section == var->gui.section_count ? 1.f : 0.f, 6.f, static_easing);

		if (var->gui.content_alpha == 0.f)
			var->gui.active_section = var->gui.section_count;

		gui->easing(var->window.size.x, var->gui.stage_count > 0 ? var->window.new_width : var->window.default_size.x, 800.f, static_easing);
		gui->easing(var->window.size.y, var->gui.stage_count > 1 ? (var->gui.section_count == 1 ? var->window.section_1_height : var->window.stage_2_height) : (var->gui.section_count == 1 ? var->window.section_1_height : var->window.default_size.y), 200.f, static_easing);

		gui->set_pos(SCALE(0, 0), pos_all);
		gui->begin_content("content", gui->window_size(), SCALE(elements->window.padding), SCALE(elements->widgets.spacing.x, elements->window.padding.y), window_flags_no_scroll_with_mouse | window_flags_no_scrollbar);
		{
			gui->push_var(style_var_alpha, var->gui.stage_alpha);
			if (var->gui.active_stage > 0)
			{
				widgets->top_bar("AFKbot Local", "22.05.2026");
				gui->sameline();
				widgets->settings_button();
			}
			gui->pop_var();
			gui->push_var(style_var_alpha, var->gui.content_alpha * var->gui.stage_alpha);
			if (var->gui.active_section == 1)
			{
				ImGui::BeginChild("settings_scroll_area", ImVec2(0, gui->content_avail().y), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

					widgets->widgets_child("HotkeysConfig", gui->language("Hotkeys / Config", "Кнопки / Конфиг", true), "B");
					{
						auto& rt = afkbot::runtime();
						rt.poll_console_key_capture();

						int action = -1;
						const std::string bind_label = rt.waiting_console_bind ? gui->language("Press key...", "Нажми кнопку...", true) : gui->language("Change bind", "Изменить бинд", true);
						widgets->action_buttons_row("ConsoleBindRow", gui->language("Console bind", "Бинд консоли", true), bind_label, rt.console_key_name(), action);
						if (action == 0)
							rt.begin_console_key_capture();

						const std::string hint_text = gui->language("Press Change bind, then any keyboard key. ESC cancels.", "Нажми Изменить бинд, затем любую кнопку. ESC отменяет.", true);
						draw->text_clipped(gui->window_drawlist(), font->get(suisse_intl_medium_data, 13), gui->get_window()->DC.CursorPos, gui->get_window()->DC.CursorPos + ImVec2(gui->content_avail().x, SCALE(18)), draw->get_clr(clr->main.text, 0.42f), hint_text.c_str());
						ImGui::Dummy(SCALE(0.0f, 18.0f));

						bool save_config_ui = rt.save_config_enabled;
						bool before_save_config_ui = save_config_ui;
						widgets->checkbox_ex("Save AFKbot config", gui->language("Save AFKbot config", "Сохранять конфиг AFKbot", true), &save_config_ui);
						if (save_config_ui != before_save_config_ui)
							rt.set_save_config_enabled(save_config_ui);

						bool show_log_console_ui = rt.log_console_visible;
						bool before_show_log_console_ui = show_log_console_ui;
						widgets->checkbox_ex("Show runtime console", gui->language("Show runtime console", "Показать консоль логов", true), &show_log_console_ui);
						if (show_log_console_ui != before_show_log_console_ui)
							rt.set_log_console_visible(show_log_console_ui);

						action = -1;
						widgets->action_buttons_row("SaveConfigNowRow", gui->language("Save config now", "Сохранить конфиг сейчас", true), gui->language("Save", "Сохранить", true), "appsettings.cfg", action);
						if (action == 0)
							rt.save_config();
					}
					widgets->widgets_end_child();

					gui->set_screen_pos(GImGui->LastItemData.Rect.Max.y + SCALE(elements->widgets.spacing.y), pos_y);
				widgets->widgets_child("General", gui->language("General", "Общие", true), "O");
				{
					static bool launch_win{ true };
					widgets->checkbox_ex("Launch on Windows startup", gui->language("Launch on Windows startup", "Запуск при старте Windows", true), &launch_win);
					static bool start_min{ false };
					widgets->checkbox_ex("Start minimized to tray", gui->language("Start minimized to tray", "Запуск сворачивается в трей", true), &start_min);
					static bool show_up{ true };
					widgets->checkbox_ex("Show Update Notifications", gui->language("Show Update Notifications", "Показать уведомления об обновлениях", true), &show_up);
				}
				widgets->widgets_end_child();

				gui->set_screen_pos(GImGui->LastItemData.Rect.Max.y + SCALE(elements->widgets.spacing.y), pos_y);
				widgets->selection_buttons("Language", gui->language("Language", "Язык", true), "English", "Русский", var->gui.lang_count);

				gui->set_screen_pos(GImGui->LastItemData.Rect.Max.y + SCALE(elements->widgets.spacing.y), pos_y);
				widgets->widgets_child("Integration", gui->language("Integration", "Интеграция", true), "P");
				{
					static bool auto_in{ false };
					widgets->checkbox_ex("Auto arrange windows after launch", gui->language("Auto arrange windows after launch", "Авто-раскладка окон после запуска", true), &auto_in);
					static bool enable_in{ true };
					widgets->checkbox_ex("Enable window watchdog", gui->language("Enable window watchdog", "Включить наблюдение за окнами", true), &enable_in);
				}
				widgets->widgets_end_child();

				gui->set_screen_pos(GImGui->LastItemData.Rect.Max.y + SCALE(elements->widgets.spacing.y), pos_y);
				static int process_sel{ 1 };
				widgets->selection_buttons("Window Input Mode", gui->language("Window Input Mode", "Режим ввода окна", true), gui->language("Normal", "Нормальный", true), gui->language("High", "Высокий", true), process_sel);

				gui->set_screen_pos(GImGui->LastItemData.Rect.Max.y + SCALE(elements->widgets.spacing.y), pos_y);
				widgets->widgets_child("Security", gui->language("Security", "Безопасность", true), "Q");
				{
					static bool auto_cl{ true };
					widgets->checkbox_ex("Save event logs on exit", gui->language("Save event logs on exit", "Сохранять логи событий при выходе", true), &auto_cl);
					ImGui::Dummy(SCALE(0.0f, 6.0f));

					auto& rt = afkbot::runtime();
					bool enable_tg = rt.telegram_reports_enabled;
					bool before_enable_tg = enable_tg;
					widgets->checkbox_ex("Enable Telegram reports", gui->language("Enable Telegram reports", "Включить Telegram отчёты", true), &enable_tg);
					if (enable_tg != before_enable_tg)
					{
						rt.telegram_reports_enabled = enable_tg;
						if (rt.save_config_enabled)
							rt.save_config();
					}
					ImGui::Dummy(SCALE(0.0f, 8.0f));

					static bool telegram_fields_loaded{ false };
					static bool telegram_token_visible{ false };
					static char telegram_token_buf[256]{};
					static char telegram_chat_buf[128]{};
					if (!telegram_fields_loaded)
					{
						strncpy_s(telegram_token_buf, rt.telegram_bot_token.c_str(), _TRUNCATE);
						strncpy_s(telegram_chat_buf, rt.telegram_chat_id.c_str(), _TRUNCATE);
						telegram_fields_loaded = true;
					}

					auto mask_token = [](const char* raw) -> std::string
					{
						std::string value = raw ? std::string(raw) : std::string();
						while (!value.empty() && isspace(static_cast<unsigned char>(value.front()))) value.erase(value.begin());
						while (!value.empty() && isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
						if (value.empty()) return "empty";
						if (value.size() <= 12) return std::string(value.size(), '*');
						return value.substr(0, 8) + "..." + value.substr(value.size() - 4);
					};

					int token_action = -1;
					if (telegram_token_visible)
					{
						widgets->text_field("telegram_token_field_visible", gui->language("Telegram bot token", "Токен Telegram бота", true), "C", telegram_token_buf, sizeof(telegram_token_buf));
						rt.telegram_bot_token = telegram_token_buf;
						widgets->action_buttons_row("TelegramTokenVisibleRow", gui->language("Token visibility", "Видимость токена", true), gui->language("Hide", "Скрыть", true), gui->language("Clear", "Очистить", true), token_action);
						if (token_action == 0)
							telegram_token_visible = false;
						if (token_action == 1)
						{
							telegram_token_buf[0] = '\0';
							rt.telegram_bot_token.clear();
							telegram_token_visible = false;
							if (rt.save_config_enabled)
								rt.save_config();
						}
					}
					else
					{
						std::string masked_token = mask_token(telegram_token_buf);
						widgets->action_buttons_row("TelegramTokenHiddenRow", gui->language("Telegram bot token", "Токен Telegram бота", true), gui->language("Show / edit", "Показать / изменить", true), masked_token, token_action);
						if (token_action == 0 || token_action == 1)
							telegram_token_visible = true;
					}

					widgets->text_field("telegram_chat_field", gui->language("Telegram chat/user ID", "Telegram chat/user ID", true), "B", telegram_chat_buf, sizeof(telegram_chat_buf));
					rt.telegram_chat_id = telegram_chat_buf;

					int telegram_action = -1;
					widgets->action_buttons_row("TelegramTestRow", gui->language("Telegram test message", "Тест Telegram", true), gui->language("Send test", "Отправить тест", true), rt.telegram_last_status, telegram_action);
					if (telegram_action == 0)
					{
						rt.telegram_bot_token = telegram_token_buf;
						rt.telegram_chat_id = telegram_chat_buf;
						if (rt.save_config_enabled)
							rt.save_config();
						rt.send_telegram_test_message();
					}

					static bool reconnect_prep{ false };
					widgets->checkbox_ex("Auto reconnect preparation", gui->language("Auto reconnect preparation", "Подготовка авто-переподключения", true), &reconnect_prep);
				}
				widgets->widgets_end_child();

				ImGui::Dummy(SCALE(0.0f, 18.0f));
				ImGui::EndChild();
			}

			if(var->gui.active_section == 0)
			{
				if (var->gui.active_stage == 0)
					widgets->log_reg_page();
				gui->begin_content("s_g_content", SCALE(0, 0), SCALE(0, 0), SCALE(elements->window.padding));
				{
					if (var->gui.active_stage == 1)
					{
						// Sync live runtime counters into the module cards.
                        {
                            auto& rt = afkbot::runtime();
                            elements->game_card.fps_count[0] = std::to_string(rt.accounts.size()) + " accounts";
                            elements->game_card.fps_count[1] = std::to_string(rt.accounts.size()) + " bound";
                            elements->game_card.fps_count[2] = std::to_string(rt.windows.size()) + " found";
                            elements->game_card.royale_count[0] = rt.last_launch_ok ? "launch sent" : "0 profiles";
                            elements->game_card.royale_count[1] = "GC " + std::to_string(rt.gc_account_ready_count()) + "/" + std::to_string(rt.gc_account_total_count());
                            elements->game_card.royale_count[2] = rt.accounts.empty() ? "0/5 party" : std::to_string(rt.accounts.size()) + "/5 party";
                            elements->game_card.royale_count[3] = std::to_string(rt.ready_accept_attempts) + " attempts";
                        }
                        widgets->games_child("core_modules", "Core Modules", "H", "V0.2");
						{
							for (int i = 0; i < elements->game_card.fps_games.size(); i++)
							{
								widgets->game_card(elements->game_card.fps_games.at(i) + "idfps", elements->game_card.fps_games.at(i), elements->game_card.fps_count.at(i), i, elements->game_card.fps_ready.at(i));
								if ((i + 1) % 3 != 0)
									gui->sameline();
							}
						}
						widgets->games_end_child();
						widgets->games_child("automation_modules", gui->language("Automation", "Автоматизация"), "I", "Local");
						{
							for (int i = 0; i < elements->game_card.royale_games.size(); i++)
							{
								widgets->game_card(elements->game_card.royale_games.at(i) + "idroyale", elements->game_card.royale_games.at(i), elements->game_card.royale_count.at(i), i + 3, elements->game_card.royale_ready.at(i));
								if ((i + 1) % 3 != 0)
									gui->sameline();
							}
						}
						widgets->games_end_child();
					}
					std::string module_desc = gui->language("AFKbot Loader module placeholder. This page is reserved for the safe multi-window launcher foundation: user profile, bot accounts, Sandboxie profiles, HWND windows, party verification, ready-check and Telegram reports.", "Модуль AFKbot Loader. Страница зарезервирована под безопасный фундамент лаунчера: профиль, боты, Sandboxie, HWND окна, party-check, ready-check и Telegram отчёты.");
						if (var->gui.active_stage == 2)
							widgets->product_page(var->gui.cs_player, 0, "Dashboard", module_desc, "V0.1", "22.05.2026", "Local", "0");
						if (var->gui.active_stage == 3)
							widgets->product_page(var->gui.cs_player, 1, "Bot Accounts", module_desc, "0 saved", "22.05.2026", "Ready", "0");
						if (var->gui.active_stage == 4)
							widgets->product_page(var->gui.apex_player, 2, "Dota Windows", module_desc, "0 found", "22.05.2026", "Ready", "0");
						if (var->gui.active_stage == 5)
							widgets->product_page(var->gui.fortnite_player, 3, "Sandbox", module_desc, "0 profiles", "22.05.2026", "Ready", "0");
						if (var->gui.active_stage == 6)
							widgets->product_page(var->gui.cs_player, 4, "GC Accounts", module_desc, "GC 0/0", "22.05.2026", "Ready", "0");
						if (var->gui.active_stage == 7)
							widgets->product_page(var->gui.cs_player, 5, "Party Builder", module_desc, "0/5", "22.05.2026", "Ready", "0");
						if (var->gui.active_stage == 8)
							widgets->product_page(var->gui.apex_player, 6, "Ready Check", module_desc, "0/0", "22.05.2026", "Ready", "0");

				}
				gui->end_content();
			}
			gui->pop_var();
		}
		gui->end_content();
		gui->pop_var();

		bool bg{ false };
		if (gui->is_window_cond(GetCurrentContext()->NavWindow, { "dropdown" }))
			bg = true;
		gui->easing(elements->loading.window_alpha, bg || var->gui.loading ? 1.f : 0.f, 8.f, static_easing);

		gui->set_pos(SCALE(0, 0), pos_all);
		gui->push_var(style_var_alpha, elements->loading.window_alpha);
		gui->begin_content("back_alpha", SCALE(0, 0), SCALE(0, 0), SCALE(0, 0));
		{
			draw->rect_filled(gui->window_drawlist(), gui->window_pos(), gui->window_pos() + gui->window_size(), draw->get_clr(clr->window.background, 0.6), SCALE(var->window.rounding));
			gui->loading();
		}
		gui->end_content();
		gui->pop_var();

		gui->move_window(var->winapi.hwnd, var->winapi.rc);
	}
	gui->end();

	if (ImGui::IsKeyPressed(ImGuiKey_Equal) && ImGui::IsKeyDown(ImGuiKey_LeftAlt) && var->gui.stored_dpi < 200)
	{
		var->gui.stored_dpi += 10;
		var->gui.dpi_changed = true;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Minus) && ImGui::IsKeyDown(ImGuiKey_LeftAlt) && var->gui.stored_dpi > 80)
	{
		var->gui.stored_dpi -= 10;
		var->gui.dpi_changed = true;
	}

	if (var->gui.dpi != var->gui.stored_dpi / 100.f)
	{
		var->gui.dpi_changed = true;
		var->gui.update_size = true;
	}
}
