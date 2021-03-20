#include <cstdarg>

#include "StringUtil.h"

namespace SunEngine
{
	void StrSplit(const String& str, Vector<String>& parts, char token, bool removeSeperatorOnlyStr)
	{
		uint offset = 0;
		for (uint i = 0; i <= str.size(); i++)
		{
			if (i == str.size() || str[i] == token)
			{
				String part = str.substr(offset, i - offset);
				if (part.size())
				{
					if (removeSeperatorOnlyStr)
					{
						bool isSeperatorOnly = true;
						for (uint j = 0; j < part.size() && isSeperatorOnly; j++)
						{
							if (!(part[j] == ' '))
								isSeperatorOnly = false;
						}

						if (!isSeperatorOnly)
							parts.push_back(part);
					}
					else
					{
						parts.push_back(part);
					}
				}
				offset = i + 1;
			}
		}

		//uint pos = 0;
		//uint offset = 0;
		//while ((offset = str.find(token, pos)) != String::npos)
		//{
		//	String part = str.substr(pos, offset - pos);
		//	if (part.length())
		//	{
		//		if (removeEmpty)
		//		{
		//			bool isEmpty = true;
		//			for (uint i = 0; i < part.size() && isEmpty; i++)
		//			{
		//				if (part[i] != ' ')
		//					isEmpty = false;
		//			}

		//			if (!isEmpty)
		//				parts.push_back(part);
		//		}
		//		else
		//		{
		//			parts.push_back(part);
		//		}


		//	}
		//	pos = offset + 1;
		//}

		//parts.push_back(str.substr(pos, str.length() - pos));
	}

	int StrToInt(const String & str)
	{
		return atoi(str.data());
	}

	float StrToFloat(const String & str)
	{
		return (float)atof(str.data());
	}

	String IntToStr(const int value)
	{
		return std::to_string(value);
	}

	String FloatToStr(const float value)
	{
		return std::to_string(value);
	}

	String StrFormat(const char * fmt, ...)
	{
		static char buffer[4096 * 4];

		va_list va;
		va_start(va, fmt);
		vsprintf_s(buffer, fmt, va);
		va_end(va);

		return buffer;
	}

	String GetFileName(const String & str)
	{
		usize fwdSlash = str.rfind('/');
		usize backSlash = str.rfind('\\');

		if (fwdSlash == backSlash && backSlash == String::npos)
			return str;

		usize startPos = 0;
		if (fwdSlash != String::npos && backSlash != String::npos)
		{
			startPos = std::max(fwdSlash, backSlash);
		}
		else if (fwdSlash != String::npos)
		{
			startPos = fwdSlash;
		}
		else if (backSlash != String::npos)
		{
			startPos = backSlash;
		}

		return str.substr(startPos + 1, str.length() - startPos - 1);
	}

	String GetFileNameNoExt(const String & str)
	{
		String filename = GetFileName(str);
		usize extPos = filename.rfind('.');
		
		if (extPos != String::npos)
			filename = filename.substr(0, extPos);

		return filename;
	}

	String GetExtension(const String& fileName)
	{
		usize extPos = fileName.rfind('.');

		if (extPos != String::npos)
			return fileName.substr(extPos + 1);

		return "";
	}

	String StripExtension(const String& fileName)
	{
		usize extPos = fileName.rfind('.');

		if (extPos != String::npos)
			return fileName.substr(0, extPos);

		return fileName;
	}

	String GetDirectory(const String & fileName)
	{
		usize fwdSlash = fileName.rfind('/');
		usize backSlash = fileName.rfind('\\');

		if (fwdSlash == backSlash && backSlash == String::npos)
			return "";

		usize endPos = 0;
		if (fwdSlash != String::npos && backSlash != String::npos)
		{
			endPos = std::max(fwdSlash, backSlash);
		}
		else if (fwdSlash != String::npos)
		{
			endPos = fwdSlash;
		}
		else if (backSlash != String::npos)
		{
			endPos = backSlash;
		}

		return fileName.substr(0, endPos+1);
	}

	String StrToLower(const String & str)
	{
		String outString = str;

		for (usize i = 0; i < str.size(); i++)
		{
			if (outString[i] >= 'A' && str[i] <= 'Z')
				outString[i] += ' ';
		}

		return outString;
	}

	String StrReplace(const String & str, const Vector<char>& inTokens, char outToken)
	{
		String outString = str;

		for (usize i = 0; i < str.size(); i++)
		{
			if (Contains(inTokens, str[i]))
				outString[i] = outToken;
		}

		return outString;
	}

	String StrTrimStart(const String & str)
	{
		for (usize i = 0; i < str.size(); i++)
		{
			if (str[i] != ' ' && str[i] != '\t' && str[i] != '\r' && str[i] != '\n')
				return &str[i];
		}

		return str;
	}

	String StrTrimEnd(const String& str)
	{
		usize i = str.size() - 1;
		for (; i >= 0; i--)
		{
			if (str[i] != ' ' && str[i] != '\t' && str[i] != '\r' && str[i] != '\n')
				break;
		}

		return str.substr(0, i + 1);
	}

	String StrTrim(const String& str)
	{
		return StrTrimEnd(StrTrimStart(str));
	}

	bool StrStartsWith(const String & str, const String & startsWith)
	{
		usize offset = str.find(startsWith);
		return offset == 0;
	}

	String StrRemove(const String & str, char token)
	{
		String outString;
		outString.resize(str.size());

		usize index = 0;
		for (usize i = 0; i < str.size(); i++)
		{
			if (str[i] != token)
				outString[index++] = str[i];
		}

		return outString.data();
	}

	bool StrContains(const String& str, const char* inStr)
	{
		return str.find(inStr) != String::npos;
	}

	bool StrAlphaNumeric(const String & str)
	{
		for (uint i = 0; i < str.length(); i++)
		{
			if (!CharAlphaNumeric(str[i]))
				return false;
		}

		return true;
	}

	bool CharAlphaNumeric(char c)
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
	}

	bool StrIsUInt(const String & str)
	{
		for (uint i = 0; i < str.length(); i++)
		{
			if (!(str[i] >= '0' && str[i] <= '9'))
			{
				return false;
			}
		}
		return true;
	}

	int GetNumOccurrences(const String & str, char c)
	{
		int count = 0;
		for (uint i = 0; i < str.length(); i++)
		{
			if (str[i] == c)
				count++;
		}

		return count;
	}

	String VecToStr(const Vector<String>& parts, char seperator)
	{
		if (parts.size() == 0)
			return "";

		String str;

		for (uint i = 0; i < parts.size() - 1; i++)
		{
			str += parts[i];
			str += seperator;
		}

		str += parts[parts.size() - 1];
		return str;
	}
}