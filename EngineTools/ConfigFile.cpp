#include <assert.h>
#include "FileBase.h"
#include "StringUtil.h"

#include "ConfigFile.h"

namespace SunEngine
{

	ConfigFile::ConfigFile()
	{
	}


	ConfigFile::~ConfigFile()
	{
	}

	bool ConfigFile::Load(const String& fileName)
	{
		FileStream reader;
		if (reader.OpenForRead(fileName.c_str()))
		{
			this->_sections.clear();
			this->_filename = fileName;

			String txtFile;
			reader.ReadText(txtFile);
			txtFile = StrRemove(txtFile, '\r');

			Vector<String> lines;
			StrSplit(txtFile, lines, '\n');

			ConfigSection *pSection = 0;
			for (uint i = 0; i < lines.size(); i++)
			{
				String &line = lines[i];

				usize startBracket = line.find('[');
				usize endBracket = line.find(']');
				usize eqPos = line.find('=');

				if (startBracket == 0 && endBracket == line.size() - 1 && startBracket < endBracket)
				{
					String section = line.substr(startBracket+1, endBracket - startBracket - 1);
					pSection = AddSection(section);
				}
				else if (eqPos != String::npos)
				{
					String key = line.substr(0, eqPos);
					String value = line.substr(eqPos + 1, line.length() - eqPos - 1);

					pSection->_dataPairs[key] = value;
				}
				else if (line.length())
				{
					pSection->_dataPairs[line] = "";
				}
			}

			reader.Close();
			return true;
		}
		else
		{
			return false;
		}
	}

	bool ConfigFile::Save(const String& fileName)
	{
		FileStream writer;

		if (writer.OpenForWrite(fileName.c_str()))
		{
			_filename = fileName;

			StrMap<ConfigSection>::iterator sectionIt = _sections.begin();
			while (sectionIt != _sections.end())
			{
				String secStr = StrFormat("[%s]\n", (*sectionIt).first.data());	
				writer.WriteText(secStr);

				StrMap<String> &dataPairs = (*sectionIt).second._dataPairs;
				StrMap<String>::iterator dataIt = dataPairs.begin();
				while (dataIt != dataPairs.end())
				{
					String dataStr = StrFormat("%s=%s\n", (*dataIt).first.data(), (*dataIt).second.data());
					writer.WriteText(dataStr);
					dataIt++;
				}

				writer.Write('\n');
				sectionIt++;
			}

			writer.Close();
			return true;
		}
		else
		{
			return false;
		}
	}

	ConfigSection * ConfigFile::AddSection(const String& section)
	{
		ConfigSection* pSection = GetSection(section);
		if (pSection)
			return pSection;

		pSection = &_sections[section];
		pSection->_name = section;
		return pSection;
	}

	ConfigSection * ConfigFile::GetSection(const String& section)
	{
		StrMap<ConfigSection>::iterator it = _sections.find(section);
		if (it != _sections.end())
		{
			return &(*it).second;
		}
		else
		{
			return 0;
		}
	}

	const ConfigSection* ConfigFile::GetSection(const String& section) const
	{
		StrMap<ConfigSection>::const_iterator it = _sections.find(section);
		if (it != _sections.end())
		{
			return &(*it).second;
		}
		else
		{
			return 0;
		}
	}

	const String& ConfigFile::GetFilename() const
	{
		return _filename;
	}

	void ConfigFile::Clear()
	{
		_filename = "";
		_sections.clear();
	}

	ConfigSection::ConfigSection()
	{
	}

	ConfigSection::~ConfigSection()
	{
	}

	String ConfigSection::GetString(const String& key, const char* defaultValue) const
	{
		const String *pStr;
		if (GetValue(key, &pStr))
		{
			return *pStr;
		}
		else
		{
			return defaultValue;
		}
	}

	int ConfigSection::GetInt(const String& key, int defaultValue) const
	{
		const String *pStr;
		if (GetValue(key, &pStr))
		{
			return StrToInt(*pStr);
		}
		else
		{
			return defaultValue;
		}
	}

	float ConfigSection::GetFloat(const String& key, float defaultValue) const
	{
		const String *pStr;
		if (GetValue(key, &pStr))
		{
			return StrToFloat(*pStr);
		}
		else
		{
			return defaultValue;
		}
	}

	bool ConfigSection::GetBlock(const String& key, StrMap<String>& block, char blockStart, char blockEnd) const
	{
		const String *pStr;
		if (GetValue(key, &pStr))
		{
			usize start = blockStart != 0 ? pStr->find_first_of(blockStart) : String::npos;
			usize end = blockEnd != 0 ? pStr->find_last_of(blockEnd) : pStr->length();

			Vector<String> pairList;
			StrSplit(pStr->substr(start + 1, end - start - 1), pairList, ',');

			for (uint i = 0; i < pairList.size(); i++)
			{
				String strPair = pairList[i];
				usize eqPos = strPair.find('=');
				if (eqPos != String::npos)
				{
					String strKey = StrTrimStart(StrTrimEnd(strPair.substr(0, eqPos)));
					String strValue = StrTrimStart(StrTrimEnd(strPair.substr(eqPos + 1)));
					block[strKey] = strValue;
				}
				else
				{
					String strKey = StrTrimStart(StrTrimEnd(strPair));
					block[strKey] = "";
				}
			}

			return true;

		}
		else
		{
			return false;
		}
	}

	bool ConfigSection::GetValue(const String& key, const String ** pStr) const
	{
		StrMap<String>::const_iterator it = _dataPairs.find(key);
		if (it != _dataPairs.end())
		{
			*pStr = &(*it).second;
			return true;
		}
		else
		{
			return false;
		}
	}

	void ConfigSection::SetString(const String& key, const char* value)
	{
		_dataPairs[key] = value;
	}

	void ConfigSection::SetInt(const String& key, const int value)
	{
		_dataPairs[key] = IntToStr(value);
	}

	void ConfigSection::SetFloat(const String& key, const float value)
	{
		_dataPairs[key] = FloatToStr(value);
	}

	void ConfigSection::SetBlock(const String& key, const StrMap<String>& block, char blockStart, char blockEnd)
	{
		String strBlock;	
		if (blockStart != 0) strBlock += blockStart;
		{
			String strPairs;
			StrMap<String>::const_iterator iter = block.begin();
			while (iter != block.end())
			{
				strPairs += StrFormat("%s=%s,", (*iter).first.data(), (*iter).second.data());
				++iter;
			}

			if (strPairs.length())
			{
				strPairs = strPairs.substr(0, strPairs.length() - 1);
			}

			strBlock += strPairs;
		}
		if(blockEnd != 0) strBlock += blockEnd;
		_dataPairs[key] = strBlock;
	}

	uint ConfigSection::GetCount() const
	{
		return (uint)_dataPairs.size();
	}

	bool ConfigSection::HasKey(const String& key) const
	{
		return _dataPairs.find(key) != _dataPairs.end();
	}

	ConfigSection::Iterator ConfigSection::Begin() const
	{
		return _dataPairs.begin();
	}

	ConfigSection::Iterator ConfigSection::End() const
	{
		return _dataPairs.end();
	}

	ConfigFile::Iterator ConfigFile::Begin() const
	{
		return _sections.begin();
	}

	ConfigFile::Iterator ConfigFile::End() const
	{
		return _sections.end();
	}
}