# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Set CMake policies for modern behavior (e.g., OpenGL find behavior)
cmake_policy(SET CMP0072 NEW)

# Define your project name and language
project(YourRaylibGame CXX)

# Set the default build type (Debug for development, Release for final builds)
set(CMAKE_BUILD_TYPE Debug)

# Set C++ standard (important for modern C++ features and Box2D)
set(CMAKE_CXX_STANDARD 11) # Or 14, 17, 20 depending on your preference and compiler support
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure output directories for executables and libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Add Raylib as a subdirectory (submodule)
# IMPORTANT: Point to the root of the raylib-src folder, not its internal src/
add_subdirectory(_deps/raylib-src)

# Add Box2D as a subdirectory (submodule)
# Box2D's CMakeLists.txt is at the root of its repo
add_subdirectory(_deps/box2d-src)

# Define your source files (assuming they are in the 'src' folder)
set(GAME_SOURCES
    src/main.cpp
    src/structures.cpp
    # Add any other .cpp files here
)

# Create your executable
add_executable(${PROJECT_NAME} ${GAME_SOURCES})

# --- Now that the executable target is defined, we can configure it ---
set(BOX2D_LIB_OUTPUT_DIR "${CMAKE_BINARY_DIR}/_deps/box2d-build/bin")

# Add include directories for your own project headers (like structures.h)
# Assuming structures.h is in 'include/' as per your file structure, or adjust to 'src/'
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include # Your own project headers (e.g., structures.h)
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${BOX2D_LIB_OUTPUT_DIR}
    # Add any other specific library search paths if needed (like for Raylib's libs)
    # "${CMAKE_BINARY_DIR}/lib" # This is where libraylib.a ends up
)

# Link your executable to Raylib and Box2D libraries
# 'raylib' and 'Box2D' are the target names created by their respective CMakeLists.txt files
target_link_libraries(${PROJECT_NAME} PRIVATE raylib box2d::box2d) # Use PRIVATE linking

# Set compile definitions specific to raylib for desktop platform
# These are often required for raylib to build correctly
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PLATFORM_DESKTOP
    GRAPHICS_API_OPENGL_33 # Or GRAPHICS_API_OPENGL_21 if you have issues, but 3.3 is common
)

# Ensure our target includes raylib's headers and Box2D's headers
# Raylib's public headers are typically in `_deps/raylib-src/src`
# Box2D's public headers are typically in `_deps/box2d-src/box2d`
target_include_directories(${PROJECT_NAME} PRIVATE
    _deps/raylib-src/src # Raylib's public headers
    _deps/box2d-src/box2d # Box2D's public headers (where box2d.h is)
)

# If you're on Linux, ensure necessary system libraries for GLFW (raylib dependency) are linked
if (UNIX)
    # Find system libraries required by GLFW (Raylib's windowing backend)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(X11_REQUIRED REQUIRED x11 xrandr xcursor xi xinerama xkbcommon)

    # Ensure OpenGL is found (might be provided by mesa-libGL-devel, libgl-dev, etc.)
    find_package(OpenGL REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${X11_REQUIRED_LIBRARIES}
        ${OPENGL_LIBRARIES}
    )
endif()