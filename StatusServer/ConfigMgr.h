#pragma once

#include <map>
#include <string>

struct SectionInfo {
  SectionInfo() {}
  ~SectionInfo() { _section_datas.clear(); }
  SectionInfo(const SectionInfo &other) {
    _section_datas = other._section_datas;
  }
  SectionInfo &operator=(const SectionInfo &other);
  std::string operator[](const std::string &key);
  std::map<std::string, std::string> _section_datas;
};

class ConfigMgr {
public:
  static ConfigMgr &GetInstance();
  ~ConfigMgr();
  SectionInfo operator[](const std::string &section);
  ConfigMgr &operator=(const ConfigMgr &other);
  ConfigMgr(const ConfigMgr &other);

private:
  ConfigMgr();

  std::map<std::string, SectionInfo> _config_map;
};
