#pragma once
#include "const.h"
#include <map>
#include <string>
// �Լ���װ�������ݽṹ�͹�����


struct SectionInfo
{
	SectionInfo() {}
	~SectionInfo() {}

	// ��������
	SectionInfo(const SectionInfo& src)
	{
		_section_datas = src._section_datas;
	}

	// ��ֵ�����
	SectionInfo& operator=(const SectionInfo& src)
	{
		if (&src == this)
		{
			return *this;
		}

		this->_section_datas = src._section_datas;
		return *this;
	}

	std::string operator[](const std::string& key)
	{
		if (_section_datas.find(key) == _section_datas.end())
		{
			return "";
		}
		return _section_datas[key];
	}

	std::map<std::string, std::string> _section_datas;
};


class ConfigMgr
{
public:
	~ConfigMgr();
	ConfigMgr& operator=(const ConfigMgr& src);
	SectionInfo operator[](const std::string& section);

	static ConfigMgr& GetInstance()
	{
		static ConfigMgr instance;
		return instance;
	}
private:
	ConfigMgr();
	ConfigMgr(const ConfigMgr& src);
	std::map<std::string, SectionInfo> _config_map;
};