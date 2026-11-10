#include "backup.hpp"
#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
namespace rb {
namespace fs=std::filesystem;
namespace {
bool fileExists(const std::string&p){struct stat s{};return !p.empty()&&stat(p.c_str(),&s)==0&&S_ISREG(s.st_mode);}
bool run(const std::vector<std::string>&a,int timeout,const std::string&out=""){
 if(a.empty())return false;pid_t p=fork();if(p<0)return false;
 if(p==0){if(!out.empty()){int fd=open(out.c_str(),O_WRONLY|O_TRUNC);if(fd<0||dup2(fd,STDOUT_FILENO)<0)_exit(126);close(fd);}
 std::vector<char*>v;for(const auto&s:a)v.push_back(const_cast<char*>(s.c_str()));v.push_back(nullptr);execvp(v[0],v.data());_exit(errno==ENOENT?127:126);}
 int st=0;for(int i=0;i<timeout*10;++i){pid_t r=waitpid(p,&st,WNOHANG);if(r==p)return WIFEXITED(st)&&WEXITSTATUS(st)==0;if(r<0)return false;usleep(100000);}
 kill(p,SIGTERM);for(int i=0;i<20;++i){pid_t r=waitpid(p,&st,WNOHANG);if(r==p)return false;usleep(100000);}kill(p,SIGKILL);waitpid(p,&st,0);return false;
}
std::string q(const std::string&s){std::string o="\"";for(unsigned char c:s){if(c<0x20||c==0x7f)return{};if(c=='\\'||c=='"')o+='\\';o+=static_cast<char>(c);}return o+"\"";}
bool temp(const std::string&d,const std::string&n,std::string&p,int&fd){std::string x=d+"/"+n+"-XXXXXX";std::vector<char>b(x.begin(),x.end());b.push_back('\0');fd=mkstemp(b.data());if(fd<0)return false;p=b.data();return true;}
bool keyCopy(const std::string&s,std::string&d){std::ifstream in(s,std::ios::binary);if(!in.good())return false;char p[]="/tmp/rb-key-XXXXXX";int fd=mkstemp(p);if(fd<0)return false;bool ok=true;char b[4096];while(in.good()){in.read(b,sizeof(b));auto n=in.gcount();std::streamsize o=0;while(o<n){auto w=write(fd,b+o,static_cast<size_t>(n-o));if(w<=0){ok=false;break;}o+=w;}if(!ok)break;}if(in.bad()||fchmod(fd,0600)!=0)ok=false;close(fd);if(!ok){unlink(p);return false;}d=p;return true;}
}
bool createAndUploadBackup(const Config&c,const ConfigStore&s,std::string&m){
 if(c.host.empty()||c.username.empty()){m="CONFIGURE SFTP HOST AND USER";return false;}
 char*e=nullptr;long port=strtol(c.port.c_str(),&e,10);if(!e||*e||port<1||port>65535){m="INVALID SFTP PORT";return false;}
 std::string src=s.resolve(c.savesPath),key=s.resolve(c.identityFile),known=s.resolve(c.knownHostsFile);std::error_code ec;
 if(!fs::is_directory(src,ec)||ec){m="SAVES FOLDER NOT FOUND";return false;}
 if(!fileExists(key)){m="SSH PRIVATE KEY NOT FOUND";return false;}if(!fileExists(known)){m="KNOWN_HOSTS FILE NOT FOUND";return false;}
 if(c.remotePath.empty()||c.remotePath.find_first_of("\r\n")!=std::string::npos){m="INVALID REMOTE PATH";return false;}
 fs::path root(src);std::string rootName=root.filename().string();if(rootName.empty()||rootName=="."||rootName==".."){m="INVALID SAVES PATH";return false;}
 std::vector<std::string>files;uintmax_t bytes=0;
 for(fs::recursive_directory_iterator it(root,fs::directory_options::skip_permission_denied,ec),last;it!=last;it.increment(ec)){
  if(ec){ec.clear();continue;}auto st=it->symlink_status(ec);if(ec){ec.clear();continue;}
  if(fs::is_symlink(st)){if(fs::is_directory(st))it.disable_recursion_pending();continue;}
  if(fs::is_regular_file(st)){auto n=it->file_size(ec);auto rel=fs::relative(it->path(),root,ec);if(!ec){files.push_back((fs::path(rootName)/rel).generic_string());bytes+=n;}ec.clear();}
 }
 if(files.empty()){m="NO SAVE FILES FOUND";return false;}
 struct statvfs sv{};if(statvfs(s.appDir().c_str(),&sv)==0&&static_cast<uintmax_t>(sv.f_bavail)*sv.f_frsize<bytes+8U*1024U*1024U){m="NOT ENOUGH SD CARD SPACE";return false;}
 std::string arc,man,list,batch,hash;int a=-1,b=-1,l=-1,d=-1,h=-1;
 if(!temp(s.appDir(),".rb-archive",arc,a)||!temp(s.appDir(),".rb-manifest",man,b)||!temp("/tmp","rb-list",list,l)||!temp("/tmp","rb-batch",batch,d)){
  if(a>=0)close(a);if(b>=0)close(b);if(l>=0)close(l);if(d>=0)close(d);
  if(!arc.empty())unlink(arc.c_str());if(!man.empty())unlink(man.c_str());if(!list.empty())unlink(list.c_str());if(!batch.empty())unlink(batch.c_str());m="CANNOT CREATE TEMP FILES";return false;
 }
 close(a);close(b);close(l);close(d);auto clean=[&](){unlink(arc.c_str());unlink(man.c_str());unlink(list.c_str());unlink(batch.c_str());};
 {std::ofstream o(list,std::ios::binary);for(const auto&f:files){o.write(f.data(),static_cast<std::streamsize>(f.size()));o.put('\0');}if(!o.good()){clean();m="INVENTORY WRITE FAILED";return false;}}
 if(!run({"tar","--null","-T",list,"-cf",arc,"-C",root.parent_path().string()},900)){clean();m="TAR SNAPSHOT FAILED";return false;}
 if(!temp("/tmp","rb-hash",hash,h)){clean();m="CANNOT CREATE HASH FILE";return false;}close(h);
 if(!run({"sha256sum",arc},120,hash)){clean();unlink(hash.c_str());m="SHA256 TOOL FAILED";return false;}
 std::ifstream hi(hash);std::string line;std::getline(hi,line);unlink(hash.c_str());if(line.size()<64){clean();m="CHECKSUM FAILED";return false;}
 {std::ofstream o(man);o<<line.substr(0,64)<<"  backup.tar\n";if(!o.good()){clean();m="MANIFEST WRITE FAILED";return false;}}
 time_t now=time(nullptr);tm utc{};gmtime_r(&now,&utc);char stamp[32]{};strftime(stamp,sizeof(stamp),"%Y%m%dT%H%M%SZ",&utc);
 std::string id=std::string(stamp)+"-"+std::to_string(getpid()),tmpdir=".incomplete-"+id;std::string qr=q(c.remotePath),qa=q(arc),qm=q(man);
 if(qr.empty()||qa.empty()||qm.empty()){clean();m="INVALID SFTP PATH";return false;}
 {std::ofstream o(batch);o<<"cd "<<qr<<"\nmkdir "<<q(tmpdir)<<"\nput "<<qa<<" "<<q(tmpdir+"/backup.tar")<<"\nput "<<qm<<" "<<q(tmpdir+"/manifest.sha256")<<"\nrename "<<q(tmpdir)<<" "<<q(id)<<"\n";if(!o.good()){clean();m="SFTP BATCH WRITE FAILED";return false;}}
 std::string staged;if(!keyCopy(key,staged)){clean();m="CANNOT STAGE SSH KEY";return false;}
 bool ok=run({"sftp","-q","-b",batch,"-P",c.port,"-i",staged,"-oBatchMode=yes","-oIdentitiesOnly=yes","-oStrictHostKeyChecking=yes","-oUserKnownHostsFile="+known,"-oConnectTimeout=8",c.username+"@"+c.host},1800);
 unlink(staged.c_str());clean();if(!ok){m="UPLOAD FAILED; INCOMPLETE REMOTE DATA MAY REMAIN";return false;}
 std::ostringstream out;out<<"BACKUP "<<id<<" - "<<files.size()<<" FILES";m=out.str();return true;
}
}
