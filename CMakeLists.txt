cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
# You can change the name, but then you manually have to search for 'HMT26_FIRMWARE.elf' and change it everywhere in this repository.
set(CMAKE_PROJECT_NAME HMT26_FIRMWARE)


# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Enable CMake support for ASM and C languages
enable_language(C CXX ASM)

# Core project settings
# Cannot change the project name to something else other than the CMAKE_PROJECT_NAME, because
# then it gives errors with the STM32 generated code.
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Include GIT hash of this repo
include(${CMAKE_SOURCE_DIR}/cmake/openstmodules/CMake_check_git.cmake)
check_git_setup()

# Include all modules and dependencies
include(${CMAKE_SOURCE_DIR}/cmake/openstmodules/CMake_download_git_repo.cmake)
process_modules_from_file("${CMAKE_SOURCE_DIR}/modules.txt")


# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

file(GLOB_RECURSE SOURCES "Modules/**.c" "Modules/**.cpp")

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${SOURCES}
)

file(GLOB_RECURSE INC_DIRS LIST_DIRECTORIES true RELATIVE ${CMAKE_SOURCE_DIR} Modules/** Modules/**/Inc)

# Filter only directories that end with "/Inc"
set(FINAL_INC_DIRS "")
foreach(dir ${INC_DIRS})
    if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/${dir} AND dir MATCHES ".*/Inc$")
        list(APPEND FINAL_INC_DIRS ${dir})
    endif()
endforeach()

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${FINAL_INC_DIRS}
    Project/Inc
    Modules/trice/src
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

include(${CMAKE_SOURCE_DIR}/Project/hmt-compile-project-lib.cmake)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    HMT_PROJECT_LIB
    stm32cubemx
    git_version

    # Add user defined libraries
)
