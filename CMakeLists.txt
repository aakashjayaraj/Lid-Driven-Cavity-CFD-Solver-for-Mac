cmake_minimum_required(VERSION 3.16)
project(CFD_GUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake where Homebrew's Qt is (Apple Silicon path; adjust if needed)
set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")

find_package(Qt6 COMPONENTS Widgets REQUIRED)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/Grid.cpp
    src/Field.cpp
    src/Solver.cpp
)

set(HEADERS
    src/MainWindow.h
    src/Grid.h
    src/Field.h
    src/Solver.h
)

qt_wrap_cpp(MOC_SOURCES ${HEADERS})

add_executable(CFD_GUI
    ${SOURCES}
    ${MOC_SOURCES}
)

# These must use CFD_GUI, the target created above
target_include_directories(CFD_GUI PRIVATE src)
target_link_libraries(CFD_GUI PRIVATE Qt6::Widgets)
