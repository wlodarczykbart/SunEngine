#pragma once

#include "GraphicsAPIDef.h"
#include "Types.h"

namespace SunEngine
{
	class CommandBuffer;

	class GraphicsObject
	{
	public:
		enum Type
		{
			SURFACE,
			SHADER,
			GRAPHICS_PIPELINE,
			RENDER_TARGET,
			COMMAND_BUFFER,
			MESH,
			SAMPLER,
			UNIFORM_BUFFER,
			TEXTURE,
			SHADER_BINDINGS,
			TEXTURE_CUBE,
			TEXTURE_ARRAY,
			VR_INTERFACE,
		};

		GraphicsObject::Type GetType() const;

		GraphicsObject(GraphicsObject::Type type);
		GraphicsObject(const GraphicsObject&) = delete;
		GraphicsObject& operator = (const GraphicsObject&) = delete;
		virtual ~GraphicsObject();

		virtual IObject* GetAPIHandle() const = 0;
		virtual bool Destroy();
		const String& GetErrStr() const;
		bool Bind(CommandBuffer* cmdBuffer, IBindState* pBindState = 0);
		bool Unbind(CommandBuffer* cmdBuffer);

	protected:
		String _errStr;

	private:
		GraphicsObject::Type _type;
	};

}