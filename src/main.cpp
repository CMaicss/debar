#include <iostream>
#include "cmd.h"
#include "cache.h"
#include "mermaid.h"
#include "utils.h"

int main(int argc, char const *argv[]) {
    DEBAR::CMD::init_args(argc, argv);
    
    if (DEBAR::CMD::is_init())
    {
        if (!DEBAR::Cache::init_work_directory()) return -1;
    } else {
        if (!DEBAR::Cache::load_work_directory()) return -1;
    }

    if (DEBAR::CMD::is_depends_mermaid())
    {
        auto pkg = DEBAR::Cache::find_package(DEBAR::CMD::get_package_name());
        if (pkg) DEBAR::Mermaid::print_depends(pkg);
        return 0;
    }
    

    if (DEBAR::CMD::is_update())
    {
        if (!DEBAR::Cache::update_cache()) return -1;
    }

    if (DEBAR::CMD::is_get())
    {
        auto name = DEBAR::CMD::get_package_name();
        if (!DEBAR::Cache::download_package(name)) return -1;
    }

    if (DEBAR::CMD::is_info()) {
        auto name = DEBAR::CMD::get_package_name();
        auto pkg = DEBAR::Cache::find_package(name);
        if (!pkg) {
            std::cerr << "package " << name << " is not found.";
            return -1;
        }
        std::cout << "Package: " << pkg->name << std::endl;
        std::cout << "Version: " << pkg->version << std::endl;
        std::cout << "Size: " << DEBAR::Utils::format_size(pkg->size) << std::endl;
        std::cout << "Filename: " << pkg->filename << std::endl;
        std::cout << "Depends: ";
        for (auto dep : pkg->depends) {
            std::cout << dep->name << "(" << dep->version << "), ";
        }
        std::cout << std::endl;
        std::cout << "Description: " << pkg->description << std::endl;
    }

    if (DEBAR::CMD::is_search())
    {
        auto text = DEBAR::CMD::get_text();
        auto packages = DEBAR::Cache::search_package(text);
        for (auto pkg : packages) {
            std::cout << pkg->name << " (" << pkg->version << ")" << std::endl;
            std::cout << "\t" << pkg->description << "\n" << std::endl;
        }
    }

    return 0;
}
