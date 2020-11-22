#pragma once

#include "Types.h"

namespace SunEngine
{
	void StrSplit(const String &str, Vector<String> &parts, char token, bool removeSeperatorOnlyStr = true);

	int StrToInt(const String &str);
	float StrToFloat(const String &str);

	String IntToStr(const int value);
	String FloatToStr(const float value);
	String VecToStr(const Vector<String>& parts, char seperator);

	String StrFormat(const char * fmt, ...);

	String GetFileName(const String &str);
	String GetFileNameNoExt(const String &str);
	String GetDirectory(const String &fileName);
	String GetExtension(const String& fileName);
	String StripExtension(const String& fileName);

	String StrToLower(const String& str);
	String StrReplace(const String& str, const Vector<char> &inTokens, char outToken);
	String StrTrimStart(const String& str);
	String StrTrimEnd(const String& str);
	bool StrStartsWith(const String& str, const String& startsWith);
	String StrRemove(const String& str, char token);
	bool StrContains(const String& str, const char* inStr);
	bool StrAlphaNumeric(const String& str);
	bool StrIsUInt(const String& str);
	bool CharAlphaNumeric(char c);

	int GetNumOccurrences(const String& str, char c);


}

