#pragma once

#include <cstdarg>
#include <string>

namespace ModelImporter
{
	namespace StrUtil
	{
		inline std::string ToLower(const std::string& str)
		{
			std::string copy = str;
			for (uint32_t i = 0; i < copy.length(); i++)
				if (copy[i] >= 'A' && copy[i] <= 'Z')
					copy[i] += ' ';

			return copy;
		}

		inline std::string GetFileName(const std::string& str)
		{
			size_t pos = str.find_last_of("\\/");
			return pos != str.npos ? str.substr(pos + 1) : str;
		}

		inline std::string GetDirectory(const std::string& str)
		{
			size_t pos = str.find_last_of("\\/");
			return pos != str.npos ? str.substr(0, pos) : str;
		}

		inline std::string GetExtension(const std::string& str)
		{
			size_t pos = str.find_last_of(".");
			return pos != str.npos ? str.substr(pos + 1) : "";
		}

		inline std::string Remove(const std::string& str, char token)
		{
			std::string copy = str;
			uint32_t counter = 0;
			for (uint32_t i = 0; i < str.length(); i++)
				if (str[i] != token)
					copy[counter++] = str[i];

			return copy.substr(0, counter);
		}

		inline std::string TrimStart(const std::string& str)
		{
			size_t pos = str.find_first_not_of(" \t");
			return pos != str.npos ? str.substr(pos) : str;
		}

		inline void Split(const std::string& str, std::vector<std::string>& parts, char token, bool removeEmtpy = true)
		{
			size_t pos, offset = 0;
			do
			{
				pos = str.find(token, offset);
				std::string line = str.substr(offset, pos - offset);
				if (line.size())
					parts.push_back(line);
				else
					if (!removeEmtpy) 
						parts.push_back(line);
				offset = pos;
				offset++;
			} while (offset != 0);
			
		}

		inline float ToFloat(const std::string& str)
		{
			return (float)atof(str.data());
		}

		inline int ToInt(const std::string& str)
		{
			return atoi(str.data());
		}

		inline std::string Format(const char* fmt, ...)
		{
			char buffer[4096];

			va_list va;
			va_start(va, fmt);
			vsprintf_s(buffer, fmt, va);
			va_end(va);

			return buffer;
		}
	};
}
