#pragma once

#include <memory>
#include <vector>
#include <string>

namespace DEBAR {

struct PackageInfo;
typedef std::shared_ptr<PackageInfo> PackageInfoPtr;
struct PackageInfo {
    std::string name;
    std::string version;
    std::string description;
    std::vector<PackageInfoPtr> depends;
    std::vector<PackageInfoPtr> suggests;
    std::string filename;
    size_t size;
    std::string md5;
};

}
