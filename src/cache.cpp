#include "cache.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <curl/curl.h>
#include <zlib.h>
#include <string.h>

#include "cmd.h"
#include "utils.h"

using namespace DEBAR;
namespace fs = std::filesystem;

#define CACHE_INS DEBAR::Cache::instance()

Cache *Cache::m_instance = nullptr;

struct DEBAR::CachePrivate
{
    std::string path = ".";
    std::string repo_url = "";
    std::string arch = "";
    std::string release_name = "";
    std::vector<std::string> components;

    std::map<std::string, PackageInfoPtr> already_found;
    std::set<std::string> already_not_found;
    std::set<std::string> exclude;
    std::map<std::string, PackageInfoPtr> already_download;
};

Cache *DEBAR::Cache::instance()
{
    if (m_instance == nullptr)
    {
        m_instance = new Cache();
    }
    return m_instance;
}

bool DEBAR::Cache::init_work_directory()
{
    if (!fs::is_empty(CACHE_INS->d->path)) {
        std::cerr << "The work directory is not empty, you must choose a empty directory to work." << std::endl;
        return false;
    }

    fs::create_directory(CACHE_INS->d->path + "/.debar");
    
    YAML::Node config;
    config["repo"]["url"] = "https://mirrors.tuna.tsinghua.edu.cn/ubuntu/";
    config["repo"]["components"].push_back("main");
    config["repo"]["components"].push_back("restricted");
    config["repo"]["components"].push_back("universe");
    config["repo"]["components"].push_back("multiverse");
    config["repo"]["arch"] = "amd64";
    config["repo"]["release_name"] = "focal";

    std::ofstream configFile("config.yaml", std::ios::out);
    configFile << config;
    configFile.close();

    return true;
}

bool DEBAR::Cache::load_work_directory()
{
    try
    {
        YAML::Node config = YAML::LoadFile("config.yaml");

        CACHE_INS->d->repo_url = config["repo"]["url"].as<std::string>();
        CACHE_INS->d->components = config["repo"]["components"].as<std::vector<std::string>>();
        CACHE_INS->d->arch = config["repo"]["arch"].as<std::string>();
        CACHE_INS->d->release_name = config["repo"]["release_name"].as<std::string>();
        if (config["exclude"].IsDefined())
        {
            auto exclude = config["exclude"].as<std::vector<std::string>>();
            for (auto e : exclude)
            {
                CACHE_INS->d->exclude.insert(e);
            }
        }
    }
    catch(const YAML::BadFile& e)
    {
        std::cout << "The work directory is not initialized, you must run `debar --init` first." << std::endl;
        return false;
    }
    
    return true;
}

bool DEBAR::Cache::update_cache()
{
    std::cout << "Downloading Cache files..." << std::endl;
    std::ofstream indexFile(CACHE_INS->d->path + "/.debar/index", std::ios::out | std::ios::binary);

    for (int i = 0; i < CACHE_INS->d->components.size(); i++)
    {
        auto component = CACHE_INS->d->components[i];
        std::string url = CACHE_INS->d->repo_url + "dists/" + CACHE_INS->d->release_name + "/" + component + "/binary-" + CACHE_INS->d->arch + "/Packages.gz";
        std::string text = "Downloading " + component + " Packages.gz";
        auto packageZipFile = CACHE_INS->d->path + "/.debar/" + component + ".Packages.gz";
        if (!Utils::download_file(url, packageZipFile, text.c_str())) {
            std::cerr << "Failed to download file: " << url << std::endl;
            return false;
        }
        if (!unzip_gz_file(packageZipFile)) {
            std::cerr << "Failed to unzip file: " << packageZipFile << std::endl;
            return false;
        }
        auto packageFile = packageZipFile.substr(0, packageZipFile.find_last_of('.'));
        std::ifstream packageFileStream(packageFile, std::ios::in);
        if (!packageFileStream) {
            std::cerr << "Failed to open file: " << packageFile << std::endl;
            indexFile.close();
            return false;
        }
        std::string line;
        std::string packageName;
        std::streamoff pos_num = 0;
        while (std::getline(packageFileStream, line)) {
            if (line.find("Package: ") == 0) {
                packageName = line.substr(9);
                char name[128];
                memset(name, 0, 128);
                memcpy(name, packageName.c_str(), packageName.size());
                indexFile.write(name, 128); 
                memset(name, 0, 128);
                memcpy(name, component.c_str(), component.size());
                indexFile.write(name, 128);
                indexFile.write((char*)&pos_num, sizeof(std::streamoff));
            }
            pos_num = static_cast<std::streamoff>(packageFileStream.tellg());
        }
    }
    indexFile.flush();
    indexFile.close();

    std::cout << "Update Cache successfully." << std::endl;
    return true;
}

