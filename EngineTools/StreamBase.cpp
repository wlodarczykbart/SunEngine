#include "StreamBase.h"



namespace SunEngine
{

	StreamBase::StreamBase()
	{
	}


	StreamBase::~StreamBase()
	{
	}

	uint StreamBase::Size()
	{
		uint curr = Tell();
		if (Seek(0, START))
		{
			Seek(0, END);
			uint size = Tell();
			Seek(curr, START);
			return size;
		}
		else
		{
			return 0;
		}
	}
}