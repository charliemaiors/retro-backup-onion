#pragma once
#include "config.hpp"
#include <string>
#include <vector>
namespace rb {
bool createAndUploadBackup(const Config&, const ConfigStore&, std::string&);
bool listRemoteBackups(const Config&, const ConfigStore&, std::vector<std::string>&, std::string&);
bool restoreRemoteBackup(const Config&, const ConfigStore&, const std::string&, std::string&);
}
