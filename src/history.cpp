#include "backup.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

namespace rb {
namespace fs = std::filesystem;
namespace {
bool runCapture(const std::vector<std::string>& args, int timeout, const std::string& output) {
    if (args.empty()) return false;
    pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0) {
        int fd = open(output.c_str(), O_WRONLY | O_TRUNC);
        if (fd < 0 || dup2(fd, STDOUT_FILENO) < 0) _exit(126);
        close(fd);
        int nullfd = open("/dev/null", O_WRONLY);
        if (nullfd >= 0) { dup2(nullfd, STDERR_FILENO); close(nullfd); }
        std::vector<char*> argv;
        for (const auto& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        _exit(errno == ENOENT ? 127 : 126);
    }
    int status = 0;
    for (int i = 0; i < timeout * 10; ++i) {
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) return WIFEXITED(status) && WEXITSTATUS(status) == 0;
        if (result < 0) return false;
        usleep(100000);
    }
    kill(pid, SIGTERM);
    for (int i = 0; i < 20; ++i) {
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) return false;
        usleep(100000);
    }
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);
    return false;
}
std::string quoteSftp(const std::string& value) {
    std::string result = "\"";
    for (unsigned char ch : value) {
        if (ch < 0x20 || ch == 0x7f) return {};
        if (ch == '\\' || ch == '"') result += '\\';
        result += static_cast<char>(ch);
    }
    return result + "\"";
}
bool stageKey(const std::string& source, std::string& staged) {
    std::ifstream input(source, std::ios::binary);
    if (!input.good()) return false;
    char path[] = "/tmp/rb-history-key-XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) return false;
    bool ok = true;
    char buffer[4096];
    while (input.good()) {
        input.read(buffer, sizeof(buffer));
        std::streamsize count = input.gcount(), offset = 0;
        while (offset < count) {
            ssize_t written = write(fd, buffer + offset, static_cast<size_t>(count - offset));
            if (written <= 0) { ok = false; break; }
            offset += written;
        }
        if (!ok) break;
    }
    if (input.bad() || fchmod(fd, 0600) != 0) ok = false;
    close(fd);
    if (!ok) { unlink(path); return false; }
    staged = path;
    return true;
}
bool validBackupId(const std::string& id) {
    if (id.size() < 18 || id[8] != 'T' || id[15] != 'Z' || id[16] != '-') return false;
    for (size_t i = 0; i < 16; ++i)
        if (i != 8 && !std::isdigit(static_cast<unsigned char>(id[i]))) return false;
    for (size_t i = 17; i < id.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(id[i]))) return false;
    return true;
}
bool runSftp(const Config& config, const ConfigStore& store, const std::string& batch,
             const std::string& output, int timeout) {
    char* end = nullptr;
    long port = std::strtol(config.port.c_str(), &end, 10);
    if (!end || *end || port < 1 || port > 65535 || config.host.empty() || config.username.empty())
        return false;
    const std::string key = store.resolve(config.identityFile);
    const std::string known = store.resolve(config.knownHostsFile);
    struct stat st{};
    if (stat(key.c_str(), &st) != 0 || !S_ISREG(st.st_mode) ||
        stat(known.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
    std::string staged;
    if (!stageKey(key, staged)) return false;
    bool ok = runCapture({"sftp", "-q", "-b", batch, "-P", config.port, "-i", staged,
        "-oBatchMode=yes", "-oIdentitiesOnly=yes", "-oStrictHostKeyChecking=yes",
        "-oUserKnownHostsFile=" + known, "-oConnectTimeout=8",
        config.username + "@" + config.host}, timeout, output);
    unlink(staged.c_str());
    return ok;
}
bool createTemp(const std::string& directory, const std::string& prefix, std::string& path) {
    std::string pattern = directory + "/" + prefix + "-XXXXXX";
    std::vector<char> data(pattern.begin(), pattern.end());
    data.push_back('\0');
    int fd = mkstemp(data.data());
    if (fd < 0) return false;
    close(fd);
    path = data.data();
    return true;
}
bool safeArchive(const std::string& archive, const std::string& rootName,
                 const std::string& listing, const std::string& types) {
    if (!runCapture({"tar", "-tf", archive}, 180, listing) ||
        !runCapture({"tar", "-tvf", archive}, 180, types)) return false;
    std::ifstream typeIn(types);
    std::string line;
    while (std::getline(typeIn, line)) {
        if (line.empty()) continue;
        if (line[0] != '-' && line[0] != 'd') return false;
    }
    if (!typeIn.eof()) return false;
    std::ifstream pathIn(listing);
    bool foundRoot = false;
    while (std::getline(pathIn, line)) {
        while (!line.empty() && line.back() == '/') line.pop_back();
        if (line.empty() || line[0] == '/' || line.find('\\') != std::string::npos) return false;
        std::stringstream parts(line);
        std::string part;
        while (std::getline(parts, part, '/'))
            if (part.empty() || part == "." || part == "..") return false;
        if (line == rootName || line.compare(0, rootName.size() + 1, rootName + "/") == 0)
            foundRoot = true;
        else return false;
    }
    return pathIn.eof() && foundRoot;
}
}
bool listRemoteBackups(const Config& config, const ConfigStore& store,
                       std::vector<std::string>& backups, std::string& message) {
    backups.clear();
    if (config.remotePath.empty() || config.remotePath.find_first_of("\r\n") != std::string::npos) {
        message = "INVALID REMOTE PATH"; return false;
    }
    std::string batch, output;
    if (!createTemp("/tmp", "rb-ls-batch", batch) || !createTemp("/tmp", "rb-ls-output", output)) {
        message = "CANNOT CREATE TEMP FILES"; return false;
    }
    const std::string remote = quoteSftp(config.remotePath);
    if (remote.empty()) { unlink(batch.c_str()); unlink(output.c_str()); message = "INVALID REMOTE PATH"; return false; }
    { std::ofstream out(batch); out << "cd " << remote << "\nls -la\n"; }
    bool ok = runSftp(config, store, batch, output, 60);
    unlink(batch.c_str());
    if (!ok) { unlink(output.c_str()); message = "CANNOT READ REMOTE BACKUP HISTORY"; return false; }
    std::ifstream in(output);
    std::string line;
    while (std::getline(in, line)) {
        if (line.size() < 2 || line[0] != 'd') continue;
        const size_t pos = line.find_last_of(" \t");
        if (pos == std::string::npos) continue;
        std::string name = line.substr(pos + 1);
        if (validBackupId(name)) backups.push_back(name);
    }
    unlink(output.c_str());
    std::sort(backups.begin(), backups.end(), std::greater<std::string>());
    message = backups.empty() ? "NO COMPLETED BACKUPS FOUND" : std::to_string(backups.size()) + " BACKUPS FOUND";
    return true;
}

bool restoreRemoteBackup(const Config& config, const ConfigStore& store, const std::string& id,
                         std::string& message) {
    if (!validBackupId(id)) { message = "INVALID BACKUP SELECTION"; return false; }
    if (config.remotePath.empty() || config.remotePath.find_first_of("\r\n") != std::string::npos) {
        message = "INVALID REMOTE PATH"; return false;
    }
    const fs::path target(store.resolve(config.savesPath));
    std::error_code ec;
    if (!fs::is_directory(target, ec) || ec) { message = "SAVES FOLDER NOT FOUND"; return false; }
    const std::string rootName = target.filename().string();
    if (rootName.empty() || rootName == "." || rootName == "..") { message = "INVALID SAVES PATH"; return false; }

    std::string archive, manifest, listing, types, batch, output;
    if (!createTemp(store.appDir(), ".rb-restore-archive", archive) ||
        !createTemp("/tmp", "rb-restore-manifest", manifest) ||
        !createTemp("/tmp", "rb-restore-list", listing) ||
        !createTemp("/tmp", "rb-restore-types", types) ||
        !createTemp("/tmp", "rb-restore-batch", batch) ||
        !createTemp("/tmp", "rb-restore-output", output)) {
        message = "CANNOT CREATE RESTORE TEMP FILES"; return false;
    }
    auto cleanup = [&]() {
        unlink(archive.c_str()); unlink(manifest.c_str()); unlink(listing.c_str());
        unlink(types.c_str()); unlink(batch.c_str()); unlink(output.c_str());
    };
    const std::string remote = quoteSftp(config.remotePath + "/" + id);
    if (remote.empty()) { cleanup(); message = "INVALID REMOTE PATH"; return false; }
    {
        std::ofstream out(batch);
        out << "cd " << remote << "\n"
            << "get backup.tar " << quoteSftp(archive) << "\n"
            << "get manifest.sha256 " << quoteSftp(manifest) << "\n";
        if (!out.good()) { cleanup(); message = "CANNOT PREPARE RESTORE"; return false; }
    }
    if (!runSftp(config, store, batch, output, 1800)) {
        cleanup(); message = "BACKUP DOWNLOAD FAILED"; return false;
    }
    std::ifstream manifestIn(manifest);
    std::string expected, filename;
    manifestIn >> expected >> filename;
    if (expected.size() != 64 || filename != "backup.tar" ||
        !std::all_of(expected.begin(), expected.end(), [](unsigned char ch) { return std::isxdigit(ch); })) {
        cleanup(); message = "INVALID BACKUP MANIFEST"; return false;
    }
    std::string hashOutput;
    if (!createTemp("/tmp", "rb-restore-hash", hashOutput) ||
        !runCapture({"sha256sum", archive}, 180, hashOutput)) {
        if (!hashOutput.empty()) unlink(hashOutput.c_str());
        cleanup(); message = "CANNOT VERIFY BACKUP CHECKSUM"; return false;
    }
    std::ifstream hashIn(hashOutput);
    std::string actual; hashIn >> actual;
    unlink(hashOutput.c_str());
    if (actual != expected) { cleanup(); message = "BACKUP CHECKSUM MISMATCH"; return false; }
    if (!safeArchive(archive, rootName, listing, types)) {
        cleanup(); message = "UNSAFE OR INVALID BACKUP ARCHIVE"; return false;
    }

    fs::path parent = target.parent_path();
    std::string stageTemplate = (parent / ".rb-restore-XXXXXX").string();
    std::vector<char> stageBuffer(stageTemplate.begin(), stageTemplate.end());
    stageBuffer.push_back('\0');
    char* stageRaw = mkdtemp(stageBuffer.data());
    if (!stageRaw) { cleanup(); message = "CANNOT CREATE RESTORE STAGING DIRECTORY"; return false; }
    fs::path stage(stageRaw);
    if (!runCapture({"tar", "-xf", archive, "-C", stage.string(), "--no-same-owner", "--no-same-permissions"}, 900, output)) {
        fs::remove_all(stage, ec); cleanup(); message = "BACKUP EXTRACTION FAILED"; return false;
    }
    fs::path extracted = stage / rootName;
    if (!fs::is_directory(extracted, ec) || ec) {
        fs::remove_all(stage, ec); cleanup(); message = "RESTORED SAVES DIRECTORY MISSING"; return false;
    }
    time_t now = time(nullptr);
    tm utc{}; gmtime_r(&now, &utc);
    char stamp[32]{}; strftime(stamp, sizeof(stamp), "%Y%m%dT%H%M%SZ", &utc);
    fs::path rollback = parent / (rootName + ".rollback-" + std::string(stamp));
    if (fs::exists(rollback, ec) || ec) {
        fs::remove_all(stage, ec); cleanup(); message = "ROLLBACK PATH ALREADY EXISTS"; return false;
    }
    fs::rename(target, rollback, ec);
    if (ec) {
        fs::remove_all(stage, ec); cleanup(); message = "CANNOT MOVE CURRENT SAVES TO ROLLBACK"; return false;
    }
    fs::rename(extracted, target, ec);
    if (ec) {
        std::error_code restoreError;
        fs::rename(rollback, target, restoreError);
        fs::remove_all(stage, restoreError); cleanup();
        message = restoreError ? "RESTORE FAILED - CHECK ROLLBACK DIRECTORY" : "RESTORE FAILED - ORIGINAL SAVES RESTORED";
        return false;
    }
    fs::remove_all(stage, ec);
    cleanup();
    message = "RESTORE COMPLETE - OLD SAVES: " + rollback.filename().string();
    return true;
}
}
