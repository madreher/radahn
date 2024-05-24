# Build the base library
#=======================================
set(library_MODULE RadahnLib   )
#=======================================

file(GLOB_RECURSE files "*.cpp" )

add_library( ${library_MODULE} SHARED ${files} )
target_include_directories(${library_MODULE} 
                PUBLIC 
                    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)

target_link_libraries( ${library_MODULE}
                PUBLIC
                    conduit::conduit
                    RADAHN_project_libraries
                    RADAHN_project_options
                    RADAHN_project_warnings
                     )

include(GNUInstallDirs)
install(TARGETS ${library_MODULE} EXPORT godrick DESTINATION ${CMAKE_INSTALL_LIBDIR})