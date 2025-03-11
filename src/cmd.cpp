
#include "cmd.h"
#include <cxxopts.hpp>
#include <iostream>

using namespace DEBAR;

CMD* CMD::m_instance = nullptr;

struct DEBAR::CMDPrivate
{
    bool init = false;
    bool update = false;
    bool get = false;
    std::string package;
};


void DEBAR::CMD::init_args(int argc, char const *argv[])
{
    if (m_instance == nullptr)
    {
        m_instance = new CMD(argc, argv);
    }
}

bool DEBAR::CMD::is_init()
{
    return m_instance->d->init;
}

bool DEBAR::CMD::is_update()
{
    return m_instance->d->update;
}

bool DEBAR::CMD::is_get()
{
    return m_instance->d->get;
}

std::string DEBAR::CMD::get_package_name()
{
    return m_instance->d->package;
}

CMD::CMD(int argc, char const *argv[])
    : d(new CMDPrivate())
{
    try {
        cxxopts::Options options(argv[0], "Download deb package with his depend from debian repository.");
        options.add_options()
            ("init", "Init repo data in current directory.")
            ("update", "Update repo data in current directory.")
            ("get", "Download deb package and depends.", cxxopts::value<std::string>(), "<package_name>")
            ("help", "Print help");

        auto result = options.parse(argc, argv);

        if (result.count("help") || argc == 1) {
            std::cout << options.help() << std::endl;
            exit(0);
        }

        if (result.count("init")) {
            d->init = true;
        }

        if (result.count("update")) {
            d->update = true;
        }

        if (result.count("get")) {
            d->get = true;
            d->package = result["get"].as<std::string>();
        }
    } catch (const cxxopts::exceptions::exception& e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        exit(1);
    }
}

CMD::~CMD()
{
}