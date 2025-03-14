#include <iostream>
#include "cmd.h"
#include "cache.h"
#include "mermaid.h"

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
    
    
    return 0;
}