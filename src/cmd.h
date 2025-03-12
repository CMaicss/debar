/**
 * @file cmd.h
 * @brief Command line arguments.
 * @date 2025-03-10
 * @author Maicss <maicss@126.com>
 */

#pragma once
#include <string>

namespace DEBAR {

struct CMDPrivate;

/**
 * @brief Command line arguments.
 */
class CMD
{

private:
    static CMD* m_instance;
    CMDPrivate* d;

public:
    /**
     * @brief Initialize command line arguments.
     * @param argc Number of arguments.
     * @param argv Array of arguments.
     */
    static void init_args(int argc, char const *argv[]);

    /**
     * @brief Check if command line has --init argument.
     * @return true if --init argument is present.
     */
    static bool is_init();

    /**
     * @brief Check if command line has --update argument.
     * @return true if --update argument is present.
     */
    static bool is_update();

    /**
     * @brief Check if command line has --get argument.
     * @return true if --get argument is present.
     */
    static bool is_get();

    /**
     * @brief Check if command line has --suggests argument.
     * @return true if --suggests argument is present.
     */
    static bool is_suggests();

    /**
     * @brief Get package name from --get argument.
     * @return Package name.
     */
    static std::string get_package_name();

private:
    CMD(int argc, char const *argv[]);
    ~CMD();
};

}