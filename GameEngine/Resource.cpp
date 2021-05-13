#include "Resource.h"

namespace SunEngine
{
    Resource::Resource()
    {
        _userData = 0;
    }

    Resource::~Resource()
    {
    }

    bool Resource::Write(StreamBase& stream)
    {
        if (!stream.Write(_name)) return false;
        
        return true;
    }

    bool Resource::Read(StreamBase& stream)
    {
        if (!stream.Read(_name)) return false;

        return true;
    }
}