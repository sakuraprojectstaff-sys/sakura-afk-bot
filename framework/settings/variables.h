#pragma once
#include <string>
#include <vector>
#include "imgui.h"
#include "../headers/flags.h"
#include <wtypes.h>

#include <d3d11.h>
#include <dxgi.h>

#include "imgui_internal.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <deque>
#include <functional>
#include <memory>
// AFKbot safe build: FFmpeg/PortAudio preview playback removed.
// The original UIEngine source depended on FFmpeg DLLs at runtime.
// For AFKbot Loader V0.0 we keep a lightweight preview widget stub instead.

struct video_state
{
    bool playing = false;
    double current_pts = 0.0;
    double duration = 0.0;
};

class c_video_player
{
public:
    bool init_video(const char*) { return false; }
    void cleanup_video() {}
    void seek_video(double) {}
    void render(std::string_view id, const ImVec2& size, std::string_view name);

    video_state state{};
    bool texture_initialized = false;

    ID3D11Device* g_pd3dDevice = nullptr;
    ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
};

struct ID3D11ShaderResourceView;
struct ID3D11Device;
struct ID3D11DeviceContext;

class c_variables
{
public:

	struct
	{
		HWND hwnd;
		RECT rc;

		bool is_fullscreen;
		RECT restore_rect;

		ID3D11Device* device_dx11{ nullptr };
		ID3D11DeviceContext* device_context{ nullptr };
		IDXGISwapChain* swap_chain{ nullptr };
	} winapi;

	struct
	{
		ImVec2 size{ 1024, 544 };
		ImVec2 default_size{ 1024, 544 };
		float new_width{ 812 };
		float stage_2_height{ 456 };
		float section_1_height{ 526 };
		float rounding{ 16 };
		float border_size{ 1 };
		float scroll_bar_width{ 4 };
		float scroll_bar_rounding{ 100 };
		window_flags flags{ window_flags_no_saved_settings | window_flags_no_nav | window_flags_no_decoration | window_flags_no_scrollbar | window_flags_no_scroll_with_mouse | window_flags_no_background };
	} window;

	struct
	{
		bool registration{ false };
		bool registered{ false };
		std::string username{ "Sakura" };
		std::string password{ "12345678901" };
		int lang_count{ 0 };
		bool lang_changing{ false };

		float stage_alpha{ 0.f };
		int active_stage{ 0 };
		int stage_count{ 0 };

		float content_alpha{ 0.f };
		int active_section{ 0 };
		int section_count{ 0 };

		float dpi = 1.f;
		int stored_dpi = 100;
		bool dpi_changed = true;
		bool update_size{ false };
		bool dragging{ false };

		bool loading = false;

		c_video_player cs_player;
		c_video_player apex_player;
		c_video_player fortnite_player;

		ID3D11ShaderResourceView* menu_background;
		ID3D11ShaderResourceView* decoration[2];
		ID3D11ShaderResourceView* img_for_versions[5];
		ID3D11ShaderResourceView* games[6];
		ID3D11ShaderResourceView* flags[2];

	} gui;

	gui_style style;

};

inline std::unique_ptr<c_variables> var = std::make_unique<c_variables>();