void get_all_package_depends(PackageInfoPtr package, std::vector<PackageInfoPtr> &data, int level = 0)
{
    static std::map<std::string, PackageInfoPtr> printed; 
    if (level == 0)
    {
        printed.clear();
    }
    printed.insert(std::pair<std::string, PackageInfoPtr>(package->name, package));
    data.push_back(package);
    for (auto dep : package->depends) {
        if (!dep)
            continue;
        if (printed.find(dep->name) == printed.end())
        get_all_package_depends(dep, data, level + 1);
    }
    if (CMD::is_suggests())
    {
        for (auto dep : package->suggests) {
            if (!dep)
                continue;
            if (printed.find(dep->name) == printed.end())
            get_all_package_depends(dep, data, level + 1);
        }
    }
}

bool DEBAR::Cache::download_package(const std::string &name)
{
    auto package = find_package(name);
    CACHE_INS->d->already_download.clear();
    std::cout << "You want to download package: \n" << std::endl;
    std::cout << "\t" << package->name << " (" << package->version << ")\n" << std::endl;
    std::cout << "This package has the following dependencies: \n" << std::endl;
    std::vector<PackageInfoPtr> depends;
    get_all_package_depends(package, depends);
    std::cout << "\t";
    size_t size = package->size;
    for (int i = 0; i < depends.size(); i++)
    {
        std::string text = depends[i]->name + " (" + depends[i]->version + ")";
        size += depends[i]->size;
        std::cout << text << "   ";
    }
    
    std::cout << "\n" << std::endl;
    std::cout << "All " << depends.size() + 1 << " packages, Total size " << Utils::format_size(size) << ".\n" << std::endl;
    __download_package(package);
    std::cout << "All packages are downloaded." << std::endl;


    return true;
}


bool DEBAR::Cache::__download_package(PackageInfoPtr package)
{
    if (CACHE_INS->d->already_download.find(package->name) != CACHE_INS->d->already_download.end()) return true;
    std::string url = CACHE_INS->d->repo_url + "/" + package->filename;
    std::string text = "Downloading: " + package->name + " (" + package->version + ")";
    std::string dir = CACHE_INS->d->path + "/packages";
    if (!fs::exists(dir)) {
        fs::create_directory(dir);
    }
    Utils::download_file(url, dir + package->filename.substr(package->filename.find_last_of("/")), text.c_str());
    CACHE_INS->d->already_download.insert(std::pair<std::string, PackageInfoPtr>(package->name, package));

    for (auto dep : package->depends)
    {
        __download_package(dep);
    }
    if (CMD::is_suggests())
    {
        for (auto suggests : package->suggests)
        {
            __download_package(suggests);
        }
    }

    return true;
}

bool DEBAR::Cache::unzip_gz_file(const std::string &path)
{
    gzFile gz_file = gzopen(path.c_str(), "rb");
    if (!gz_file) {
        std::cerr << "Failed to open gz file: " << path << std::endl;
        return false;
    }

    std::string output_path = path.substr(0, path.find_last_of('.'));
    std::ofstream out_file(output_path, std::ios::out | std::ios::binary);
    if (!out_file) {
        std::cerr << "Failed to create output file: " << output_path << std::endl;
        gzclose(gz_file);
        return false;
    }

    char buffer[4096];
    int bytes_read;
    while ((bytes_read = gzread(gz_file, buffer, sizeof(buffer))) > 0) {
        out_file.write(buffer, bytes_read);
    }

    if (bytes_read < 0) {
        int err;
        const char *error_string = gzerror(gz_file, &err);
        std::cerr << "gzread error: " << error_string << std::endl;
        gzclose(gz_file);
        out_file.close();
        return false;
    }

    gzclose(gz_file);
    out_file.close();
    return true;
}

struct PackageName {
    std::string name;
    std::string version;
};

typedef std::vector<PackageName> PackageItem;

