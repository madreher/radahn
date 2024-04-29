cmake_minimum_required(VERSION 3.15)


################################################################################
include(version.cmake)
################################################################################

# Set the project name to your project name, my project isn't very descriptive
project(Radahn
        VERSION
            ${RADAHN_VERSION_MAJOR}.${RADAHN_VERSION_MINOR}.${RADAHN_VERSION_PATCH}
        )

# Reference for RPATH with conan: https://docs.conan.io/1/howtos/manage_shared_libraries/rpaths.html
if( EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake )
    include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(NO_OUTPUT_DIRS TARGETS KEEP_RPATHS)
else()
    message(FATAL_ERROR "\n\nERROR: \nConan config file not found. You need to call the following from the build directory first: \n  conan profile new qt_cmake_autodetect --detect --force\n  conan install -s compiler.libcxx=libstdc++11  ${CMAKE_SOURCE_DIR} --build missing")
endif()

#########################################################
# Other targets
#########################################################
add_custom_target( ${PROJECT_NAME}_other_files
                        SOURCES
                           README.md
                           run_cmake_script.sh
                           conanfile.txt
                           cmake/CompilerWarnings.cmake
                           cmake/StandardProjectSettings.cmake
                           .gitignore
                )
#########################################################

include(GNUInstallDirs)
include(cmake/StandardProjectSettings.cmake)

#######################################################################################
# standard compiler warnings
add_library(RADAHN_project_warnings INTERFACE)
include(cmake/CompilerWarnings.cmake)
set_project_warnings(RADAHN_project_warnings)
#######################################################################################

#######################################################################################
# Link this 'library' to set the c++ standard / compile-time options requested
add_library(RADAHN_project_options INTERFACE)
#######################################################################################
target_compile_features(RADAHN_project_options INTERFACE cxx_std_20)

target_compile_definitions(RADAHN_project_options
                            INTERFACE
                                RADAHN_CMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}"
                                RADAHN_CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
                                RADAHN_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
                                RADAHN_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}"
                                RADAHN_VERSION_STRING="${RADAHN_VERSION_STRING}"
                                RADAHN_VERSION_MAJOR=${RADAHN_VERSION_MAJOR}
                                RADAHN_VERSION_MINOR=${RADAHN_VERSION_MINOR}
                                RADAHN_VERSION_PATCH=${RADAHN_VERSION_PATCH}
                                RADAHN_VERSION_TWEAK=${RADAHN_VERSION_BUILD}
                                RADAHN_CMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
                                RADAHN_GIT_COMMIT_HASH="${GIT_COMMIT_HASH}")

#######################################################################################
# Link this interface to include a standard set of libs that would be needed
# for developing Godrick
#######################################################################################
add_library(RADAHN_project_libraries INTERFACE )
#######################################################################################


target_include_directories( RADAHN_project_libraries
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>  
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()


find_package(MPI REQUIRED)
find_package(Conduit REQUIRED)
find_package(Godrick REQUIRED)

if(WIN32)
target_link_libraries( RADAHN_project_libraries
    INTERFACE
        CONAN_PKG::spdlog
        CONAN_PKG::lyra
        CONAN_PKG::nlohmann_json
        conduit::conduit
)
elseif(MACOSX)
target_link_libraries( RADAHN_project_libraries
    INTERFACE
        CONAN_PKG::spdlog
        CONAN_PKG::lyra
        CONAN_PKG::nlohmann_json
        conduit::conduit
)
else()
target_link_libraries( RADAHN_project_libraries
    INTERFACE
        CONAN_PKG::spdlog
        CONAN_PKG::lyra
        CONAN_PKG::nlohmann_json
        stdc++fs
        conduit::conduit
)
endif()

set(RADAHN_MODULE_DIR       ${CMAKE_INSTALL_PREFIX}/bin)

add_subdirectory(src)

install(DIRECTORY 
            ${CMAKE_SOURCE_DIR}/workflow 
        DESTINATION  
            ${CMAKE_INSTALL_PREFIX})

