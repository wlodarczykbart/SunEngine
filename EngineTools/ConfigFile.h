#pragma once

#include "Types.h"

namespace SunEngine
{
	class ConfigSection
	{
	public:
		ConfigSection();
		~ConfigSection();

		const String& GetName() const { return _name; }

		String GetString(const String& key, const char* defaultValue = "") const;
		int GetInt(const String& key, int defaultValue = 0) const;
		uint GetUInt(const String& key, uint defaultValue = 0) const;
		float GetFloat(const String& key, float defaultValue = 0.0f) const;
		bool GetBool(const String& key, bool defaultValue = true) const;
		bool GetBlock(const String& key, StrMap<String>& block, char blockStart = 0, char blockEnd = 0) const;

		void SetString(const String& key, const char* value);
		void SetInt(const String& key, const int value);
		void SetFloat(const String& key, const float value);
		void SetBlock(const String& key, const StrMap<String>& block, char blockStart = 0, char blockEnd = 0);

		bool HasKey(const String& key) const;
		
		uint GetCount() const;

		typedef StrMap<String>::const_iterator Iterator;
		Iterator Begin() const;
		Iterator End() const;

	private:
		bool GetValue(const String& key, const String** pStr) const;

		friend class ConfigFile;
		StrMap<String> _dataPairs;
		String _name;
	};

	class ConfigFile
	{
	public:
		ConfigFile();
		~ConfigFile();

		bool Load(const String& fileName);
		bool Save(const String& fileName);

		ConfigSection* AddSection(const String& section);
		ConfigSection* GetSection(const String& section);
		const ConfigSection* GetSection(const String& section) const;

		const String& GetFilename() const;

		void Clear();

		typedef StrMap<ConfigSection>::const_iterator Iterator;
		Iterator Begin() const;
		Iterator End() const;

	private:
		String _filename;
		StrMap<ConfigSection> _sections;

	};
}

