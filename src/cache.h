/**
 * @file cache.h
 * @brief Cache system.
 * @date 2025-03-10
 * @author Maicss <maicss@126.com>
 */

#pragma once
#include <string>
#include <vector>
#include <memory>

namespace DEBAR {

struct PackageInfo;
typedef std::shared_ptr<PackageInfo> PackageInfoPtr;
struct PackageInfo {
    std::string name;
    std::string version;
    std::string description;
    std::vector<PackageInfoPtr> depends;
    std::string filename;
    size_t size;
    std::string md5;
};

struct InfoPos
{
    std::string name;
    std::string component;
    std::streamoff pos;
};


struct CachePrivate;
class Cache
{
private:
    CachePrivate* d;
    static Cache* m_instance;
public:
    static Cache* instance();

    /**
     * @brief Initialize work directory.
     * @return true if work directory initialized successfully.
     */
    static bool init_work_directory();

    /**
     * @brief Load work directory.
     * @return true if work directory loaded successfully.
     */
    static bool load_work_directory();

    /**
     * @brief Update cache, download Packages.gz.
     * @return true if cache updated successfully.
     */
    static bool update_cache();

    /**
     * @brief Download package by name.
     * @param name The name of package.
     * @return true if download package successfully.
     */
    static bool download_package(const std::string& name);
private:

    /**
     * @brief Download file from url.
     * @param url The url of file.
     * @param path The path to save file.
     * @return true if download file successfully.
     */
    static bool download_file(const std::string& url, const std::string& path);

    static bool __download_package(PackageInfoPtr package);

    /**
     * @brief Unzip .gz file.
     * @param path The path of .gz file.
     * @return true if unzip file successfully.
     */
    static bool unzip_gz_file(const std::string& path);

    /**
     * @brief Find package by name.
     * @param name The name of package.
     * @return The package info.
     */
    static PackageInfoPtr find_package(const std::string& name);

    static PackageInfoPtr __find_package(const std::string& name);

    /**
     * @brief Find package position by name.
     * @param name The name of package.
     * @return The package position.
     */
    static InfoPos find_package_pos(const std::string& name);
private:
    Cache();
};

}