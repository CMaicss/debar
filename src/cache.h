/**
 * @file cache.h
 * @brief Cache system.
 * @date 2025-03-10
 * @author Maicss <maicss@126.com>
 */

#pragma once
#include <list>
#include <string>
#include <memory>

#include "structs.h"

namespace DEBAR {

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

    /**
     * @brief Find package by name.
     * @param name The name of package.
     * @return The package info.
     */
    static PackageInfoPtr find_package(const std::string& name);

    /**
     * @brief Search package by a text.
     * @param text A text for search.
     * @return The text in package name.
     */
    static std::list<PackageInfoPtr> search_package(const std::string& text);

private:

    static bool __download_package(PackageInfoPtr package);

    /**
     * @brief Unzip .gz file.
     * @param path The path of .gz file.
     * @return true if unzip file successfully.
     */
    static bool unzip_gz_file(const std::string& path);

    static PackageInfoPtr get_package_info(const InfoPos& name);

    /**
     * @brief Find package position by name.
     * @param name The name of package.
     * @return The package position.
     */
    static InfoPos find_package_pos(const std::string& name);

    static std::list<InfoPos> find_packages_pos(const std::string& name);
private:
    Cache();
};

}