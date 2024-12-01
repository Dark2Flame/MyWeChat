#include "ConfigMgr.h"

ConfigMgr::ConfigMgr()
{
	boost::filesystem::path current_path = boost::filesystem::current_path();
	boost::filesystem::path config_path = current_path / "config.ini";

	std::cout << "Config path: " << config_path << std::endl;

	boost::property_tree::ptree pt;
	boost::property_tree::read_ini(config_path.string(), pt); // 按树形结构读配置文件
	for (const auto& section_pair : pt)
	{
		const std::string& section_name = section_pair.first;
		const boost::property_tree::ptree& section_tree = section_pair.second;
		std::map<std::string, std::string> section_config;
		for (const auto& key_value_pair : section_tree)
		{
			const std::string& key = key_value_pair.first;
			const std::string& value = key_value_pair.second.get_value<std::string>();
			section_config[key] = value;
		}

		SectionInfo sectonInfo;
		sectonInfo._section_datas = section_config;
		_config_map[section_name] = sectonInfo;
	}

	// 输出所有的section和key-value对  
	for (const auto& section_entry : _config_map) {
		const std::string& section_name = section_entry.first;
		SectionInfo section_config = section_entry.second;
		std::cout << "[" << section_name << "]" << std::endl;
		for (const auto& key_value_pair : section_config._section_datas) {
			std::cout << key_value_pair.first << "=" << key_value_pair.second << std::endl;
		}
	}
}

ConfigMgr::ConfigMgr(const ConfigMgr& src)
{
	_config_map = src._config_map;
}

ConfigMgr::~ConfigMgr()
{
	_config_map.clear();
}

ConfigMgr& ConfigMgr::operator=(const ConfigMgr& src)
{
	if (&src == this)
	{
		return *this;
	}
	_config_map = src._config_map;
	return *this;
}

SectionInfo ConfigMgr::operator[](const std::string& section)
{
	if (_config_map.find(section) == _config_map.end())
	{
		return SectionInfo();
	}
	return _config_map[section];
}
