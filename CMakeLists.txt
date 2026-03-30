cmake_minimum_required(VERSION 3.16)
project(CFD_GUI LANGUAGES CXX)

# Allow Objective-C++ files in this target
enable_language(OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Homebrew Qt path (Apple Silicon). Adjust if using Intel mac.
set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")

find_package(Qt6 COMPONENTS Widgets REQUIRED)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/Grid.cpp
    src/Field.cpp
    src/Solver.cpp
    src/CpuSolverBackend.cpp
    src/GpuMetalSolverBackend.cpp
    src/GpuMetalRunner.mm
)

set(HEADERS
    src/MainWindow.h
    src/Grid.h
    src/Field.h
    src/Solver.h
    src/ComputeConfig.h
    src/ISolverBackend.h
    src/CpuSolverBackend.h
    src/GpuMetalSolverBackend.h
    src/GpuMetalRunner.h
)

# Metal shaders
set(METAL_SOURCES
    src/GpuKernels.metal
)

qt_wrap_cpp(MOC_SOURCES ${HEADERS})

add_executable(CFD_GUI
    ${SOURCES}
    ${METAL_SOURCES}
    ${MOC_SOURCES}
)

# Copy Metal shader source into the build directory so we can load it at runtime
foreach(metal_file ${METAL_SOURCES})
    get_filename_component(metal_name ${metal_file} NAME)
    add_custom_command(TARGET CFD_GUI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/${metal_file}
                ${CMAKE_CURRENT_BINARY_DIR}/${metal_name}
    )
endforeach()

target_include_directories(CFD_GUI PRIVATE src)

target_link_libraries(CFD_GUI PRIVATE
    Qt6::Widgets
    "-framework Metal"
    "-framework Foundation"
)
