#pragma once

#include "BaseShader.h"
#include "BaseMesh.h"
#include "GraphicsPipeline.h"
#include "BaseTexture.h"
#include "UniformBuffer.h"

namespace SunEngine
{
	struct GWEventData;
	class Editor;
	class GraphicsWindow;
	class CommandBuffer;
	class RenderTarget;
	class View;

	class GUIRenderer
	{
	public:
		struct GUIVertex
		{
			void Set(float x, float y, float u, float v, float r, float g, float b, float a)
			{
				pos_uv[0] = x; pos_uv[1] = y;
				pos_uv[2] = u; pos_uv[3] = v;
				color[0] = r; color[1] = g; color[2] = b; color[3] = a;
			}

			float pos_uv[4];
			float color[4];
		};

		class UpdateCommand
		{
		public:
			UpdateCommand() {};
			virtual ~UpdateCommand() {};

			virtual bool Execute() = 0;
		};

		void PushUpdateCommand(UpdateCommand* cmd) { _updateCommands.push(UniquePtr<UpdateCommand>(cmd)); }

		GUIRenderer();
		GUIRenderer(const GUIRenderer&) = delete;
		GUIRenderer& operator = (const GUIRenderer&) = delete;
		virtual ~GUIRenderer();

		bool Init(Editor* pEditor);
		void Update(const GWEventData* pEvents, uint numEvents);
		void Render(CommandBuffer* cmdBuffer);

		bool RegisterView(View* pView);
		bool UpdateView(View* pView);

		Editor* GetEditor() const { return _pEditor; }
	private:
		bool SetupPlatform();
		bool SetupGraphics();

		virtual void CustomRender() = 0;
		void RenderViews();

		Editor* _pEditor;
		bool _bIsFocused;

		BaseShader _shader;
		Vector<UniquePtr<BaseMesh>> _meshes;
		Vector<GUIVertex> _vertices;
		Vector<uint> _indices;
		GraphicsPipeline _pipeline;

		BaseTexture _fontTexture;
		ShaderBindings _matrixBinding;
		UniformBuffer _matrixBuffer;

		Map<void*, UniquePtr<ShaderBindings>> _textureBindings;

		Queue<UniquePtr<UpdateCommand>> _updateCommands;
	};
}
