#pragma once

#include <windows.h>
#include <tchar.h>
#include <sstream>

#include "misc.h"
#include "high_resolution_timer.h"

#ifdef USE_IMGUI
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern ImWchar glyphRangesJapanese[];
#endif

#include <d3d11.h>
#include "sprite.h"
#include <wrl.h>
#include "geometric_primitive.h"
#include "static_mesh.h"

CONST LONG SCREEN_WIDTH{ 1280 };
CONST LONG SCREEN_HEIGHT{ 720 };
CONST BOOL FULLSCREEN{ FALSE };
CONST LPWSTR APPLICATION_NAME{ L"X3DGP" };

class framework
{
public:
	CONST HWND hwnd;

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediate_context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth_stencil_view;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_state;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_stencil_state;
	Microsoft::WRL::ComPtr<ID3D11BlendState> blend_state;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_state;

	float timer{0.0f};
	bool flag{false};

	DirectX::XMFLOAT3 camera_position{ 0.0f, 0.0f, -10.0f };
	DirectX::XMFLOAT3 camera_focus{ 0.0f, 0.0f, 0.0f };
	float rotateX{ 0.0f };
	float rotateY{ DirectX::XMConvertToRadians(180) };
	POINT cursor_position; 
	float wheel{ 0 };
	float distance{ 10.0f };

	DirectX::XMFLOAT3 translation{ 0, 0, 0 };
	DirectX::XMFLOAT3 scaling{ 1, 1, 1 };
	DirectX::XMFLOAT3 rotation{ 0, 0, 0 };
	DirectX::XMFLOAT4 material_color{ 1 ,1, 1, 1 };

	std::vector<std::unique_ptr<static_mesh>> dummy_static_meshes;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> mesh_vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> mesh_input_layout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> mesh_pixel_shader;

	std::unique_ptr<sprite> dummy_sprite;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> sprite_vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> sprite_input_layout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> sprite_pixel_shader;

	framework(HWND hwnd);
	~framework();

	framework(const framework&) = delete;
	framework& operator=(const framework&) = delete;
	framework(framework&&) noexcept = delete;
	framework& operator=(framework&&) noexcept = delete;

	int run()
	{
		MSG msg{};

		if (!initialize())
		{
			return 0;
		}

#ifdef USE_IMGUI
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 14.0f, nullptr, glyphRangesJapanese);
		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX11_Init(device.Get(), immediate_context.Get());
		ImGui::StyleColorsDark();
#endif

		while (WM_QUIT != msg.message)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				tictoc.tick();
				calculate_frame_stats();
				update(tictoc.time_interval());
				render(tictoc.time_interval());
			}
		}

#ifdef USE_IMGUI
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
#endif

		BOOL fullscreen{};
		swap_chain->GetFullscreenState(&fullscreen, 0);
		if (fullscreen)
		{
			swap_chain->SetFullscreenState(FALSE, 0);
		}

		return uninitialize() ? static_cast<int>(msg.wParam) : 0;
	}

	LRESULT CALLBACK handle_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
#ifdef USE_IMGUI
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) { return true; }
#endif
		switch (msg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);
			
			EndPaint(hwnd, &ps);
			break;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_CREATE:
			break;
		case WM_KEYDOWN:
			if (wparam == VK_ESCAPE)
			{
				PostMessage(hwnd, WM_CLOSE, 0, 0);
			}
			break;
		case WM_ENTERSIZEMOVE:
			tictoc.stop();
			break;
		case WM_EXITSIZEMOVE:
			tictoc.start();
			break;
		case WM_MOUSEWHEEL:
			wheel = GET_WHEEL_DELTA_WPARAM(wparam);
			break;
		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		return 0;
	}

private:
	bool initialize();
	void update(float elapsed_time/*Elapsed seconds from last frame*/);
	void render(float elapsed_time/*Elapsed seconds from last frame*/);
	bool uninitialize();

private:
	high_resolution_timer tictoc;
	uint32_t frames{ 0 };
	float elapsed_time{ 0.0f };
	void calculate_frame_stats()
	{
		if (++frames, (tictoc.time_stamp() - elapsed_time) >= 1.0f)
		{
			float fps = static_cast<float>(frames);
			std::wostringstream outs;
			outs.precision(6);
			outs << APPLICATION_NAME << L" : FPS : " << fps << L" / " << L"Frame Time : " << 1000.0f / fps << L" (ms)";
			SetWindowTextW(hwnd, outs.str().c_str());

			frames = 0;
			elapsed_time += 1.0f;
		}
	}

