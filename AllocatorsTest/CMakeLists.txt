# AllocatorsTest - Test application for Allocators library
cmake_minimum_required(VERSION 3.24)

# Define the test executable
add_executable(AllocatorsTest
    AllocatorsTest.cpp
    pch.cpp
    pch.h
)

# Set target properties
set_target_properties(AllocatorsTest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "AllocatorsTest"
)

# Include directories
target_include_directories(AllocatorsTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../Inc
)

# Use FetchContent to get Google Test
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Preprocessor definitions
target_compile_definitions(AllocatorsTest PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    _CONSOLE
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(AllocatorsTest PRIVATE
        WIN32
        _WINDOWS
    )
    
    # Link with Windows libraries
    target_link_libraries(AllocatorsTest PRIVATE
        kernel32
        user32
        gdi32
        winspool
        comdlg32
        advapi32
        shell32
        ole32
        oleaut32
        uuid
        odbc32
        odbccp32
    )
    
    # Set subsystem to console
    set_target_properties(AllocatorsTest PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

# Link with Allocators and Google Test
target_link_libraries(AllocatorsTest PRIVATE
    Allocators
    gtest_main
)

# Use precompiled headers
target_precompile_headers(AllocatorsTest PRIVATE pch.h)

# Register with CTest
enable_testing()
add_test(NAME AllocatorsTests COMMAND AllocatorsTest)

# Installation
install(TARGETS AllocatorsTest
    RUNTIME DESTINATION bin
)