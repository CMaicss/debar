file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

add_executable(debar ${SOURCES} ${HEADERS})

target_include_directories(debar PRIVATE
    ${CMAKE_SOURCE_DIR}/third-party/cxxopts
)

target_link_libraries(debar PUBLIC
    yaml-cpp::yaml-cpp
    curl
    ZLIB::ZLIB
)

install(TARGETS debar DESTINATION bin)