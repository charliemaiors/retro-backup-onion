#pragma once
#include <string>
namespace rb {
struct Config {
    std::string host;
    std::string port{"22"};
    std::string username;
    std::string remotePath{"/retro-backup"};
    std::string identityFile{"id_ed25519"};
    std::string knownHostsFile{"known_hosts"};
    std::string savesPath{"../../Saves"};
};
class ConfigStore {
public:
    explicit ConfigStore(std::string appDir);
    bool load(Config& config, std::string& error) const;
    bool save(const Config& config, std::string& error) const;
    std::string resolve(const std::string& path) const;
    const std::string& appDir() const { return appDir_; }
private:
    std::string appDir_;
    std::string configPath_;
};
}