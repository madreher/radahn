find_package(LAMMPS REQUIRED)

add_executable(lammpsDriver lammpsDriver.cpp)

target_link_libraries(lammpsDriver 
    godrick::godrick
    #godrick::godrick_mpi
    LAMMPS::lammps
    RADAHN_project_options
    RADAHN_project_libraries
    RADAHN_project_warnings
    RadahnLib)

install(
    TARGETS 
        lammpsDriver
    PERMISSIONS
        OWNER_EXECUTE OWNER_WRITE OWNER_READ WORLD_EXECUTE WORLD_WRITE WORLD_READ
    DESTINATION
        ${RADAHN_MODULE_DIR})