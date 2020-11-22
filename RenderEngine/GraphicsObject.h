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
		};

		GraphicsObject::Type GetType() const;

		GraphicsObject(GraphicsObject::Type type);
		virtual ~GraphicsObject();

		virtual IObject* GetAPIHandle() const = 0;
		virtual bool Destroy();
		const String& GetErrStr() const;
		bool Bind(CommandBuffer* cmdBuffer);
		bool Unbind(CommandBuffer* cmdBuffer);

	protected:
		virtual bool OnBind(CommandBuffer* cmdBuffer);
		virtual bool OnUnbind(CommandBuffer* cmdBuffer);

		String _errStr;

	private:
		GraphicsObject::Type _type;
	};

}