#include "ConfigMgr.h"
#include "const.h"

SectionInfo& SectionInfo::operator=(const SectionInfo& other) {
	if (&other == this) {
		return *this;
	}
	this->_section_datas = other._section_datas;
	return *this;
}

std::string SectionInfo::operator[](const std::string& key) {
	if (_section_datas.find(key) == _section_datas.end()) {
		return "";
	}
	return _section_datas[key];
}

ConfigMgr::ConfigMgr() {
	boost::filesystem::path current_path = boost::filesystem::current_path();
	boost::filesystem::path config_path = current_path / "config.ini";
	std::cout << "--------- config.ini path:" << config_path << " ---------" << std::endl; 

	// 解析 ini 文件到 pt
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini(config_path.string(), pt);

	// 遍历 ini 文件加载所有配置项
	for (const auto& section_pair : pt) {
		const std::string& section_name = section_pair.first;
		const boost::property_tree::ptree& section_tree = section_pair.second;

		// 每个 section 遍历存储所有 kv 对
		std::map<std::string, std::string> section_config;
		for (const auto& kv_pair : section_tree) {
			const std::string& key = kv_pair.first;
			const std::string& val = kv_pair.second.get_value<std::string>();
			section_config[key] = val;
		}

		SectionInfo section_info;
		section_info._section_datas = section_config;
		_config_map[section_name] = section_info;
	}

	// 输出打印配置项
	for (const auto& section_entry : _config_map) {
		const std::string& section_name = section_entry.first;
		SectionInfo section_config = section_entry.second;
		std::cout << "[" << section_name << "]" << std::endl;
		for (const auto& kv_pair: section_config._section_datas) {
			std::cout << kv_pair.first << "=" << kv_pair.second << std::endl;
		}
	}
}

ConfigMgr& ConfigMgr::GetInstance()
{
	static ConfigMgr config_mgr;
	return config_mgr;
}

ConfigMgr::~ConfigMgr() {
	_config_map.clear();
}

SectionInfo ConfigMgr::operator[](const std::string& section) {
	if (_config_map.find(section) == _config_map.end()) {
		return SectionInfo();
	}
	return _config_map[section];
}

ConfigMgr& ConfigMgr::operator=(const ConfigMgr& other) {
	if (&other == this) {
		return *this;
	}
	this->_config_map = other._config_map;
}

ConfigMgr::ConfigMgr(const ConfigMgr& other) {
	this->_config_map = other._config_map;
}
