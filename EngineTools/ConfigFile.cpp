#include "FileReader.h"
#include "FileWriter.h"
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

	bool ConfigFile::Load(const char * fileName)
	{
		FileReader reader;
		if (reader.Open(fileName))
		{
			this->_sections.clear();
			this->_filename = fileName;

			String txtFile;
			reader.ReadAllText(txtFile);
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
					pSection = &_sections[section];
				}
				else if (eqPos != String::npos)
				{
					String key = line.substr(0, eqPos);
					String value = line.substr(eqPos + 1, line.length() - eqPos - 1);

					pSection->_dataPairs[key] = value;
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

	bool ConfigFile::Save(const char * fileName)
	{
		FileWriter writer;

		if (writer.Open(fileName))
		{
			_filename = fileName;

			OrderedStrMap<ConfigSection>::iterator sectionIt = _sections.begin();
			while (sectionIt != _sections.end())
			{
				String secStr = StrFormat("[%s]\n", (*sectionIt).first.data());	
				writer.Write(secStr.data(), secStr.length());

				OrderedStrMap<String> &dataPairs = (*sectionIt).second._dataPairs;
				OrderedStrMap<String>::iterator dataIt = dataPairs.begin();
				while (dataIt != dataPairs.end())
				{
					String dataStr = StrFormat("%s=%s\n", (*dataIt).first.data(), (*dataIt).second.data());
					writer.Write(dataStr.data(), dataStr.size());
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

	ConfigSection * ConfigFile::AddSection(const char * section)
	{
		ConfigSection* pSection = &_sections[section];
		return pSection;
	}

	ConfigSection * ConfigFile::GetSection(const char * section)
	{
		return operator[] (section);
	}

	ConfigSection * ConfigFile::operator[](const char * section)
	{
		OrderedStrMap<ConfigSection>::iterator it = _sections.find(section);
		if (it != _sections.end())
		{
			return &(*it).second;
		}
		else
		{
			return 0;
		}
	}

	const ConfigSection* ConfigFile::GetSection(const char* section) const
	{
		return operator[] (section);
	}

	const ConfigSection* ConfigFile::operator[](const char* section) const
	{
		OrderedStrMap<ConfigSection>::const_iterator it = _sections.find(section);
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

	String ConfigSection::GetString(const char * key, const char* defaultValue) const
	{
		const String *pStr;
		if (GetValue(key, &pStr))
		{
			return *pStr;
		}
		else
		{
			return defaultValue ? defaultValue : "";
		}
	}

	int ConfigSection::GetInt(const char * key, int defaultValue) const
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

	float ConfigSection::GetFloat(const char * key, float defaultValue) const
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

	bool ConfigSection::GetBlock(const char* key, OrderedStrMap<String>& block) const
	{
		const String *pStr;
		if (GetValue(key, &pStr))
		{
			usize start = pStr->find_first_of('{');
			usize end = pStr->find_last_of('}');
			if (start != String::npos && end != String::npos)
			{
				Vector<String> pairList;
				StrSplit(pStr->substr(start + 1, end - start - 1), pairList, ',');

				for (uint i = 0; i < pairList.size(); i++)
				{
					String strPair = pairList[i];
					usize eqPos = strPair.find('=');
					if (eqPos != String::npos)
					{
						String strKey = strPair.substr(0, eqPos);
						String strValue = strPair.substr(eqPos + 1);
						block[strKey] = strValue;
					}
				}

				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	bool ConfigSection::GetValue(const char * key, const String ** pStr) const
	{
		OrderedStrMap<String>::const_iterator it = _dataPairs.find(key);
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

	void ConfigSection::SetString(const char * key, const char* value)
	{
		_dataPairs[key] = value;
	}

	void ConfigSection::SetInt(const char * key, const int value)
	{
		_dataPairs[key] = IntToStr(value);
	}

	void ConfigSection::SetFloat(const char * key, const float value)
	{
		_dataPairs[key] = FloatToStr(value);
	}

	void ConfigSection::SetBlock(const char * key, const OrderedStrMap<String>& block)
	{
		String strBlock;	
		strBlock += "{";
		{
			String strPairs;
			OrderedStrMap<String>::const_iterator iter = block.begin();
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
		strBlock += "}";
		_dataPairs[key] = strBlock;
	}

	uint ConfigSection::GetCount() const
	{
		return (uint)_dataPairs.size();
	}

	bool ConfigSection::HasKey(const char * key) const
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