public:
	struct scroll_constants
	{
		DirectX::XMFLOAT2 scroll_direction;
		DirectX::XMFLOAT2 scroll_dummy;
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> scroll_constant_buffer;
	DirectX::XMFLOAT2 scroll_direction;

	struct dissolve_constants
	{
		DirectX::XMFLOAT4 parameters;	//x:ディゾルブ適応量、yzw:空き
	};
	float dissolve_value{ 0.0f };
	Microsoft::WRL::ComPtr<ID3D11Buffer> dissolve_constant_buffer;

	struct scene_constants
	{
		DirectX::XMFLOAT4X4 view_projection;
		DirectX::XMFLOAT4 options;	//	xy : マウスの座標値, z : タイマー, w : フラグ
		DirectX::XMFLOAT4 camera_position;
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> scene_constant_buffer;

	struct enviroment_constants
	{
		float enviroment_value;
		DirectX::XMFLOAT3 dummy;
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> enviroment_constant_buffer;
	D3D11_TEXTURE2D_DESC enviroment_texture2dDesc;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> environment_texture;
	float encironent_value{ 0.5f };

	struct hemisphere_light_constants
	{
		DirectX::XMFLOAT4 sky_color;
		DirectX::XMFLOAT4 ground_color;
		DirectX::XMFLOAT4 hemisphere_weight;//x : wight, yzw : 空き
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> hemisphere_light_constant_buffer;
	DirectX::XMFLOAT4 sky_color{ 1.0f,0.0f,0.0f,1.0f };
	DirectX::XMFLOAT4 ground_color{ 0.0f,0.0f,1.0f,1.0f };
	float hemisphere_weight{ 0.0f };

	struct fog_constants
	{
		DirectX::XMFLOAT4 fog_color;
		DirectX::XMFLOAT4 fog_range; // x : near, y : far, zw : 空き
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> fog_constant_buffer;
	DirectX::XMFLOAT4 fog_color{ 0.2f,0.2f,0.2f,1.0f };//クリアカラーに設定
	DirectX::XMFLOAT4 fog_range{ 0.1f,100.0f,0,0 };

	struct point_lights
	{
		DirectX::XMFLOAT4 position{ 0, 0, 0, 0 };
		DirectX::XMFLOAT4 color{ 1, 1, 1, 1 };
		float range{ 0 };
		DirectX::XMFLOAT3 dummy;
	};
	struct spot_lights
	{
		DirectX::XMFLOAT4 position{ 0, 0, 0, 0 };
		DirectX::XMFLOAT4 direction{ 0, 0, 1, 0 };
		DirectX::XMFLOAT4 color{ 1, 1, 1, 1 };
		float range{ 0 };
		float innerCorn{ 0.99f };
		float outerCorn{ 0.9f };
		float dummy;
	};
	struct light_constants
	{
		DirectX::XMFLOAT4 ambient_color;
		DirectX::XMFLOAT4 directional_light_direction;
		DirectX::XMFLOAT4 directional_light_color;
		point_lights point_light[8];
		spot_lights spot_light[8];
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> light_constant_buffer;
	DirectX::XMFLOAT4 ambient_color{ 0.2f, 0.2f, 0.2f, 0.2f };
	DirectX::XMFLOAT4 directional_light_direction{ 0.0f, -1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 directional_light_color{ 1.0f, 1.0f, 1.0f, 1.0f };
	point_lights point_light[8];
	spot_lights spot_light[8];

	struct color_filter
	{
		float hueShift;		//色相調整
		float saturation;	//彩度調整
		float brightness;	//明度調整
		float dummy;
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> color_filter_constant_buffer;
	DirectX::XMFLOAT4 color_filter_parameter{ 0.0f,1.0f,1.0f,0.0f };
private:
	D3D11_TEXTURE2D_DESC mask_texture2dDesc;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mask_texture;
	D3D11_TEXTURE2D_DESC ramp_texture2dDesc;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ramp_texture;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> ramp_sampler_state;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> scene_render_target_view;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scene_shader_resource_view;
};

