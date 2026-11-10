#include "sftp.hpp"
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <fcntl.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
namespace rb {
namespace { bool regularFile(const std::string& p,struct stat& s){return !p.empty()&&stat(p.c_str(),&s)==0&&S_ISREG(s.st_mode);} }
bool testSftpConnection(const Config& c,const ConfigStore& store,std::string& message){
    if(c.host.empty()||c.username.empty()){message="SET HOST AND USERNAME FIRST";return false;}
    char* end=nullptr; const long port=std::strtol(c.port.c_str(),&end,10);
    if(!end||*end!='\0'||port<1||port>65535){message="INVALID SFTP PORT";return false;}
    const std::string key=store.resolve(c.identityFile), known=store.resolve(c.knownHostsFile);
    struct stat ks{},hs{};
    if(!regularFile(key,ks)){message="SSH PRIVATE KEY NOT FOUND";return false;}
    if(!regularFile(known,hs)){message="KNOWN_HOSTS FILE NOT FOUND";return false;}
    // SD cards are commonly FAT/exFAT and cannot preserve Unix mode 0600.
    // Copy the key into a private temporary file so OpenSSH accepts it safely.
    std::ifstream keyInput(key, std::ios::binary);
    if(!keyInput.good()){message="CANNOT READ SSH PRIVATE KEY";return false;}
    char tempTemplate[]="/tmp/retro-backup-key-XXXXXX";
    const int tempFd=mkstemp(tempTemplate);
    if(tempFd<0){message="CANNOT CREATE TEMPORARY SSH KEY";return false;}
    bool copied=true;
    char buffer[4096];
    while(keyInput.good()){
        keyInput.read(buffer,sizeof(buffer));
        const std::streamsize count=keyInput.gcount();
        std::streamsize offset=0;
        while(offset<count){
            const ssize_t written=write(tempFd,buffer+offset,static_cast<size_t>(count-offset));
            if(written<=0){copied=false;break;}
            offset+=written;
        }
        if(!copied)break;
    }
    if(keyInput.bad())copied=false;
    if(fchmod(tempFd,0600)!=0)copied=false;
    close(tempFd);
    if(!copied){unlink(tempTemplate);message="CANNOT STAGE SSH PRIVATE KEY";return false;}
    const std::string stagedKey=tempTemplate;
    const std::string dest=c.username+"@"+c.host;
    std::vector<std::string> a={"sftp","-b","/dev/null","-P",c.port,"-i",stagedKey,
      "-oBatchMode=yes","-oIdentitiesOnly=yes","-oStrictHostKeyChecking=yes",
      "-oUserKnownHostsFile="+known,"-oConnectTimeout=8",dest};
    std::vector<char*> av; for(auto& x:a)av.push_back(const_cast<char*>(x.c_str())); av.push_back(nullptr);
    pid_t pid=fork(); if(pid<0){unlink(stagedKey.c_str());message="CANNOT START SFTP CLIENT";return false;}
    if(pid==0){execvp(av[0],av.data());_exit(errno==ENOENT?127:126);}
    int status=0;
    for(int i=0;i<120;++i){
        pid_t r=waitpid(pid,&status,WNOHANG);
        if(r==pid){
            unlink(stagedKey.c_str());
            if(WIFEXITED(status)&&WEXITSTATUS(status)==0){message="SFTP CONNECTION OK";return true;}
            message=(WIFEXITED(status)&&WEXITSTATUS(status)==127)?"SFTP CLIENT NOT INSTALLED":"SFTP AUTH OR HOST KEY FAILED";
            return false;
        }
        if(r<0){message="SFTP CHECK FAILED";return false;}
        usleep(100000);
    }
    kill(pid,SIGTERM); waitpid(pid,&status,0); unlink(stagedKey.c_str()); message="SFTP CONNECTION TIMEOUT"; return false;
}
}