# Allocators - Memory allocator library
cmake_minimum_required(VERSION 3.24)

project(Allocators VERSION 1.0.0)

# Options
option(QALLOCATORS_BUILD_TESTS "Build Allocators unit tests" ON)

# Create an interface library for header-only libraries
add_library(Allocators INTERFACE)

# Include directories
target_include_directories(Allocators INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Inc>
    $<INSTALL_INTERFACE:include/Allocators>
)

# Add tests
if (QALLOCATORS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(AllocatorsTest)
endif()

# Installation of include files
install(DIRECTORY Inc/
    DESTINATION include/Allocators
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl"
)

# Export targets
install(TARGETS Allocators
    EXPORT AllocatorsTargets
    INCLUDES DESTINATION include
)

install(EXPORT AllocatorsTargets
    FILE AllocatorsTargets.cmake
    NAMESPACE Allocators::
    DESTINATION lib/cmake/Allocators
)