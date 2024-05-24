add_executable(engine engine.cpp)

target_link_libraries(engine 
    godrick::godrick
    RadahnLib
    RADAHN_project_options
    RADAHN_project_libraries
    RADAHN_project_warnings)

install(
    TARGETS 
        engine
    DESTINATION
        ${RADAHN_MODULE_DIR})