PackageItem parsePackageItem(const std::string& item) {
    auto packages = Utils::split_str(item, " | ");
    PackageItem res;
    for (auto pkg : packages)
    {
        auto tmp = Utils::split_str(pkg, " (");
        PackageName name;
        name.name = tmp[0];
        if (name.name.find(":any") != std::string::npos)
        {
            name.name = name.name.substr(0, name.name.size() - 4);
        }
        
        if (tmp.size() > 1)
        {
            name.version = tmp[1].substr(0, tmp[1].size() - 1);
        }
        res.push_back(name);
    }
    return res;
}

PackageInfoPtr DEBAR::Cache::__find_package(const std::string &name)
{
    auto pos = find_package_pos(name);
    if (pos.name.empty()) return PackageInfoPtr();
    if (CACHE_INS->d->exclude.find(name) != CACHE_INS->d->exclude.end()) return PackageInfoPtr();

    std::ifstream packageFile(CACHE_INS->d->path + "/.debar/" + pos.component + ".Packages", std::ios::in);
    if (!packageFile) {
        std::cerr << "Failed to open package file: " << pos.component << ".Packages" << std::endl;
        return PackageInfoPtr();
    }

    packageFile.seekg(pos.pos, std::ios::beg);

    PackageInfoPtr package = std::make_shared<PackageInfo>();
    std::string line;
    std::string depends_str;
    std::string suggests_str;
    while (std::getline(packageFile, line)) {
        if (line.find("Package: ") == 0) {
            auto _name = line.substr(9);
            package->name = _name;
        } else if (line.find("Version: ") == 0) {
            package->version = line.substr(9);
        } else if (line.find("Filename: ") == 0) {
            package->filename = line.substr(10);
        } else if (line.find("Size: ") == 0) {
            package->size = std::stoul(line.substr(6));
        } else if (line.find("MD5sum: ") == 0) {
            package->md5 = line.substr(8);
        } else if (line.find("Depends: ") == 0) {
            depends_str = line.substr(9);
        } else if (line.find("Suggests: ") == 0) {
            suggests_str = line.substr(10);
        } else if (line.find("Description: ") == 0) {
            package->description = line.substr(13);
        } else if (line.empty()) {
            break;
        }
    }
    CACHE_INS->d->already_found.insert(std::pair<std::string, PackageInfoPtr>(package->name, package));

    if (!depends_str.empty())
    {
        auto depends = Utils::split_str(depends_str, ", ");
        for (auto dep_name : depends)
        {
            auto dep = parsePackageItem(dep_name);
            if (CACHE_INS->d->already_found.find(dep[0].name) == CACHE_INS->d->already_found.end())
            {   
                auto res = __find_package(dep[0].name);
                if (res) package->depends.push_back(res);
            } else {
                package->depends.push_back(CACHE_INS->d->already_found[dep[0].name]);
            }
        }
    }
    
    if (!suggests_str.empty())
    {
        auto suggests = Utils::split_str(suggests_str, ", ");
        for (auto suggests_name : suggests)
        {
            auto sug = parsePackageItem(suggests_name);
            if (CACHE_INS->d->already_found.find(sug[0].name) == CACHE_INS->d->already_found.end())
            {
                auto res = __find_package(sug[0].name);
                if (res) package->suggests.push_back(__find_package(sug[0].name));
            } else {
                package->suggests.push_back(CACHE_INS->d->already_found[sug[0].name]);
            }
        }
    }

    return package;
}


PackageInfoPtr DEBAR::Cache::find_package(const std::string &name) {
    CACHE_INS->d->already_found.clear();
    auto res = __find_package(name);
    return res;
}

InfoPos DEBAR::Cache::find_package_pos(const std::string &name)
{
    if (CACHE_INS->d->already_not_found.find(name) !=
        CACHE_INS->d->already_not_found.end())
    {
        return InfoPos();
    }
    
    std::ifstream indexFile(CACHE_INS->d->path + "/.debar/index", std::ios::in | std::ios::binary);
    if (!indexFile) {
        std::cerr << "Failed to open index file." << std::endl;
        return InfoPos();
    }

    char packageName[128];
    char component[128];
    std::streamoff pos;

    while (indexFile.read(packageName, 128)) {
        indexFile.read(component, 128);
        indexFile.read(reinterpret_cast<char*>(&pos), sizeof(std::streamoff));
        if (name == packageName) {
            InfoPos infoPos;
            infoPos.name = packageName;
            infoPos.component = component;
            infoPos.pos = pos;
            return infoPos;
        }
    }
    CACHE_INS->d->already_not_found.insert(name);
    return InfoPos();
}

Cache::Cache()
    : d(new CachePrivate())
{
}
