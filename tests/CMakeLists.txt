add_executable(testConversion test_conversion.cpp)

target_link_libraries(testConversion 
    godrick::godrick
    RadahnLib
    RADAHN_project_options
    RADAHN_project_libraries
    RADAHN_project_warnings)

install(
    TARGETS 
    testConversion
    PERMISSIONS
        OWNER_EXECUTE OWNER_WRITE OWNER_READ WORLD_EXECUTE WORLD_WRITE WORLD_READ
    DESTINATION
        ${RADAHN_MODULE_DIR})