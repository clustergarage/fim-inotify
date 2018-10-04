#include "fimd_util.h"

#include <glob.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fimd {
// @TODO: document this
int FimdUtil::getPidForContainer(std::string id) {
    int pid = 0;
    std::string cgroupType = "memory";
    std::string cgroupRoot = findCgroupMountpoint(cgroupType);
    std::string cgroupThis = getThisCgroup(cgroupType);

    id += '*';
    std::vector<std::string> attempts = {
        cgroupRoot + cgroupThis + '/' + id + "/tasks",
        // with more recent lxc, cgroup will be in lxc/
        cgroupRoot + cgroupThis + "/lxc/" + id + "/tasks",
        // with more recent docker, cgroup will be in docker/
        cgroupRoot + cgroupThis + "/docker/" + id + "/tasks",
        // even more recent docker versions under systemd, use docker-<id>.scope/
        cgroupRoot + "/system.slice/docker-" + id + ".scope/tasks",
        // even more recent docker versions under cgroup/systemd/docker/<id>/
        cgroupRoot + "/../systemd/docker/" + id + "/tasks",
        // kubernetes with docker and CNI is even more different
        cgroupRoot + "/../systemd/kubepods/*/pod*/" + id + "/tasks"
    };

    for (auto it = attempts.begin(); it != attempts.end(); ++it) {
        auto files = fglob(*it);
        if (files.size() > 0) {
            std::ifstream output(files[0]);
            std::string line;
            std::getline(output, line);
            pid = atoi(line.c_str());
        }
    }
    return pid;
}

std::vector<std::string> FimdUtil::fglob(const std::string &pattern) {
    std::vector<std::string> filenames;
    glob_t globResult;
    int err = glob(pattern.c_str(), GLOB_TILDE, NULL, &globResult);
    if (err == 0) {
        for (size_t i = 0; i < globResult.gl_pathc; ++i) {
            filenames.push_back(std::string(globResult.gl_pathv[i]));
        }
    }
    globfree(&globResult);
    return filenames;
}

std::string FimdUtil::findCgroupMountpoint(std::string cgroupType) {
    std::ifstream output("/proc/mounts");
    std::string line;
    // /proc/mounts has 6 fields per line, one mount per line, e.g.
    // cgroup /sys/fs/cgroup/devices cgroup rw,relatime,devices 0 0
    while (std::getline(output, line)) {
        std::string fsSpec, fsFile, fsVfstype, fsMntops, fsFreq, fsPassno;
        output >> fsSpec >> fsFile >> fsVfstype >> fsMntops >> fsFreq >> fsPassno;
        if (fsVfstype == "cgroup") {
            std::vector<std::string> results = split(fsMntops, ',');
            for (auto it = results.begin(); it != results.end(); ++it) {
                if ((*it) == cgroupType) {
                    return fsFile;
                }
            }
        }
    }
}

// returns the relative path to the cgroup docker is running in
std::string FimdUtil::getThisCgroup(std::string cgroupType) {
    std::ifstream dockerpid("/var/run/docker.pid");
    std::string line;
    std::getline(dockerpid, line);
    // split by \n, check if len == 0 || len result[0] == 0
    int pid = atoi(line.c_str());

    std::stringstream ss;
    ss << "/proc/" << pid << "/cgroup";
    std::ifstream output(ss.str());
    while (std::getline(output, line)) {
        auto results = split(line, ':');
        if (results[1] == cgroupType) {
            return results[2];
        }
    }
}
} // namespace fimd
