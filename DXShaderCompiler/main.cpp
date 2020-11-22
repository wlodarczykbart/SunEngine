#include "ShaderCompiler.h"
#include "GraphicsContext.h"

int main(int argc, const char ** argv)
{
	SunEngine::String errStr;
	if (!SunEngine::ShaderCompiler::Compile(argc, argv, errStr))
	{
		printf(errStr.data());
		getchar();
		return -1;
	}

	if (errStr.length())
	{
		printf(errStr.data());
	}

	return 0;
}