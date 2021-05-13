#include "GameEditor.h"
#include "FileReader.h"
#include "StringUtil.h"
#include "ThreadPool.h"

using namespace SunEngine;

#if 0
#include <fstream>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

void ResizeImages()
{
	printf("Starting resize...\n");
	for (auto& p : fs::recursive_directory_iterator("F:/Downloads/EmeraldSquare1024"))
	{
		String filename = p.path().string();
		String ext = StrToLower(GetExtension(filename));

		if (ext == "png" || ext == "jpg" || ext == "tga" || ext == "bmp")
		{
			Image img;
			if (img.Load(filename))
			{
				uint width = img.Width();
				uint height = img.Height();
				img.Resize(1024, 1024);
				printf("Resized %s from (%d,%d) to (%d,%d)\n", filename.c_str(), width, height, img.Width(), img.Height());
				img.Save(filename);
			}
		}
	}
	printf("Finished resize...\n");
}
#else
void ResizeImages() {}
#endif

int main(int argc, const char** argv)
{
	ResizeImages();

	GameEditor editor;
	if (!editor.Init(argc, argv))
		return -1;

	if (!editor.Run())
		return -1;

	return 0;
}