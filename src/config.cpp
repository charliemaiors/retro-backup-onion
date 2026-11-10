#include "config.hpp"
#include <fstream>
#include <map>
#include <sys/stat.h>
#include <utility>
namespace rb {
namespace {
std::string trim(const std::string& value) {
    const auto first=value.find_first_not_of(" \t\r\n");
    if(first==std::string::npos) return {};
    const auto last=value.find_last_not_of(" \t\r\n");
    return value.substr(first,last-first+1);
}
}
ConfigStore::ConfigStore(std::string appDir):appDir_(std::move(appDir)),configPath_(appDir_+"/config.ini"){}
bool ConfigStore::load(Config& c,std::string& error) const {
    std::ifstream in(configPath_);
    if(!in.good()){error="CONFIG NOT FOUND - EDIT SETTINGS";return false;}
    std::map<std::string,std::string> v; std::string line;
    while(std::getline(in,line)){
        line=trim(line); if(line.empty()||line[0]=='#'||line[0]==';')continue;
        const auto p=line.find('='); if(p!=std::string::npos)v[trim(line.substr(0,p))]=trim(line.substr(p+1));
    }
    c.host=v["host"]; if(!v["port"].empty())c.port=v["port"]; c.username=v["username"];
    if(!v["remote_path"].empty())c.remotePath=v["remote_path"];
    if(!v["identity_file"].empty())c.identityFile=v["identity_file"];
    if(!v["known_hosts_file"].empty())c.knownHostsFile=v["known_hosts_file"];
    return true;
}
bool ConfigStore::save(const Config& c,std::string& error) const {
    std::ofstream out(configPath_,std::ios::trunc);
    if(!out.good()){error="CANNOT WRITE CONFIG";return false;}
    out<<"# Retro Backup SFTP settings\n"
       <<"host="<<c.host<<"\nport="<<c.port<<"\nusername="<<c.username
       <<"\nremote_path="<<c.remotePath<<"\nidentity_file="<<c.identityFile
       <<"\nknown_hosts_file="<<c.knownHostsFile<<"\n";
    out.close(); if(!out.good()){error="CONFIG WRITE FAILED";return false;}
    chmod(configPath_.c_str(),0600); return true;
}
std::string ConfigStore::resolve(const std::string& path) const {
    if(path.empty()||path[0]=='/')return path; return appDir_+"/"+path;
}
}