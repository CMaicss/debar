#include "cache.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <curl/curl.h>
#include <zlib.h>
#include <string.h>

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
        if (!download_file(url, packageZipFile, text.c_str())) {
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
}

std::string format_size(curl_off_t size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double display_size = static_cast<double>(size);
    while (display_size >= 1024 && unit_index < 4) {
        display_size /= 1024;
        unit_index++;
    }
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%.2f %s", display_size, units[unit_index]);
    return std::string(buffer);
};

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
    std::cout << "All " << depends.size() + 1 << " packages, Total size " << format_size(size) << ".\n" << std::endl;
    __download_package(package);
    std::cout << "All packages are downloaded." << std::endl;


    return true;
}

int progress_callback(void* ptr, curl_off_t total_to_download, curl_off_t now_downloaded, curl_off_t total_to_upload, curl_off_t now_uploaded)
{
    if (total_to_download > 0) {
        std::cout << (char*)ptr << " - " << (now_downloaded * 100 / total_to_download) << "% ("
                  << format_size(now_downloaded) << " / " << format_size(total_to_download) << ")           \r" << std::flush;
    }
    return 0;
}

bool DEBAR::Cache::download_file(const std::string &url, const std::string &path, const char* prefix)
{
    CURL *curl;
    FILE *fp;
    CURLcode res;
    char errorBuffer[CURL_ERROR_SIZE];
    
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(path.c_str(), "wb");
        if (!fp) {
            std::cerr << "Failed to open file: " << path << std::endl;
            return false;
        }
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "User-Agent: Debian APT-HTTP/1.3 (1.0.1ubuntu2)");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L); // 启用自定义进度输出
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, prefix);
        
        res = curl_easy_perform(curl);
        std::cout << std::endl;
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << errorBuffer << std::endl;
            fclose(fp);
            curl_easy_cleanup(curl);
            return false;
        }

        fclose(fp);
        curl_easy_cleanup(curl);
        
        return true;
    }
    return false;
}

bool DEBAR::Cache::__download_package(PackageInfoPtr package)
{
    if (CACHE_INS->d->already_download.find(package->name) != CACHE_INS->d->already_download.end()) return true;
    std::string url = CACHE_INS->d->repo_url + "/" + package->filename;
    std::string text = "Downloading: " + package->name + " (" + package->version + ")";
    download_file(url, CACHE_INS->d->path + "/" + package->filename.substr(package->filename.find_last_of("/")), text.c_str());

    for (auto dep : package->depends)
    {
        __download_package(dep);
    }
    CACHE_INS->d->already_download.insert(std::pair<std::string, PackageInfoPtr>(package->name, package));

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

PackageInfoPtr DEBAR::Cache::__find_package(const std::string &name)
{
    auto pos = find_package_pos(name);
    if (pos.name.empty()) return PackageInfoPtr();

    std::ifstream packageFile(CACHE_INS->d->path + "/.debar/" + pos.component + ".Packages", std::ios::in);
    if (!packageFile) {
        std::cerr << "Failed to open package file: " << pos.component << ".Packages" << std::endl;
        return PackageInfoPtr();
    }

    packageFile.seekg(pos.pos, std::ios::beg);

    PackageInfoPtr package = std::make_shared<PackageInfo>();
    std::string line;
    std::string depends_str;
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
        } else if (line.find("Description: ") == 0) {
            package->description = line.substr(13);
        } else if (line.empty()) {
            break;
        }
    }
    CACHE_INS->d->already_found.insert(std::pair<std::string, PackageInfoPtr>(package->name, package));

    while (depends_str.find(", ") != std::string::npos) {
        auto depends = depends_str.substr(0, depends_str.find(", "));
        depends_str = depends_str.substr(depends_str.find(", ") + 2);
        auto dep_name = depends.substr(0, depends.find(" "));
        if (CACHE_INS->d->already_found.find(dep_name) == CACHE_INS->d->already_found.end())
        {
            package->depends.push_back(__find_package(dep_name));
        } else {
            package->depends.push_back(CACHE_INS->d->already_found[dep_name]);
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

    std::cerr << "Package not found: " << name << std::endl;
    return InfoPos();
}

Cache::Cache()
    : d(new CachePrivate())
{
}
