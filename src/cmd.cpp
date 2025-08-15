
#include "cmd.h"
#include <cxxopts.hpp>
#include <iostream>

using namespace DEBAR;

CMD* CMD::m_instance = nullptr;

struct DEBAR::CMDPrivate
{
    bool init = false;
    bool update = false;
    bool search = false;
    bool get = false;
    bool info = false;
    bool suggests = false;
    bool depends_mermaid = false;
    std::string package;
    std::string text;
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

bool CMD::is_search() {
    return m_instance->d->search;
}

bool DEBAR::CMD::is_get()
{
    return m_instance->d->get;
}

bool DEBAR::CMD::is_suggests()
{
    return m_instance->d->suggests;
}

bool DEBAR::CMD::is_depends_mermaid()
{
    return m_instance->d->depends_mermaid;
}

std::string DEBAR::CMD::get_package_name()
{
    return m_instance->d->package;
}

std::string CMD::get_text() {
    return m_instance->d->text;
}

bool CMD::is_info() {
    return m_instance->d->info;
}

CMD::CMD(int argc, char const *argv[])
    : d(new CMDPrivate())
{
    try {
        cxxopts::Options options(argv[0], "Download deb package with his depend from debian repository.");
        options.add_options()
            ("init", "Init repo data in current directory.")
            ("update", "Update repo data in current directory.")
            ("search", "Search deb package.", cxxopts::value<std::string>(), "<text>")
            ("get", "Download deb package and depends.", cxxopts::value<std::string>(), "<package_name>")
            ("info", "Show info of the deb package.", cxxopts::value<std::string>(), "<package_name>")
            ("suggests", "Think of suggests as depends, must cooperate --get used.")
            ("depends-mermaid", "Print the dependency relationship using Mermaid.", cxxopts::value<std::string>(), "<package_name>")
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

        if (result.count("suggests")) {
            d->suggests = true;
        }

        if (result.count("depends-mermaid")) {
            d->depends_mermaid = true;
            d->package = result["depends-mermaid"].as<std::string>();
        }

        if (result.count("get")) {
            d->get = true;
            d->package = result["get"].as<std::string>();
        }

        if (result.count("info")) {
            d->info = true;
            d->package = result["info"].as<std::string>();
        }

        if (result.count("search")) {
            d->search = true;
            d->text = result["search"].as<std::string>();
        }
        
    } catch (const cxxopts::exceptions::exception& e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        exit(1);
    }
}

CMD::~CMD()
{
}