cmake_minimum_required(VERSION 3.16)

project(Testing_Task VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
elseif (MSVC)
    add_compile_options(/W4)
endif()

add_library(server_core
        EpollServer.cpp EpollServer.h
        ThreadPool.cpp ThreadPool.h
        Client.cpp Client.h
        Utils.cpp Utils.h
)

# Создаем исполняемый файл
add_executable(Testing_Task main.cpp)
target_link_libraries(Testing_Task PRIVATE server_core)

install(TARGETS Testing_Task
        RUNTIME DESTINATION /usr/bin
)

install(FILES packaging/Testing_Task.service
        DESTINATION /lib/systemd/system
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

# ИЗМЕНЕНО: пути директорий с Testing_Task на testing-task
install(DIRECTORY DESTINATION /var/log/testing-task)
install(DIRECTORY DESTINATION /var/lib/testing-task)

set(CPACK_PACKAGE_NAME "testing-task")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION "Epoll Server for Testing Tasks")
set(CPACK_PACKAGE_MAINTAINER "Nanagi_Sleep <nanagi.sleep@gmail.com>")
set(CPACK_PACKAGE_VENDOR "Nanagi_Sleep")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/anylaim//Testing_Task")

set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.28), libstdc++6 (>= 9)")
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "logrotate")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "curl")

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_SOURCE_DIR}/packaging/postinst;${CMAKE_SOURCE_DIR}/packaging/postrm")

set(CPACK_GENERATOR "DEB")

set(CPACK_PACKAGE_CONTACT "Nanagi Sleep <nanagi.sleep@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Nanagi_Sleep <nanagi.sleep@gmail.com>")

include(CPack)

add_custom_target(deb-package
        COMMAND cpack -G DEB
        DEPENDS Testing_Task
        COMMENT "Building .deb package"
)