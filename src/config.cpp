#include "config.hpp"
#include <fstream>
#include <map>
#include <sys/stat.h>
#include <utility>
namespace rb {
namespace { std::string trim(const std::string&v){auto a=v.find_first_not_of(" \t\r\n");if(a==std::string::npos)return{};auto b=v.find_last_not_of(" \t\r\n");return v.substr(a,b-a+1);} }
ConfigStore::ConfigStore(std::string d):appDir_(std::move(d)),configPath_(appDir_+"/config.ini"){}
bool ConfigStore::load(Config&c,std::string&e)const{std::ifstream in(configPath_);if(!in.good()){e="CONFIG NOT FOUND - EDIT SETTINGS";return false;}std::map<std::string,std::string>v;std::string l;while(std::getline(in,l)){l=trim(l);if(l.empty()||l[0]=='#'||l[0]==';')continue;auto p=l.find('=');if(p!=std::string::npos)v[trim(l.substr(0,p))]=trim(l.substr(p+1));}c.host=v["host"];if(!v["port"].empty())c.port=v["port"];c.username=v["username"];if(!v["remote_path"].empty())c.remotePath=v["remote_path"];if(!v["identity_file"].empty())c.identityFile=v["identity_file"];if(!v["known_hosts_file"].empty())c.knownHostsFile=v["known_hosts_file"];if(!v["saves_path"].empty())c.savesPath=v["saves_path"];return true;}
bool ConfigStore::save(const Config&c,std::string&e)const{std::ofstream o(configPath_,std::ios::trunc);if(!o.good()){e="CANNOT WRITE CONFIG";return false;}o<<"# Retro Backup SFTP settings\nhost="<<c.host<<"\nport="<<c.port<<"\nusername="<<c.username<<"\nremote_path="<<c.remotePath<<"\nidentity_file="<<c.identityFile<<"\nknown_hosts_file="<<c.knownHostsFile<<"\nsaves_path="<<c.savesPath<<"\n";o.close();if(!o.good()){e="CONFIG WRITE FAILED";return false;}chmod(configPath_.c_str(),0600);return true;}
std::string ConfigStore::resolve(const std::string&p)const{if(p.empty()||p[0]=='/')return p;return appDir_+"/"+p;}
}
