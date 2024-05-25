#include "framework.h"

#include "shader.h"
#include "texture.h"

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
		// 描画オブジェクトの読み込み
		{
			dummy_static_mesh = std::make_unique<static_mesh>(device.Get(), L".\\resources\\ball\\ball.obj", true);
			scaling.x = 0.01f;
			scaling.y = 0.01f;
			scaling.z = 0.01f;
			//dummy_sprite = std::make_unique<sprite>(device.Get(), L".\\resources\\chip_win.png");
			//load_texture_from_file(device.Get(), L".\\resources\\mask\\dissolve_animation.png", mask_texture.GetAddressOf(), &mask_texture2dDesc);
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
#if false
				create_vs_from_cso(device.Get(),
					"static_mesh_vs.cso",
					mesh_vertex_shader.GetAddressOf(),
					mesh_input_layout.GetAddressOf(),
					input_element_desc,
					ARRAYSIZE(input_element_desc));
				create_ps_from_cso(device.Get(),
					"static_mesh_ps.cso",
					mesh_pixel_shader.GetAddressOf());
#else
				create_vs_from_cso(device.Get(),
					"phong_shader_vs.cso",
					mesh_vertex_shader.GetAddressOf(),
					mesh_input_layout.GetAddressOf(),
					input_element_desc,
					ARRAYSIZE(input_element_desc));
				create_ps_from_cso(device.Get(),
					"phong_shader_ps.cso",
					mesh_pixel_shader.GetAddressOf());
#endif			
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

				create_vs_from_cso(device.Get(),
					"sprite_dissolve_vs.cso",
					sprite_vertex_shader.GetAddressOf(),
					sprite_input_layout.GetAddressOf(),
					input_element_desc,
					ARRAYSIZE(input_element_desc));
				create_ps_from_cso(device.Get(),
					"sprite_dissolve_ps.cso",
					sprite_pixel_shader.GetAddressOf());
			}

		}

		return true;
	}
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
	ImGui::SliderFloat3("scaling", &scaling.x, -10.0f, +10.0f);
	ImGui::SliderFloat3("rotation", &rotation.x, -10.0f, +10.0f);
	ImGui::ColorEdit4("material_color", reinterpret_cast<float*>(&material_color));
	ImGui::Checkbox("utility flag", &flag); 
	ImGui::Separator();
	ImGui::ColorEdit3("ambient_color", &ambient_color.x);
	ImGui::SliderFloat3("direction_color", &directional_light_direction.x, -1.0f, +1.0f);
	ImGui::ColorEdit3("direction_light_color", &directional_light_color.x);


	//ImGui::SliderFloat2("scroll_direction", &scroll_direction.x, -4.0f, +4.0f);
	//ImGui::SliderFloat("scroll_value", &dissolve_value, 0.0f, +1.0f);
	ImGui::End();
#endif
}


void framework::render(float elapsed_time/*Elapsed seconds from last frame*/)
{
	HRESULT hr{ S_OK };

	// レンダーターゲット等の設定とクリア
	FLOAT color[]{ 0.2f, 0.2f, 0.2f, 1.0f };
	immediate_context->ClearRenderTargetView(render_target_view.Get(), color);
	immediate_context->ClearDepthStencilView(depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	immediate_context->OMSetRenderTargets(1, render_target_view.GetAddressOf(), depth_stencil_view.Get());

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
	// static_mesh描画
	if(dummy_static_mesh)
	{
		//定数バッファの更新
		{
			scroll_constants scroll{};
			scroll.scroll_direction.x = scroll_direction.x;
			scroll.scroll_direction.y = scroll_direction.y;
			immediate_context->UpdateSubresource(scroll_constant_buffer.Get(), 0, 0, &scroll, 0, 0);
			immediate_context->VSSetConstantBuffers(2, 1, scroll_constant_buffer.GetAddressOf());
			immediate_context->PSSetConstantBuffers(2, 1, scroll_constant_buffer.GetAddressOf());

			//light_constants lights{};
			//lights.ambient_color = ambient_color;
			//lights.directional_light_color = direction_light_color;
			//lights.directional_light_direction = direction_light_direction;
			//immediate_context->UpdateSubresource(light_constant_buffer.Get(), 0, 0, &lights, 0, 0);
			//immediate_context->VSSetConstantBuffers(2, 1, light_constant_buffer.GetAddressOf());
			//immediate_context->PSSetConstantBuffers(2, 1, light_constant_buffer.GetAddressOf());
		}

		immediate_context->IASetInputLayout(mesh_input_layout.Get());
		immediate_context->VSSetShader(mesh_vertex_shader.Get(), nullptr, 0);
		immediate_context->PSSetShader(mesh_pixel_shader.Get(), nullptr, 0);
		immediate_context->PSSetSamplers(0, 1, sampler_state.GetAddressOf());

		DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.z) };
		DirectX::XMMATRIX R{ DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) };
		DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z) };
		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(&world, S * R * T);
		dummy_static_mesh->render(immediate_context.Get(), world, material_color);
	}
	// sprite描画
	if(dummy_sprite)
	{
		//定数バッファの更新
		{
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

			dissolve_constants dissolve{};
			dissolve.parameters.x = dissolve_value;
			immediate_context->UpdateSubresource(dissolve_constant_buffer.Get(), 0, 0, &dissolve, 0, 0);
			immediate_context->VSSetConstantBuffers(3, 1, dissolve_constant_buffer.GetAddressOf());
			immediate_context->PSSetConstantBuffers(3, 1, dissolve_constant_buffer.GetAddressOf());

			light_constants lights{};
			lights.ambient_color = ambient_color;
			lights.directional_light_direction = directional_light_direction;
			lights.directional_light_color = directional_light_color;
			immediate_context->UpdateSubresource(light_constant_buffer.Get(), 0, 0, &lights, 0, 0);
			immediate_context->VSSetConstantBuffers(2, 1, light_constant_buffer.GetAddressOf());
			immediate_context->PSSetConstantBuffers(2, 1, light_constant_buffer.GetAddressOf());

		}

		immediate_context->IASetInputLayout(sprite_input_layout.Get());
		immediate_context->PSSetShaderResources(1, 1, mask_texture.GetAddressOf());
		immediate_context->VSSetShader(sprite_vertex_shader.Get(), nullptr, 0);
		immediate_context->PSSetShader(sprite_pixel_shader.Get(), nullptr, 0);
		immediate_context->PSSetSamplers(0, 1, sampler_state.GetAddressOf());
		dummy_sprite->render(immediate_context.Get(), 256, 128, SCREEN_WIDTH - 256 * 2, SCREEN_HEIGHT - 128 * 2);
	}

#ifdef USE_IMGUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif

	UINT sync_interval{ 0 };
	swap_chain->Present(sync_interval, 0);
}