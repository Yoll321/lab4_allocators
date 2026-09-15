cmake_minimum_required(VERSION 3.23)

set(PATCH_VERSION "1" CACHE STRING "Patch version provided by GitHub")
project(allocators
    VERSION 1.0.${PATCH_VERSION}
    LANGUAGES CXX)

add_executable(allocators)
target_sources(allocators
    PRIVATE
        main.cpp
)

add_library(my_list INTERFACE)
target_include_directories(my_list
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(allocators)
target_link_libraries(allocators PRIVATE my_list)

set(CPACK_PACKAGE_NAME "allocators")
set(CPACK_PACKAGE_VENDOR "nom4")
set(CPACK_PACKAGE_CONTACT "nom4@tpu.ru")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Alocators application")

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(UNIX)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "nom4@tpu.ru")
endif()

include(CPack)