#pragma once

#include "Types.h"

namespace SunEngine
{
	class ConfigSection
	{
	public:
		ConfigSection();
		~ConfigSection();

		String GetString(const char * key) const;
		int GetInt(const char* key, int defaultValue = 0) const;
		float GetFloat(const char* key, float defaultValue = 0.0f) const;
		bool GetBlock(const char* key, OrderedStrMap<String>& block) const;

		void SetString(const char * key, const char * value);
		void SetInt(const char* key, const int value);
		void SetFloat(const char* key, const float value);
		void SetBlock(const char* key, const OrderedStrMap<String>& block);

		bool HasKey(const char* key) const;
		
		uint GetCount() const;

		typedef OrderedStrMap<String>::const_iterator Iterator;
		Iterator GetIterator() const;
		Iterator End() const;

	private:
		bool GetValue(const char *key, const String** pStr) const;

		friend class ConfigFile;
		OrderedStrMap<String> _dataPairs;
	};

	class ConfigFile
	{
	public:
		ConfigFile();
		~ConfigFile();

		bool Load(const char * fileName);
		bool Save(const char * fileName);

		ConfigSection* AddSection(const char * section);
		ConfigSection* GetSection(const char * section);
		ConfigSection* operator[](const char * section);
		const ConfigSection* GetSection(const char* section) const;
		const ConfigSection* operator[](const char* section) const;

		const String& GetFilename() const;

		void Clear();

	private:
		String _filename;
		OrderedStrMap<ConfigSection> _sections;

	};
}

