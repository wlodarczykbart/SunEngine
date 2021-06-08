#include "Editor.h"
#include "GraphicsWindow.h"
#include "GraphicsContext.h"
#include "CommandBuffer.h"
#include "View.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "StringUtil.h"

#include "spdlog/spdlog.h"
#include "ShaderCompiler.h"
#include "GUIRenderer.h"

//#define TEST_IMGUI_BASIC

namespace SunEngine
{
	const char* GUIVertexText =
		"#include \"CameraBuffer.hlsl\"\n\
		#include \"ObjectBuffer.hlsl\"\n\
		struct VS_In\n\
		{\n\
			float4 pos_uv : POSITION;\n\
			float4 color : COLOR;\n\
		};	\n\
		struct PS_In\n\
		{\n\
			float4 clipPos : SV_POSITION;\n\
			float4 color : COLOR;\n\
			float2 texCoord : TEXCOORD;\n\
		};	\n\
		static float2 points[4] = { float2(-0.5f, -0.5f), float2(0.5f, -0.5f), float2(0.5f, 0.5f), float2(-0.5f, 0.5f)};	\n\
		PS_In main(VS_In vIn, uint vIndex : SV_VERTEXID)\n\
		{\n\
			PS_In pIn;\n\
			pIn.clipPos = mul(float4(vIn.pos_uv.xy, 0.0, 1.0), WorldMatrix);\n\
			pIn.color = vIn.color;\n\
			pIn.texCoord = vIn.pos_uv.zw;		\n\
			return pIn;\n\
		}"
		;

	const char* GUIPixelText =
		"Texture2D Texture;\n\
		SamplerState Sampler;\n\
		struct PS_In\n\
		{\n\
			float4 clipPos : SV_POSITION;\n\
			float4 color : COLOR;\n\
			float2 texCoord : TEXCOORD;\n\
		};\n\
		float4 main(PS_In pIn) : SV_TARGET\n\
		{\n\
			return Texture.Sample(Sampler, pIn.texCoord) * pIn.color;\n\
		}"
	;

	GUIRenderer::GUIRenderer()
	{
		_pEditor = 0;
		_bIsFocused = false;
	}

	GUIRenderer::~GUIRenderer()
	{
	}

	bool GUIRenderer::Init(Editor* pEditor)
	{
		if (pEditor == 0)
			return false;

		_pEditor = pEditor;

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// Setup Platform/Renderer backends
		if (!SetupPlatform())
			return false;

		if (!SetupGraphics())
			return false;

		return true;
	}

	bool GUIRenderer::RegisterView(View* pView)
	{
		if (!pView)
			return false;

		if (pView->GetRenderToGraphicsWindow())
			return false;

		ShaderBindings* pBindings = 0;

		auto found = _textureBindings.find(pView);
		if (found != _textureBindings.end())
		{
			pBindings = (*found).second.get();
		}
		else
		{
			pBindings = new ShaderBindings();
			_textureBindings[pView] = UniquePtr<ShaderBindings>(pBindings);
		}

		ShaderBindings::CreateInfo bindingInfo = {};
		bindingInfo.type = SBT_MATERIAL;
		bindingInfo.pShader = &_shader;

		if (!pBindings->Create(bindingInfo))
			return false;

		if (!pBindings->SetTexture("Texture", pView->GetRenderTarget()->GetColorTexture()))
			return false;

		if (!pBindings->SetSampler("Sampler", GraphicsContext::GetDefaultSampler(GraphicsContext::DS_LINEAR_CLAMP)))
			return false;

		return true;
	}

	bool GUIRenderer::UpdateView(View* pView)
	{
		if (!pView)
			return false;

		if (pView->GetRenderToGraphicsWindow())
			return false;

		auto found = _textureBindings.find(pView);
		if (found == _textureBindings.end())
			return false;

		if (!(*found).second->SetTexture("Texture", pView->GetRenderTarget()->GetColorTexture()))
			return false;

		return true;
	}

	bool GUIRenderer::SetupPlatform()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
		io.BackendPlatformName = "GraphicsWindow";
		io.ImeWindowHandle = _pEditor->GetGraphicsWindow()->Handle();

		// Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
		io.KeyMap[ImGuiKey_Tab] = VK_TAB;
		io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
		io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
		io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
		io.KeyMap[ImGuiKey_Home] = VK_HOME;
		io.KeyMap[ImGuiKey_End] = VK_END;
		io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
		io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
		io.KeyMap[ImGuiKey_Space] = VK_SPACE;
		io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
		io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
		io.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
		io.KeyMap[ImGuiKey_A] = 'A';
		io.KeyMap[ImGuiKey_C] = 'C';
		io.KeyMap[ImGuiKey_V] = 'V';
		io.KeyMap[ImGuiKey_X] = 'X';
		io.KeyMap[ImGuiKey_Y] = 'Y';
		io.KeyMap[ImGuiKey_Z] = 'Z';

		return true;
	}

	bool GUIRenderer::SetupGraphics()
	{
		// Setup backend capabilities flags
		ImGuiIO& io = ImGui::GetIO();
		io.BackendRendererName = GraphicsContext::GetAPIName();
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

		_vertices.resize(10000);
		_indices.resize(30000);

		auto pWindow = _pEditor->GetGraphicsWindow();
		float ww = (float)pWindow->Width();
		float wh = (float)pWindow->Height();

		_vertices[0].Set(ww * 0.25f, wh * 0.75f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		_vertices[1].Set(ww * 0.75f, wh * 0.75f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		_vertices[2].Set(ww * 0.75f, wh * 0.25f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);

		_vertices[3].Set(ww * 0.25f, wh * 0.75f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		_vertices[4].Set(ww * 0.75f, wh * 0.25f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		_vertices[5].Set(ww * 0.25f, wh * 0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);

		_indices[0] = 0;
		_indices[1] = 1;
		_indices[2] = 2;
		_indices[3] = 3;
		_indices[4] = 4;
		_indices[5] = 5;

		BaseMesh::CreateInfo meshInfo = {};
		meshInfo.numVerts = _vertices.size();
		meshInfo.pVerts = _vertices.data();
		meshInfo.numIndices = _indices.size();
		meshInfo.pIndices = _indices.data();
		meshInfo.vertexStride = sizeof(GUIVertex);

		for(uint i = 0; i < _pEditor->GetSurface()->GetBackBufferCount(); i++)
		{
			auto pMesh = new BaseMesh();
			if (!pMesh->Create(meshInfo))
				return false;
			_meshes.push_back(UniquePtr<BaseMesh>(pMesh));
		}

		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		BaseTexture::CreateInfo texInfo = {};
		texInfo.image.Width = width;
		texInfo.image.Height = height;
		texInfo.image.Pixels = (Pixel*)pixels;
		if (!_fontTexture.Create(texInfo))
			return false;

		UniformBuffer::CreateInfo buffInfo = {};
		buffInfo.isShared = false;
		buffInfo.size = sizeof(ObjectBufferData);
		if (!_matrixBuffer.Create(buffInfo))
			return false;

		ShaderCompiler shaderCompiler;
		shaderCompiler.SetVertexShaderSource(GUIVertexText);
		shaderCompiler.SetPixelShaderSource(GUIPixelText);

		if (!shaderCompiler.Compile())
			return false;

		if (!_shader.Create(shaderCompiler.GetCreateInfo()))
		{
			return false;
		}

		GraphicsPipeline::CreateInfo pipelineInfo = {};
		pipelineInfo.pShader = &_shader;
		pipelineInfo.settings.EnableAlphaBlend();
		pipelineInfo.settings.rasterizer.enableScissor = true;
		pipelineInfo.settings.rasterizer.cullMode = SE_CM_NONE;
		pipelineInfo.settings.rasterizer.frontFace = SE_FF_COUNTER_CLOCKWISE;
		//pipelineInfo.settings.depthClipEnable = true;
		//pipelineInfo.settings.depthStencil.depthCompareOp = SE_DC_ALWAYS;
		if (!_pipeline.Create(pipelineInfo))
			return false;

		ShaderBindings::CreateInfo bindingInfo = {};
		bindingInfo.pShader = &_shader;

		bindingInfo.type = SBT_OBJECT;
		if (!_matrixBinding.Create(bindingInfo)) return false;
		if (!_matrixBinding.SetUniformBuffer(ShaderStrings::ObjectBufferName, &_matrixBuffer)) {};// return false;

		bindingInfo.type = SBT_MATERIAL;

		ShaderBindings* pDefaultBindings = new ShaderBindings();
		if (!pDefaultBindings->Create(bindingInfo)) return false;
		if (!pDefaultBindings->SetTexture("Texture", &_fontTexture)) return false;
		//if (!_fontBinding.SetTexture("Texture", GraphicsContext::GetDefaultTexture(GraphicsContext::DT_WHITE))) return false;
		if (!pDefaultBindings->SetSampler("Sampler", GraphicsContext::GetDefaultSampler(GraphicsContext::DS_LINEAR_CLAMP))) return false;

		_textureBindings[nullptr] = UniquePtr<ShaderBindings>(pDefaultBindings);

		return true;
	}

	void GUIRenderer::Update(const GWEventData* pEvents, uint numEvents)
	{
		if (ImGui::GetCurrentContext() == NULL)
			return;

		ImGuiIO& io = ImGui::GetIO();

		for (uint i = 0; i < numEvents; i++)
		{
			const GWEventData& e = pEvents[i];

			switch (e.type)
			{
			case GWE_KEY_DOWN:
			{
				if (e.nativeKey < 256)
					io.KeysDown[e.nativeKey] = 1;
			} break;
			case GWE_KEY_UP:
			{
				if (e.nativeKey < 256)
					io.KeysDown[e.nativeKey] = 0;
			} break;
			case GWE_MOUSE_DOWN:
			{
				int button = 0;
				if (e.mouseButtonCode == MOUSE_LEFT) { button = 0; }
				if (e.mouseButtonCode == MOUSE_RIGHT) { button = 1; }
				if (e.mouseButtonCode == MOUSE_MIDDLE) { button = 2; }
				//if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
				//	::SetCapture(hwnd);
				io.MouseDown[button] = true;
			} break;
			case GWE_MOUSE_UP:
			{
				int button = 0;
				if (e.mouseButtonCode == MOUSE_LEFT) { button = 0; }
				if (e.mouseButtonCode == MOUSE_RIGHT) { button = 1; }
				if (e.mouseButtonCode == MOUSE_MIDDLE) { button = 2; }
				io.MouseDown[button] = false;
				//if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
				//	::ReleaseCapture();
			} break;
			case GWE_RESIZE:
			{

			} break;
			case GWE_MOUSE_WHEEL:
			{
				io.MouseWheel += e.delta;
			} break;
			case GWE_KEY_INPUT:
			{
				if (e.nativeKey > 0 && e.nativeKey < 0x10000)
					io.AddInputCharacterUTF16((unsigned short)e.nativeKey);
			} break;
			case GWE_FOCUS_SET:
			{
				_bIsFocused = true;
			} break;
			case GWE_FOCUS_LOST:
			{
				_bIsFocused = false;
			} break;
			default:
				break;
			}
		}

		IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

		auto pWindow = _pEditor->GetGraphicsWindow();

		// Setup display size (every frame to accommodate for window resizing) 
		//BART: window w/h members not updating after window resize currently
		io.DisplaySize = { (float)pWindow->Width(), (float)pWindow->Height() };

		// Setup time step
		io.DeltaTime = 1.0f / 60.0f;//BART: hardcoded 60fps /*(float)(current_time - g_Time) / g_TicksPerSecond;*/

		// Read keyboard modifiers inputs
		io.KeyCtrl = GraphicsWindow::KeyDown(KEY_CONTROL);
		io.KeyShift = GraphicsWindow::KeyDown(KEY_LSHIFT) || GraphicsWindow::KeyDown(KEY_RSHIFT);
		io.KeyAlt = GraphicsWindow::KeyDown(KEY_MENU);
		io.KeySuper = false;
		// io.KeysDown[], io.MousePos, io.MouseDown[], io.MouseWheel: filled by the WndProc handler below.

		// Update OS mouse position
		// Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
		//if (io.WantSetMousePos)
		//{
		//	POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
		//	if (::ClientToScreen(g_hWnd, &pos))
		//		::SetCursorPos(pos.x, pos.y);
		//}

		// Set mouse position
		io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
		if (_bIsFocused)
		{
			int x, y;
			pWindow->GetMousePosition(x, y);
			io.MousePos = ImVec2((float)x, (float)y);
		}

		//BART: not supporting cursors/gamepads
		//// Update OS mouse cursor with the cursor requested by imgui
		//ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
		//if (g_LastMouseCursor != mouse_cursor)
		//{
		//	g_LastMouseCursor = mouse_cursor;
		//	ImGui_ImplWin32_UpdateMouseCursor();
		//}

		//// Update game controllers (if enabled and available)
		//ImGui_ImplWin32_UpdateGamepads();

		while (!_updateCommands.empty())
		{
			_updateCommands.front()->Execute();
			_updateCommands.pop();
		}
	}

	void GUIRenderer::Render(CommandBuffer* cmdBuffer)
	{
		ImGui::NewFrame();

		static bool show_demo_window = true;
		ImGui::ShowDemoWindow(&show_demo_window);

		CustomRender();
		RenderViews();

		ImGui::Render();
		ImDrawData* draw_data = ImGui::GetDrawData();

		// Avoid rendering when minimized
		if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
			return;

		if (draw_data->TotalVtxCount == 0 || draw_data->TotalIdxCount == 0)
			return;

#ifndef TEST_IMGUI_BASIC
		// Create and grow vertex/index buffers if needed
		if (_vertices.size() < draw_data->TotalVtxCount)
		{
			_vertices.resize(draw_data->TotalVtxCount);
		}

		if (_indices.size() < draw_data->TotalIdxCount)
		{
			_indices.resize(draw_data->TotalIdxCount);
		}

		// Upload vertex/index data into a single contiguous GPU buffer
		GUIVertex* vtx_dst = (GUIVertex*)_vertices.data();
		uint * idx_dst = (uint*)_indices.data();
		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[n];

			for (int i = 0; i < cmd_list->VtxBuffer.Size; i++)
			{
				const ImDrawVert& imVert = cmd_list->VtxBuffer[i];
				ImColor col(imVert.col);
				vtx_dst->pos_uv[0] = imVert.pos.x; vtx_dst->pos_uv[1] = imVert.pos.y;
				vtx_dst->pos_uv[2] = imVert.uv.x; vtx_dst->pos_uv[3] = imVert.uv.y;
				vtx_dst->color[0] = col.Value.x; vtx_dst->color[1] = col.Value.y; vtx_dst->color[2] = col.Value.z; vtx_dst->color[3] = col.Value.w;
				vtx_dst++;
			}

			for (int i = 0; i < cmd_list->IdxBuffer.Size; i++)
			{
				*idx_dst = (uint)cmd_list->IdxBuffer[i];
				idx_dst++;
			}
		}

		auto mesh = _meshes[_pEditor->GetSurface()->GetFrameIndex()].get();
		mesh->UpdateVertices(_vertices.data(), draw_data->TotalVtxCount * sizeof(GUIVertex));
		mesh->UpdateIndices(_indices.data(), draw_data->TotalIdxCount);
#endif

		// Setup orthographic projection matrix into our constant buffer
		// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
		{
			float L = draw_data->DisplayPos.x;
			float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
			float T = draw_data->DisplayPos.y;
			float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
			float mvp[4][4] = {};

			mvp[0][0] = 2.0f / (R - L);
			mvp[1][1] = 2.0f / (T - B);
			mvp[2][2] = 0.5f;
			mvp[3][0] = (R + L) / (L - R);
			mvp[3][1] = (T + B) / (B - T);
			mvp[3][2] = 0.5f;
			mvp[3][3] = 1.0f;

			//float t[4][4];
			//for (int i = 0; i < 4; i++)
			//	for (int j = 0; j < 4; j++)
			//		t[i][j] = mvp[j][i];

			_matrixBuffer.Update(mvp, 0, sizeof(mvp));
		}

		// Setup desired Graphics State
		_shader.Bind(cmdBuffer);
		mesh->Bind(cmdBuffer);
		_pipeline.Bind(cmdBuffer);
		_matrixBinding.Bind(cmdBuffer);

#ifndef TEST_IMGUI_BASIC
		// Render command lists
		// (Because we merged all buffers into a single one, we maintain our own offset into them)
		int global_idx_offset = 0;
		int global_vtx_offset = 0;
		ImVec2 clip_off = draw_data->DisplayPos;
		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[n];
			for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
				if (pcmd->UserCallback != NULL)
				{
					// User callback, registered via ImDrawList::AddCallback()
					// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
					if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					{
					}
					else
					{
						pcmd->UserCallback(cmd_list, pcmd);
					}
				}
				else
				{
					// Bind texture, Draw
					_textureBindings.at(pcmd->TextureId)->Bind(cmdBuffer);
					
					float sx = pcmd->ClipRect.x - clip_off.x;
					float sy = pcmd->ClipRect.y - clip_off.y;
					float width = (pcmd->ClipRect.z - pcmd->ClipRect.x);
					float height = (pcmd->ClipRect.w - pcmd->ClipRect.y);
					cmdBuffer->SetScissor(sx, sy, width, height);
					cmdBuffer->DrawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
				}
			}
			global_idx_offset += cmd_list->IdxBuffer.Size;
			global_vtx_offset += cmd_list->VtxBuffer.Size;
		}

#else
		_fontBinding.Bind(cmdBuffer);
		cmdBuffer->SetScissor(0.0f, 0.0f, (float)_pWindow->Width(), (float)_pWindow->Height());
		cmdBuffer->DrawIndexed(6, 1, 0, 0, 0);
		//cmdBuffer->Draw(6, 1, 0, 0);
#endif

		_textureBindings.at(nullptr)->Bind(cmdBuffer);
		_matrixBinding.Unbind(cmdBuffer);
		_pipeline.Unbind(cmdBuffer);
		mesh->Unbind(cmdBuffer);
		_shader.Unbind(cmdBuffer);

		auto pWindow = _pEditor->GetGraphicsWindow();
		cmdBuffer->SetScissor(0.0f, 0.0f, (float)pWindow->Width(), (float)pWindow->Height());
	}

	void GUIRenderer::RenderViews()
	{
		Vector<View*> views;
		for (uint i = 0; i < _pEditor->GetViews(views); i++)
		{
			View* pView = views[i];
			if (pView->GetRenderToGraphicsWindow())
				continue;

			bool visible = pView->GetVisible();
			if (visible && ImGui::Begin(pView->GetName().c_str(), &visible))
			{
				ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;
				String strTable = StrFormat("%s_table", pView->GetName().c_str());
				if (ImGui::BeginTable(strTable.c_str(), pView->GetGUIColumns(), tableFlags))
				{
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImVec2 imageSpace = ImGui::GetContentRegionAvail();
					ImGui::Image(pView, ImVec2((float)pView->GetRenderTarget()->Width(), (float)pView->GetRenderTarget()->Height()));

					ImGuiItemStatusFlags viewFlags = ImGui::GetItemStatusFlags();
					glm::vec2 viewPos = glm::vec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y);
					glm::vec2 viewSize = glm::vec2(imageSpace.x, imageSpace.y);
					viewSize = glm::max(viewSize, glm::vec2(4.0f, 4.0f));
					
					bool mouseInsideView = viewFlags & ImGuiItemStatusFlags_HoveredRect;
					bool viewFocus = ImGui::IsWindowFocused() && mouseInsideView;
					pView->UpdateViewState(viewSize, viewPos, mouseInsideView, viewFocus);

					pView->RenderGUI(this);

					ImGui::EndTable();
					//float w = ImGui::table(ImGui::TableFindByID(ImGui::GetID(strTable.c_str())), 0);
					//spdlog::info("{}", w);
				}
				ImGui::End();
			}
			pView->SetVisible(visible);
		}
	}
}