#include "FBXEditor.h"
#include "FileReader.h"
#include "StringUtil.h"

using namespace SunEngine;

int main(int argc, const char** argv)
{
	FBXEditor editor;
	if (!editor.Init(argc, argv))
		return -1;

	if (!editor.Run())
		return -1;

	return 0;
}