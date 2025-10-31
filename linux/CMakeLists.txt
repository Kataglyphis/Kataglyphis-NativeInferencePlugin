# The Flutter tooling requires that developers have CMake 3.10 or later
# installed. You should not increase this version, as doing so will cause
# the plugin to fail to compile for some customers of the plugin.
cmake_minimum_required(VERSION 4.1.1)

# Project-level configuration.
set(PROJECT_NAME "kataglyphis_native_inference")
project(${PROJECT_NAME} LANGUAGES CXX)

# This value is used when generating builds using this plugin, so it must
# not be changed.
set(PLUGIN_NAME "kataglyphis_native_inference_plugin")

# Any new source files that you add to the plugin should be added here.
list(APPEND PLUGIN_SOURCES
  "kataglyphis_native_inference_plugin.cc"
  "my_texture.cc"
)

# Add your native library - source folder is ../native
set(RUST_FEATURES "TRUE")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../native/KataglyphisCppInference ${CMAKE_CURRENT_BINARY_DIR}/native_build)

# --- Install KataglyphisCppInference together with this plugin ----------------
# Only add install rules if the target actually exists (safety).
if(TARGET KataglyphisCppInference)
  # Install the library target (runtime, shared lib, static archive)
  install(TARGETS KataglyphisCppInference
    EXPORT KataglyphisCppInferenceTargets
    RUNTIME DESTINATION bin        # executables on some platforms
    LIBRARY DESTINATION lib        # shared libs
    ARCHIVE DESTINATION lib        # static libs / import libs
    INCLUDES DESTINATION include
  )

  # Install public headers (adjust path to actual headers if different)
  # Assume headers live in the native project include/ or Src/include.
  install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/KataglyphisCppInference/include
    DESTINATION include/KataglyphisCppInference
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    OPTIONAL
  )
else()
  message(WARNING "Target KataglyphisCppInference not found; skipping install rules.")
endif()

# Define the plugin library target. Its name must not be changed (see comment
# on PLUGIN_NAME above).
add_library(${PLUGIN_NAME} SHARED
  ${PLUGIN_SOURCES}
)

# Apply a standard set of build settings that are configured in the
# application-level CMakeLists.txt. This can be removed for plugins that want
# full control over build settings.
apply_standard_settings(${PLUGIN_NAME})

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0)


# Symbols are hidden by default to reduce the chance of accidental conflicts
# between plugins. This should not be removed; any symbols that should be
# exported should be explicitly exported with the FLUTTER_PLUGIN_EXPORT macro.
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

# Source include directories and library dependencies. Add any plugin-specific
# dependencies here.
target_include_directories(${PLUGIN_NAME} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../native/KataglyphisCppInference/Src")

target_include_directories(${PLUGIN_NAME} PRIVATE
  ${GST_INCLUDE_DIRS}
  ${GST_APP_INCLUDE_DIRS}
)

target_link_libraries(${PLUGIN_NAME} PRIVATE flutter)
target_link_libraries(${PLUGIN_NAME} PRIVATE PkgConfig::GTK)
target_link_libraries(${PLUGIN_NAME} PRIVATE KataglyphisCppInference)
target_link_libraries(${PLUGIN_NAME} PRIVATE PkgConfig::GST
  PkgConfig::GST_APP)
# List of absolute paths to libraries that should be bundled with the plugin.
# This list could contain prebuilt libraries, or libraries created by an
# external build triggered from this build file.
set(kataglyphis_native_inference_bundled_libraries
  ""
  PARENT_SCOPE
)

# === Tests ===
# These unit tests can be run from a terminal after building the example.

# Only enable test builds when building the example (which sets this variable)
# so that plugin clients aren't building the tests.
if (${include_${PROJECT_NAME}_tests})
if(${CMAKE_VERSION} VERSION_LESS "3.11.0")
message("Unit tests require CMake 3.11.0 or later")
else()
set(TEST_RUNNER "${PROJECT_NAME}_test")
enable_testing()

# Add the Google Test dependency.
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/9706f75b8f91c52a3840cf5d878a7f37ea10ef00.zip
)
# release-1.11.0.zip
# Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# Disable install commands for gtest so it doesn't end up in the bundle.
set(INSTALL_GTEST OFF CACHE BOOL "Disable installation of googletest" FORCE)

FetchContent_MakeAvailable(googletest)

# The plugin's exported API is not very useful for unit testing, so build the
# sources directly into the test binary rather than using the shared library.
add_executable(${TEST_RUNNER}
  test/kataglyphis_native_inference_plugin_test.cc
  ${PLUGIN_SOURCES}
)
apply_standard_settings(${TEST_RUNNER})
target_include_directories(${TEST_RUNNER} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${TEST_RUNNER} PRIVATE flutter)
target_link_libraries(${TEST_RUNNER} PRIVATE PkgConfig::GTK)
target_link_libraries(${TEST_RUNNER} PRIVATE gtest_main gmock)

# Enable automatic test discovery.
include(GoogleTest)
gtest_discover_tests(${TEST_RUNNER})

endif()  # CMake version check
endif()  # include_${PROJECT_NAME}_tests
