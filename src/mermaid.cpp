#include "mermaid.h"
#include <iostream>

using namespace std;

std::set<std::pair<DEBAR::PackageInfoPtr, DEBAR::PackageInfoPtr>> DEBAR::Mermaid::already_printed;

void DEBAR::Mermaid::print_depends_com(DEBAR::PackageInfoPtr package) {
    for (auto dep : package->depends)
    {
        if (already_printed.find(std::pair<PackageInfoPtr,PackageInfoPtr>(package, dep)) != already_printed.end())
        {
            continue;
        }
        
        already_printed.insert(std::pair<PackageInfoPtr,PackageInfoPtr>(package, dep));
        cout << "  " << package->name << " --> " << dep->name << endl;
        print_depends_com(dep);
    }
    
}

void DEBAR::Mermaid::print_depends(PackageInfoPtr package)
{
    cout << "flowchart TD" << endl;
    cout << "  " << package->name << "{{" << package->name << "}}" << endl;
    print_depends_com(package);
}