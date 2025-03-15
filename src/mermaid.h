#pragma once

#include <set>
#include "structs.h"

namespace DEBAR {

class Mermaid {

public:
    static void print_depends(PackageInfoPtr package);

private:
    static void print_depends_com(DEBAR::PackageInfoPtr package);
    static std::set<std::pair<PackageInfoPtr, PackageInfoPtr>> already_printed;
};
};