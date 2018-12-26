cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(doit4me
    VERSION 0.1.1
    DESCRIPTION "The best debugger around"
    HOMEPAGE_URL "github.com"
    LANGUAGES C
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

find_package(CURL REQUIRED)
find_package(Libunwind REQUIRED)

# if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang" OR 
        # NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # message(FATAL_ERROR "Only gcc and clang supported")
# endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindRuntimeDeps.cmake)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/config.h
    @ONLY
)

add_library(doit4me SHARED
    src/doit4me.c
    src/utils.c
)
target_include_directories(doit4me
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/>
)
target_link_libraries(doit4me
    PRIVATE
        CURL::libcurl
        Libunwind::libunwind
)
target_compile_options(doit4me
    PUBLIC
        -g 
        -rdynamic
    PRIVATE
        -Wall -Wextra -pedantic
)
target_compile_features(doit4me
    PRIVATE
        c_std_99
)
target_compile_definitions(doit4me
    PRIVATE
        _GNU_SOURCE
)

include(GNUInstallDirs)
install(TARGETS doit4me EXPORT doit4me-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(EXPORT doit4me-targets
    NAMESPACE doit4me::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/doit4me/
)

add_subdirectory(driver)
