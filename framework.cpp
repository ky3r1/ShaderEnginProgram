#include "framework.h"

#include "shader.h"
#include "texture.h"

#define PHONGSHADER
//#define RAMPSHADER
//#define ENVIRONMENTMAPPINGSHADER

CONST LONG SHADOWMAP_WIDTH{ 1024 };
CONST LONG SHADOWMAP_HEIGHT{ 1024 };
CONST float SHADOWMAP_DRAWRECT{ 30 };

framework::framework(HWND hwnd) : hwnd(hwnd)
{
}
framework::~framework()
{
}

bool framework::initialize()
{
	HRESULT hr{ S_OK };

	// デバイス＆スワップチェーン生成
	{
		UINT create_device_flags{ 0 };
#ifdef _DEBUG
		create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_FEATURE_LEVEL feature_levels{ D3D_FEATURE_LEVEL_11_0 };

		DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
		swap_chain_desc.BufferCount = 1;
		swap_chain_desc.BufferDesc.Width = SCREEN_WIDTH;
		swap_chain_desc.BufferDesc.Height = SCREEN_HEIGHT;
		swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
		swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
		swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.OutputWindow = hwnd;
		swap_chain_desc.SampleDesc.Count = 1;
		swap_chain_desc.SampleDesc.Quality = 0;
		swap_chain_desc.Windowed = !FULLSCREEN;
		hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, create_device_flags,
			&feature_levels, 1, D3D11_SDK_VERSION, &swap_chain_desc,
			swap_chain.GetAddressOf(), device.GetAddressOf(), NULL, immediate_context.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}
	// レンダーターゲットビューの生成
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer{};
		hr = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(back_buffer.GetAddressOf()));
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		hr = device->CreateRenderTargetView(back_buffer.Get(), NULL, render_target_view.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}
	// デプスステンシルビューの生成
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_stencil_buffer{};
		D3D11_TEXTURE2D_DESC texture2d_desc{};
		texture2d_desc.Width = SCREEN_WIDTH;
		texture2d_desc.Height = SCREEN_HEIGHT;
		texture2d_desc.MipLevels = 1;
		texture2d_desc.ArraySize = 1;
		texture2d_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		texture2d_desc.SampleDesc.Count = 1;
		texture2d_desc.SampleDesc.Quality = 0;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		texture2d_desc.CPUAccessFlags = 0;
		texture2d_desc.MiscFlags = 0;
		hr = device->CreateTexture2D(&texture2d_desc, NULL, depth_stencil_buffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
		depth_stencil_view_desc.Format = texture2d_desc.Format;
		depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depth_stencil_view_desc.Texture2D.MipSlice = 0;
		hr = device->CreateDepthStencilView(depth_stencil_buffer.Get(), &depth_stencil_view_desc, depth_stencil_view.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}
	// シーン描画用のバッファ生成
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> color_buffer{};
		D3D11_TEXTURE2D_DESC texture2d_desc{};
		texture2d_desc.Width = SCREEN_WIDTH;
		texture2d_desc.Height = SCREEN_HEIGHT;
		texture2d_desc.MipLevels = 1;
		texture2d_desc.ArraySize = 1;
		texture2d_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texture2d_desc.SampleDesc.Count = 1;
		texture2d_desc.SampleDesc.Quality = 0;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texture2d_desc.CPUAccessFlags = 0;
		texture2d_desc.MiscFlags = 0;
		hr = device->CreateTexture2D(&texture2d_desc, NULL, color_buffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//	レンダーターゲットビュー生成
		hr = device->CreateRenderTargetView(color_buffer.Get(), NULL, scene_render_target_view.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//	シェーダーリソースビュー生成
		hr = device->CreateShaderResourceView(color_buffer.Get(), NULL, scene_shader_resource_view.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}
	// サンプラステートの生成
	{
		D3D11_SAMPLER_DESC sampler_desc{};
		//画質
		sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

		//ここ変えるとループだったり伸びたりする
		sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

		sampler_desc.MipLODBias = 0;
		sampler_desc.MaxAnisotropy = 16;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		sampler_desc.BorderColor[0] = 0;
		sampler_desc.BorderColor[1] = 0;
		sampler_desc.BorderColor[2] = 0;
		sampler_desc.BorderColor[3] = 0;
		sampler_desc.MinLOD = 0;
		sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
		hr = device->CreateSamplerState(&sampler_desc, sampler_state.GetAddressOf());
	}
	// 深度ステンシルステートの生成
	{
		D3D11_DEPTH_STENCIL_DESC depth_stencil_desc{};
		depth_stencil_desc.DepthEnable = TRUE;
		depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		hr = device->CreateDepthStencilState(&depth_stencil_desc, depth_stencil_state.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}
	// ブレンドステートの生成
	{
		// アルファブレンド
		D3D11_BLEND_DESC blend_desc{};
		blend_desc.AlphaToCoverageEnable = FALSE;
		blend_desc.IndependentBlendEnable = FALSE;
		blend_desc.RenderTarget[0].BlendEnable = TRUE;
		blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		hr = device->CreateBlendState(&blend_desc, blend_state.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}
	// ラスタライザーステートの生成
	{
		D3D11_RASTERIZER_DESC rasterizer_desc{};
		rasterizer_desc.FillMode = D3D11_FILL_SOLID;
		rasterizer_desc.CullMode = D3D11_CULL_BACK;
		rasterizer_desc.FrontCounterClockwise = FALSE;
		rasterizer_desc.DepthBias = 0;
		rasterizer_desc.DepthBiasClamp = 0;
		rasterizer_desc.SlopeScaledDepthBias = 0;
		rasterizer_desc.DepthClipEnable = TRUE;
		rasterizer_desc.ScissorEnable = FALSE;
		rasterizer_desc.MultisampleEnable = FALSE;
		rasterizer_desc.AntialiasedLineEnable = FALSE;
		hr = device->CreateRasterizerState(&rasterizer_desc, rasterizer_state.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}
	// 定数バッファの生成
	{
		D3D11_BUFFER_DESC buffer_desc{};
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;
		{
			buffer_desc.ByteWidth = sizeof(scroll_constants);
			hr = device->CreateBuffer(&buffer_desc, nullptr, scroll_constant_buffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
		{
			buffer_desc.ByteWidth = sizeof(scene_constants);
			hr = device->CreateBuffer(&buffer_desc, nullptr, scene_constant_buffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
		{
			buffer_desc.ByteWidth = sizeof(dissolve_constants);
			hr = device->CreateBuffer(&buffer_desc, nullptr, dissolve_constant_buffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
		{
			buffer_desc.ByteWidth = sizeof(light_constants);
			hr = device->CreateBuffer(&buffer_desc, nullptr, light_constant_buffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
		{
			buffer_desc.ByteWidth = sizeof(enviroment_constants);
			hr = device->CreateBuffer(&buffer_desc, nullptr, enviroment_constant_buffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
		{
			buffer_desc.ByteWidth = sizeof(hemisphere_light_constants);
			hr = device->CreateBuffer(&buffer_desc, nullptr, hemisphere_light_constant_buffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
		{
			buffer_desc.ByteWidth = sizeof(fog_constants);
			hr = device->CreateBuffer(&buffer_desc, nullptr, fog_constant_buffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
		{
			buffer_desc.ByteWidth = sizeof(color_filter);
			hr = device->CreateBuffer(&buffer_desc, nullptr, color_filter_constant_buffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
		{
			buffer_desc.ByteWidth = sizeof(shadowmap_constants);
			hr = device->CreateBuffer(&buffer_desc, nullptr, shadowmap_constant_buffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
	}	//ライトから見たシーンの深度描画用のバッファ生成
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_buffer{};
		D3D11_TEXTURE2D_DESC texture2d_desc{};
		texture2d_desc.Width = SHADOWMAP_WIDTH;
		texture2d_desc.Height = SHADOWMAP_HEIGHT;
		texture2d_desc.MipLevels = 1;
		texture2d_desc.ArraySize = 1;
		texture2d_desc.Format = DXGI_FORMAT_R32_TYPELESS;
		texture2d_desc.SampleDesc.Count = 1;
		texture2d_desc.SampleDesc.Quality = 0;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		texture2d_desc.CPUAccessFlags = 0;
		texture2d_desc.MiscFlags = 0;
		hr = device->CreateTexture2D(&texture2d_desc, NULL, depth_buffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		//深度ステンシルビュー生成
		D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
		depth_stencil_view_desc.Format = DXGI_FORMAT_D32_FLOAT;
		depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depth_stencil_view_desc.Texture2D.MipSlice = 0;
		hr = device->CreateDepthStencilView(depth_buffer.Get(),
			&depth_stencil_view_desc,
			shadowmap_depth_stencil_view.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		//シェーダーリソースビュー生成
		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc{};
		shader_resource_view_desc.Format = DXGI_FORMAT_R32_FLOAT;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_desc.Texture2D.MostDetailedMip = 0;
		shader_resource_view_desc.Texture2D.MipLevels = 1;
		hr = device->CreateShaderResourceView(depth_buffer.Get(),
			&shader_resource_view_desc,
			shadowmap_shader_resource_view.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		// サンプラステートの生成
		{
			D3D11_SAMPLER_DESC sampler_desc{};
			sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
			sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
			sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
			sampler_desc.MipLODBias = 0;
			sampler_desc.MaxAnisotropy = 16;
			sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			sampler_desc.BorderColor[0] = FLT_MAX;
			sampler_desc.BorderColor[1] = FLT_MAX;
			sampler_desc.BorderColor[2] = FLT_MAX;
			sampler_desc.BorderColor[3] = FLT_MAX;
			sampler_desc.MinLOD = 0;
			sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
			hr = device->CreateSamplerState(&sampler_desc, shadowmap_sampler_state.GetAddressOf());
		}
	}
	// 描画オブジェクトの読み込み
	{
		//dummy_static_mesh = std::make_unique<static_mesh>(device.Get(), L".\\resources\\ball\\ball.obj", true);
		//dummy_static_mesh = std::make_unique<static_mesh>(device.Get(), L".\\resources\\globe\\globe.obj", true);
		//scaling.x = 0.01f;
		//scaling.y = 0.01f;
		//scaling.z = 0.01f;
		dummy_static_meshes.push_back(std::make_unique<static_mesh>(device.Get(), L".\\resources\\ball\\ball.obj", true));
		dummy_static_meshes.push_back(std::make_unique<static_mesh>(device.Get(), L".\\resources\\plane\\plane.obj", true));

		//dummy_sprite = std::make_unique<sprite>(device.Get(), L".\\resources\\chip_win.png");
		dummy_sprite = std::make_unique<sprite>(device.Get(), scene_shader_resource_view);
		//load_texture_from_file(device.Get(), L".\\resources\\mask\\dissolve_animation.png", mask_texture.GetAddressOf(), &mask_texture2dDesc);
		load_texture_from_file(device.Get(), L".\\resources\\ramp.png", ramp_texture.GetAddressOf(), &ramp_texture2dDesc);

		//サンプラーステート生成
		D3D11_SAMPLER_DESC sampler_desc{};
		sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.MaxAnisotropy = 16;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		sampler_desc.MinLOD = 0;
		sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
		hr = device->CreateSamplerState(&sampler_desc, ramp_sampler_state.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		load_texture_from_file(device.Get(), L".\\resources\\SphereMap.bmp", environment_texture.GetAddressOf(), &enviroment_texture2dDesc);
	}
	//ポイントライト・スポットライトの初期位置を設定
	{
		//point_light
		point_light[0].position.x = 10;
		point_light[0].position.y = 1;
		point_light[0].range = 10;
		point_light[0].color = { 1,0,0,1 };

		point_light[1].position.x = -10;
		point_light[1].position.y = 1;
		point_light[1].range = 10;
		point_light[1].color = { 0,1,0,1 };

		point_light[2].position.y = 1;
		point_light[2].position.z = 10;
		point_light[2].range = 10;
		point_light[2].color = { 0,0,1,1 };

		point_light[3].position.y = 1;
		point_light[3].position.z = -10;
		point_light[3].range = 10;
		point_light[3].color = { 1,1,1,1 };

		point_light[4].range = 10;
		point_light[4].color = { 1,1,1,1 };

		ZeroMemory(&point_light[5], sizeof(point_lights) * 3);

		//spot_light
		spot_light[0].position = { 15,3,15,0 };
		spot_light[0].direction = { -1,-1,-1,0 };
		spot_light[0].range = 100;
		spot_light[0].color = { 1,0,0,1 };

		spot_light[1].position = { -15,3,15,0 };
		spot_light[1].direction = { +1,-1,-1,0 };
		spot_light[1].range = 100;
		spot_light[1].color = { 0,1,0,1 };

		spot_light[2].position = { 15,3,-15,0 };
		spot_light[2].direction = { -1,-1,+1,0 };
		spot_light[2].range = 100;
		spot_light[2].color = { 0,0,1,1 };

		spot_light[3].position = { -15,3,-15,0 };
		spot_light[3].direction = { +1,-1,+1,0 };
		spot_light[3].range = 100;
		spot_light[3].color = { 1,1,1,1 };

		ZeroMemory(&spot_light[4], sizeof(spot_lights) * 4);
	}
	// シェーダーの読み込み
	{
		// static_mesh用デフォルト描画シェーダー
		{
			D3D11_INPUT_ELEMENT_DESC input_element_desc[]
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
#ifdef PHONGSHADER
			create_vs_from_cso(device.Get(),
				"phong_shader_vs.cso",
				mesh_vertex_shader.GetAddressOf(),
				mesh_input_layout.GetAddressOf(),
				input_element_desc,
				ARRAYSIZE(input_element_desc));
			create_ps_from_cso(device.Get(),
				"phong_shader_ps.cso",
				mesh_pixel_shader.GetAddressOf());
#endif // PHONGSHADER

#ifdef RAMPSHADER
			create_vs_from_cso(device.Get(),
				"ramp_shader_vs.cso",
				mesh_vertex_shader.GetAddressOf(),
				mesh_input_layout.GetAddressOf(),
				input_element_desc,
				ARRAYSIZE(input_element_desc));
			create_ps_from_cso(device.Get(),
				"ramp_shader_ps.cso",
				mesh_pixel_shader.GetAddressOf());
#endif // RAMPSHADER

#ifdef ENVIRONMENTMAPPINGSHADER
			create_vs_from_cso(device.Get(),
				"environment_mapping_shader_vs.cso",
				mesh_vertex_shader.GetAddressOf(),
				mesh_input_layout.GetAddressOf(),
				input_element_desc,
				ARRAYSIZE(input_element_desc));
			create_ps_from_cso(device.Get(),
				"environment_mapping_shader_ps.cso",
				mesh_pixel_shader.GetAddressOf());
#endif // ENVIRONMENTMAPPINGSHADER

			//シャドウマップ生成用シェーダー
			create_vs_from_cso(device.Get(),
				"shadowmap_caster_vs.cso",
				shadowmap_caster_vertex_shader.GetAddressOf(),
				shadowmap_caster_input_layout.GetAddressOf(),
				input_element_desc,
				ARRAYSIZE(input_element_desc));
		}
		// sprite用デフォルト描画シェーダー
		{
			D3D11_INPUT_ELEMENT_DESC input_element_desc[]
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			create_vs_from_cso(device.Get(),
				"UVScroll_vs.cso",
				sprite_vertex_shader.GetAddressOf(),
				sprite_input_layout.GetAddressOf(),
				input_element_desc,
				_countof(input_element_desc));
			create_ps_from_cso(device.Get(),
				"UVScroll_ps.cso",
				sprite_pixel_shader.GetAddressOf());

			create_vs_from_cso(device.Get(), "color_filter_vs.cso", sprite_vertex_shader.GetAddressOf(),
				sprite_input_layout.GetAddressOf(), input_element_desc, _countof(input_element_desc));
			create_ps_from_cso(device.Get(), "color_filter_ps.cso", sprite_pixel_shader.GetAddressOf());

		}

	}
	return true;
}

bool framework::uninitialize()
{
	return true;
}


void framework::update(float elapsed_time/*Elapsed seconds from last frame*/)
{
	// 時間経過更新
	timer += elapsed_time;

#ifdef USE_IMGUI
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif


#ifdef USE_IMGUI
	ImGui::Begin("ImGUI");

	ImGui::SliderFloat3("translation", &translation.x, -10.0f, +10.0f);
	ImGui::SliderFloat3("scaling", &scaling.x, 0.010f, +10.0f);
	ImGui::SliderFloat3("rotation", &rotation.x, -10.0f, +10.0f);
	ImGui::ColorEdit4("material_color", reinterpret_cast<float*>(&material_color));
	ImGui::Separator();
	ImGui::ColorEdit3("ambient_color", &ambient_color.x);
	ImGui::SliderFloat3("direction_color", &directional_light_direction.x, -1.0f, +1.0f);
	ImGui::ColorEdit3("direction_light_color", &directional_light_color.x);
	ImGui::Separator();
	ImGui::SliderFloat("environment_value", &encironent_value, 0.0f, +1.0f);
	ImGui::Separator();
	ImGui::ColorEdit3("sky_color", &sky_color.x);
	ImGui::ColorEdit3("ground_color", &ground_color.x);
	ImGui::SliderFloat("hemisphere_weight", &hemisphere_weight, 0.0f, 1.0f);
	ImGui::Separator();
	ImGui::ColorEdit3("fog_color", &fog_color.x);
	ImGui::SliderFloat("fog_near", &fog_range.x, 0.1f, +100.0f);
	ImGui::SliderFloat("fog_far", &fog_range.y, 0.1f, +100.0f);
	ImGui::Separator();
	ImGui::SliderFloat("hueShift", &color_filter_parameter.x, 0.0f, +360.0f);
	ImGui::SliderFloat("saturation", &color_filter_parameter.y, 0.0f, +2.0f);
	ImGui::Separator();
	ImGui::SliderFloat3("directional_light_direction", &directional_light_direction.x, -1.0f, +1.0f);
	ImGui::ColorEdit3("directional_light_color", &directional_light_color.x);
	ImGui::Separator();
	ImGui::ColorEdit3("shadow_color", &shadow_color.x);
	ImGui::SliderFloat("shadow_bias", &shadow_bias, 0.0f, +0.01f);
	ImGui::Separator();
	if (ImGui::TreeNode("texture"))
	{
		ImGui::Text("scene_texture");
		ImGui::Image(scene_shader_resource_view.Get(), { 256, 144 }, { 0, 0 }, { 1, 1 }, { 1, 1, 1, 1 });
		ImGui::Text("shadow_map");
		ImGui::Image(shadowmap_shader_resource_view.Get(), { 256, 256 }, { 0, 0 }, { 1, 1 }, { 1, 1, 1, 1 });

		ImGui::TreePop();
	}
	if (ImGui::TreeNode("points"))
	{
		for (int i = 0; i < 8; ++i)
		{
			std::string p = std::string("position") + std::to_string(i);
			ImGui::SliderFloat3(p.c_str(), &point_light[i].position.x, -10.0f, +10.0f);
			std::string c = std::string("color") + std::to_string(i);
			ImGui::ColorEdit3(c.c_str(), &point_light[i].color.x);
			std::string r = std::string("range") + std::to_string(i);
			ImGui::SliderFloat(r.c_str(), &point_light[i].range, 0.0f, +100.0f);
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("spots"))
	{
		for (int i = 0; i < 8; ++i)
		{
			std::string p = std::string("position") + std::to_string(i);
			ImGui::SliderFloat3(p.c_str(), &spot_light[i].position.x, -10.0f, +10.0f);
			std::string d = std::string("direction") + std::to_string(i);
			ImGui::SliderFloat3(d.c_str(), &spot_light[i].direction.x, 0.0f, +1.0f);
			std::string c = std::string("color") + std::to_string(i);
			ImGui::ColorEdit3(c.c_str(), &spot_light[i].color.x);
			std::string r = std::string("range") + std::to_string(i);
			ImGui::SliderFloat(r.c_str(), &spot_light[i].range, 0.0f, +1000.0f);
			std::string ic = std::string("inner") + std::to_string(i);
			ImGui::SliderFloat(ic.c_str(), &spot_light[i].innerCorn, -1.0f, +1.0f);
			std::string oc = std::string("outer") + std::to_string(i);
			ImGui::SliderFloat(oc.c_str(), &spot_light[i].outerCorn, -1.0f, +1.0f);
		}
		ImGui::TreePop();
	}

	//ImGui::SliderFloat2("scroll_direction", &scroll_direction.x, -4.0f, +4.0f);
	//ImGui::SliderFloat("scroll_value", &dissolve_value, 0.0f, +1.0f);
	ImGui::End();
#endif
}


void framework::render(float elapsed_time/*Elapsed seconds from last frame*/)
{
	HRESULT hr{ S_OK };
	//シャドウマップ生成処理
	{
		//シャドウマップ用の深度バッファに設定
		immediate_context->ClearDepthStencilView(shadowmap_depth_stencil_view.Get(),D3D11_CLEAR_DEPTH| D3D11_CLEAR_STENCIL, 1.0f, 0);
		immediate_context->OMSetRenderTargets(0, nullptr, shadowmap_depth_stencil_view.Get());
		//ビューポートの設定
		D3D11_VIEWPORT viewport{};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = static_cast<float>(SHADOWMAP_WIDTH);
		viewport.Height = static_cast<float>(SHADOWMAP_HEIGHT);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		immediate_context->RSSetViewports(1, &viewport);
		//ブレンドステートの設定
		immediate_context->OMSetBlendState(blend_state.Get(), nullptr, 0xFFFFFFFF);
		//深度ステンシルステートの設定
		immediate_context->OMSetDepthStencilState(depth_stencil_state.Get(), 0);
		//ラスタライザーステートの設定
		immediate_context->RSSetState(rasterizer_state.Get());
		//シェーダーの設定
		immediate_context->IASetInputLayout(shadowmap_caster_input_layout.Get());
		immediate_context->VSSetShader(shadowmap_caster_vertex_shader.Get(), nullptr, 0);
		immediate_context->PSSetShader(nullptr, nullptr, 0);

		DirectX::XMMATRIX S, R, T;
		DirectX::XMFLOAT4X4 world;
		{
			//ライトの位置から見た視線行列を生成
			DirectX::XMVECTOR LightPosition = DirectX::XMLoadFloat4(&directional_light_direction);
			LightPosition = DirectX::XMVectorScale(LightPosition, -50);
			DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(LightPosition,
				DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
				DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
			//シャドウマップに描画したい範囲の射影行列を生成
			DirectX::XMMATRIX P = DirectX::XMMatrixOrthographicLH(SHADOWMAP_DRAWRECT, SHADOWMAP_DRAWRECT, 0.1f, 200.0f);
			//ライトビュー行列を保存
			DirectX::XMStoreFloat4x4(&light_view_projection, V * P);

			//定数バッファの更新
			{
				// 0番はメッシュ側で更新している
				
				//1番
				scene_constants scene{};
				scene.options.x = cursor_position.x;
				scene.options.y = cursor_position.y;
				scene.options.z = timer;
				scene.options.w = flag;
				//DirectX::XMStoreFloat4x4(&scene.view_projection, V * P);
				scene.view_projection = light_view_projection;
				immediate_context->UpdateSubresource(scene_constant_buffer.Get(), 0, 0, &scene, 0, 0);
				immediate_context->VSSetConstantBuffers(1, 1, scene_constant_buffer.GetAddressOf());
				immediate_context->PSSetConstantBuffers(1, 1, scene_constant_buffer.GetAddressOf());
			}
			//大量モデル描画
			for (int x = -10; x < 10; ++x)
			{
				for (int z = 0; z < 75; ++z)
				{
					S = DirectX::XMMatrixScaling(0.01f * scaling.x, 0.01f * scaling.y, 0.01f * scaling.z);
					R = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
					T = DirectX::XMMatrixTranslation(translation.x + (static_cast<float>(x) * 3),
						translation.y,
						translation.z + (static_cast<float>(z) * 3));
					DirectX::XMStoreFloat4x4(&world, S * R * T);
					dummy_static_meshes[0]->render(immediate_context.Get(), world, material_color);
				}
			}
		}
	}
	// レンダーターゲット等の設定とクリア
	FLOAT color[]{ 0.2f, 0.2f, 0.2f, 1.0f };
	immediate_context->ClearRenderTargetView(scene_render_target_view.Get(), color);
	immediate_context->ClearDepthStencilView(depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	immediate_context->OMSetRenderTargets(1, scene_render_target_view.GetAddressOf(), depth_stencil_view.Get());


	// ビューポートの設定
	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(SCREEN_WIDTH);
	viewport.Height = static_cast<float>(SCREEN_HEIGHT);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	immediate_context->RSSetViewports(1, &viewport);
	// ブレンドステートの設定
	immediate_context->OMSetBlendState(blend_state.Get(), nullptr, 0xFFFFFFFF);
	// 深度ステンシルステートの設定
	immediate_context->OMSetDepthStencilState(depth_stencil_state.Get(), 0);
	// ラスタライザーステートの設定
	immediate_context->RSSetState(rasterizer_state.Get());
	// 視線行列を生成
	DirectX::XMMATRIX V;
	{
		DirectX::XMVECTOR up{ DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };
		// マウス操作
		{
			POINT cursor;
			RECT rc;
			::GetCursorPos(&cursor);
			::ScreenToClient(hwnd, &cursor);
			GetClientRect(hwnd, &rc);
			UINT screenW = rc.right - rc.left;
			UINT screenH = rc.bottom - rc.top;
			POINT old_cursor_position;
			old_cursor_position.x = cursor_position.x;
			old_cursor_position.y = cursor_position.y;
			cursor_position.x = (LONG)(cursor.x / static_cast<float>( SCREEN_WIDTH) * static_cast<float>(screenW));
			cursor_position.y = (LONG)(cursor.y / static_cast<float>(SCREEN_HEIGHT) * static_cast<float>(screenH));

			float moveX = (cursor_position.x - old_cursor_position.x) * 0.02f;
			float moveY = (cursor_position.y - old_cursor_position.y) * 0.02f;
			if (::GetAsyncKeyState(VK_RBUTTON) & 0x8000)
			{
				// Y軸回転
				rotateY += moveX * 0.5f;
				if (rotateY > DirectX::XM_PI)
					rotateY -= DirectX::XM_2PI;
				else if (rotateY < -DirectX::XM_PI)
					rotateY += DirectX::XM_2PI;
				// X軸回転
				rotateX += moveY * 0.5f;
				if (rotateX > DirectX::XMConvertToRadians(89.9f))
					rotateX = DirectX::XMConvertToRadians(89.9f);
				else if (rotateX < -DirectX::XMConvertToRadians(89.9f))
					rotateX = -DirectX::XMConvertToRadians(89.9f);
			}
			else if (::GetAsyncKeyState(VK_MBUTTON) & 0x8000)
			{
				V = DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&camera_position),
											  DirectX::XMLoadFloat3(&camera_focus),
											  up);
				DirectX::XMFLOAT4X4 W;
				DirectX::XMStoreFloat4x4(&W, DirectX::XMMatrixInverse(nullptr, V));
				// 平行移動
				float s = distance * 0.035f;
				float x = moveX * s;
				float y = moveY * s;
				camera_focus.x -= W._11 * x;
				camera_focus.y -= W._12 * x;
				camera_focus.z -= W._13 * x;

				camera_focus.x += W._21 * y;
				camera_focus.y += W._22 * y;
				camera_focus.z += W._23 * y;
			}
			if (wheel != 0)	// ズーム
			{
				distance -= static_cast<float>(wheel) * distance * 0.001f;
				wheel = 0;
			}
		}
		float sx = ::sinf(rotateX), cx = ::cosf(rotateX);
		float sy = ::sinf(rotateY), cy = ::cosf(rotateY);
		DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&camera_focus);
		DirectX::XMVECTOR Front = DirectX::XMVectorSet(-cx * sy, -sx, -cx * cy, 0.0f);
		DirectX::XMVECTOR Distance = DirectX::XMVectorSet(distance, distance, distance, 0.0f);
		Front = DirectX::XMVectorMultiply(Front, Distance);
		DirectX::XMVECTOR Eye = DirectX::XMVectorSubtract(Focus, Front);
		DirectX::XMStoreFloat3(&camera_position, Eye);
		V = DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&camera_position),
									  DirectX::XMLoadFloat3(&camera_focus),
									  up);
	}
	// 射影行列を生成
	DirectX::XMMATRIX P;
	{
		D3D11_VIEWPORT viewport;
		UINT num_viewports{ 1 };
		immediate_context->RSGetViewports(&num_viewports, &viewport);
		float aspect_ratio{ viewport.Width / viewport.Height };
		P = DirectX::XMMatrixPerspectiveFovLH(  DirectX::XMConvertToRadians(30),
												aspect_ratio,
												0.1f,
												100.0f);
	}

	// 定数バッファの更新
	{
		// 0番はメッシュ側で更新している

		//1番
		scene_constants scene{};
		scene.options.x = cursor_position.x;
		scene.options.y = cursor_position.y;
		scene.options.z = timer;
		scene.options.w = flag;
		scene.camera_position.x = camera_position.x;
		scene.camera_position.y = camera_position.y;
		scene.camera_position.z = camera_position.z;
		DirectX::XMStoreFloat4x4(&scene.view_projection, V * P);
		immediate_context->UpdateSubresource(scene_constant_buffer.Get(), 0, 0, &scene, 0, 0);
		immediate_context->VSSetConstantBuffers(1, 1, scene_constant_buffer.GetAddressOf());
		immediate_context->PSSetConstantBuffers(1, 1, scene_constant_buffer.GetAddressOf());

		//2番
		//scroll_constants scroll{};
		//scroll.scroll_direction.x = scroll_direction.x;
		//scroll.scroll_direction.y = scroll_direction.y;
		//immediate_context->UpdateSubresource(scroll_constant_buffer.Get(), 0, 0, &scroll, 0, 0);
		//immediate_context->VSSetConstantBuffers(2, 1, scroll_constant_buffer.GetAddressOf());
		//immediate_context->PSSetConstantBuffers(2, 1, scroll_constant_buffer.GetAddressOf());

		//light_constants lights{};
		//lights.ambient_color = ambient_color;
		//lights.directional_light_direction = directional_light_direction;
		//lights.directional_light_color = directional_light_color;
		//immediate_context->UpdateSubresource(light_constant_buffer.Get(), 0, 0, &lights, 0, 0);
		//immediate_context->VSSetConstantBuffers(2, 1, light_constant_buffer.GetAddressOf());
		//immediate_context->PSSetConstantBuffers(2, 1, light_constant_buffer.GetAddressOf());

		light_constants lights{};
		lights.ambient_color = ambient_color;
		lights.directional_light_direction = directional_light_direction;
		lights.directional_light_color = directional_light_color;
		memcpy_s(lights.point_light, sizeof(lights.point_light), point_light, sizeof(point_light));
		memcpy_s(lights.spot_light, sizeof(lights.spot_light), spot_light, sizeof(spot_light));
		immediate_context->UpdateSubresource(light_constant_buffer.Get(), 0, 0, &lights, 0, 0);
		immediate_context->VSSetConstantBuffers(2, 1, light_constant_buffer.GetAddressOf());
		immediate_context->PSSetConstantBuffers(2, 1, light_constant_buffer.GetAddressOf());

		//3番
		//dissolve_constants dissolve{};
		//dissolve.parameters.x = dissolve_value;
		//immediate_context->UpdateSubresource(dissolve_constant_buffer.Get(), 0, 0, &dissolve, 0, 0);
		//immediate_context->VSSetConstantBuffers(3, 1, dissolve_constant_buffer.GetAddressOf());
		//immediate_context->PSSetConstantBuffers(3, 1, dissolve_constant_buffer.GetAddressOf());

		enviroment_constants enviroments{};
		enviroments.enviroment_value = encironent_value;
		immediate_context->UpdateSubresource(enviroment_constant_buffer.Get(), 0, 0, &enviroments, 0, 0);
		immediate_context->VSSetConstantBuffers(3, 1, enviroment_constant_buffer.GetAddressOf());
		immediate_context->PSSetConstantBuffers(3, 1, enviroment_constant_buffer.GetAddressOf());

		//4番
		hemisphere_light_constants hemisphere_lights{};
		hemisphere_lights.sky_color = sky_color;
		hemisphere_lights.ground_color = ground_color;
		hemisphere_lights.hemisphere_weight.x = hemisphere_weight;
		immediate_context->UpdateSubresource(hemisphere_light_constant_buffer.Get(), 0, 0, &hemisphere_lights, 0, 0);
		immediate_context->VSSetConstantBuffers(4, 1, hemisphere_light_constant_buffer.GetAddressOf());
		immediate_context->PSSetConstantBuffers(4, 1, hemisphere_light_constant_buffer.GetAddressOf());


		//5番
		fog_constants fogs{};
		fogs.fog_color = fog_color;
		fogs.fog_range = fog_range;
		immediate_context->UpdateSubresource(fog_constant_buffer.Get(), 0, 0, &fogs, 0, 0);
		immediate_context->VSSetConstantBuffers(5, 1, fog_constant_buffer.GetAddressOf());
		immediate_context->PSSetConstantBuffers(5, 1, fog_constant_buffer.GetAddressOf());

		//6番
		shadowmap_constants shadowmap{};
		shadowmap.light_view_projection = light_view_projection;
		shadowmap.shadow_color = shadow_color;
		shadowmap.shadow_bias = shadow_bias;
		immediate_context->UpdateSubresource(shadowmap_constant_buffer.Get(), 0, 0, &shadowmap, 0, 0);
		immediate_context->VSSetConstantBuffers(6, 1, shadowmap_constant_buffer.GetAddressOf());
		immediate_context->PSSetConstantBuffers(6, 1, shadowmap_constant_buffer.GetAddressOf());
	}

	// static_mesh描画
	//if(dummy_static_mesh)
	{
		immediate_context->IASetInputLayout(mesh_input_layout.Get());
		immediate_context->VSSetShader(mesh_vertex_shader.Get(), nullptr, 0);
		immediate_context->PSSetShader(mesh_pixel_shader.Get(), nullptr, 0);
		immediate_context->PSSetSamplers(0, 1, sampler_state.GetAddressOf());

		//t0・t1はstatic_meshのrender関数側で設定されるので t2 から設定する
		immediate_context->PSSetShaderResources(2, 1, ramp_texture.GetAddressOf());
		immediate_context->PSSetSamplers(2, 1, ramp_sampler_state.GetAddressOf());

		immediate_context->PSSetShaderResources(3, 1, environment_texture.GetAddressOf());
		immediate_context->PSSetShaderResources(4, 1, shadowmap_shader_resource_view.GetAddressOf());
		immediate_context->PSSetSamplers(4, 1, shadowmap_sampler_state.GetAddressOf());

		//DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.z) };
		//DirectX::XMMATRIX R{ DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) };
		//DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z) };
		//DirectX::XMFLOAT4X4 world;
		//DirectX::XMStoreFloat4x4(&world, S * R * T);
		//dummy_static_mesh->render(immediate_context.Get(), world, material_color);
		DirectX::XMMATRIX S, R, T;
		DirectX::XMFLOAT4X4 world;
		//大量のモデルを表示
		for (int x = -10; x < 10; ++x)
		{
			for (int z = 0; z < 75; ++z)
			{
				S = { DirectX::XMMatrixScaling(0.01f * scaling.x, 0.01f * scaling.y, 0.01f * scaling.z) };
				R={ DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) };
				T = { DirectX::XMMatrixTranslation(translation.x + static_cast<float>(x) * 3, translation.y, translation.z + static_cast<float>(z) * 3) };
				DirectX::XMStoreFloat4x4(&world, S * R * T);
				dummy_static_meshes[0]->render(immediate_context.Get(), world, material_color);
			}
		}
		//平面モデルを表示
		S = { DirectX::XMMatrixScaling(100.0f * scaling.x, 100.0f * scaling.y, 100.0f * scaling.z) };
		R = { DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) };
		T = { DirectX::XMMatrixTranslation(translation.x, translation.y-1, translation.z ) };
		DirectX::XMStoreFloat4x4(&world, S * R * T);
		dummy_static_meshes[1]->render(immediate_context.Get(), world, material_color);
		ID3D11ShaderResourceView* clear_shader_resource_view[]{ nullptr };
		immediate_context->PSSetShaderResources(4, 1, clear_shader_resource_view);
	}
	//	バックバッファに描画先を戻して描画する
	immediate_context->ClearRenderTargetView(render_target_view.Get(), color);
	immediate_context->ClearDepthStencilView(depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	immediate_context->OMSetRenderTargets(1, render_target_view.GetAddressOf(), depth_stencil_view.Get());

	// sprite描画
	if(dummy_sprite)
	{
		immediate_context->IASetInputLayout(sprite_input_layout.Get());
		immediate_context->PSSetShaderResources(1, 1, mask_texture.GetAddressOf());
		immediate_context->VSSetShader(sprite_vertex_shader.Get(), nullptr, 0);
		immediate_context->PSSetShader(sprite_pixel_shader.Get(), nullptr, 0);
		immediate_context->PSSetSamplers(0, 1, sampler_state.GetAddressOf());

		color_filter filter{};
		filter.hueShift = color_filter_parameter.x;
		filter.saturation = color_filter_parameter.y;
		filter.brightness = color_filter_parameter.z;
		immediate_context->UpdateSubresource(color_filter_constant_buffer.Get(), 0, 0, &filter, 0, 0);
		immediate_context->VSSetConstantBuffers(4, 1, color_filter_constant_buffer.GetAddressOf());
		immediate_context->PSSetConstantBuffers(4, 1, color_filter_constant_buffer.GetAddressOf());

		dummy_sprite->render(immediate_context.Get(), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	}

#ifdef USE_IMGUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif

	UINT sync_interval{ 0 };
	swap_chain->Present(sync_interval, 0);
}