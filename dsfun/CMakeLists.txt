# CMakeList.txt : CMake project for dsfun, include source and define
# project specific logic here.
#

# Add source to this project's executable.
cmake_minimum_required(VERSION 3.16)

project(dsfun VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PATH "D:\\Software\\Qt\\6.5.1\\msvc2019_64\\bin;%PATH%")

# Without this line, cmake does not find the package
set(CMAKE_PREFIX_PATH "D:\\Software\\Qt\\6.5.1\\msvc2019_64")

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

set ( SOURCES
dsfun.cpp
mainwindow.cpp
gamereroute.cpp
timedRead.cpp
)

QT_WRAP_UI( UI_HEADERS ${UIS} )

add_executable(dsfun ${SOURCES})

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
find_package(ZLIB REQUIRED)
find_package(ixwebsocket CONFIG REQUIRED HINTS "D:/Software/vcpkg/installed/x64-windows/share/ixwebsocket")

target_link_libraries(dsfun PRIVATE Qt6::Widgets ixwebsocket::ixwebsocket)

set_target_properties(dsfun PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

 # ...
 # target_link_libraries(${PROJECT_NAME} ... ${IXWEBSOCKET_LIBRARY}) # Cmake will automatically fail the generation if the lib was not found, i.e is set to NOTFOUND

# TODO: Add tests and install targets if needed.
