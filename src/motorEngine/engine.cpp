#include <string>

#include <godrick/mpi/godrickMPI.h>

#include <lyra/lyra.hpp>
#include <spdlog/spdlog.h>

#include <conduit/conduit.hpp>

#include <radahn/core/blankMotor.h>
#include <radahn/core/moveMotor.h>
#include <radahn/core/rotateMotor.h>
#include <radahn/core/forceMotor.h>
#include <radahn/core/torqueMotor.h>

using namespace radahn::core;

std::vector<std::shared_ptr<Motor>> motors;

void printSimulationData(std::vector<conduit::Node>& data)
{
    spdlog::info("Engine received {} message chunks from the simulation.", data.size());
    for(size_t i = 0; i < data.size(); ++i)
    {
        // Access data from the simulation
        auto & simData = data[0]["simdata"];
        simIt_t simIt = simData["simIt"].as_uint64();
        spdlog::info("Engine Chunk {} simIt : {}", i, simIt);
        atomPositions_t* positions = simData["atomPositions"].value();
        atomIndexes_t* ids = simData["atomIDs"].value();
        auto nbAtoms = simData["atomIDs"].dtype().number_of_elements();
        spdlog::info("Engine Chunk {} received {} atoms.", i, nbAtoms);
        for(auto j = 0; j < nbAtoms; ++j)
        {
            spdlog::info("Chunk {} Atom {} Positions [{} {} {}]", i, ids[j], positions[3*j], positions[3*j+1], positions[3*j+2]);
        }
    }
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    std::string taskName;
    std::string configFile;
    std::string lmpInitialState;

    auto cli = lyra::cli()
        | lyra::opt( taskName, "name" )
            ["--name"]
            ("Name of the task corresponding to the python workflow definition.")
        | lyra::opt( configFile, "config" )
            ["--config"]
            ("Path to the config file.")
        | lyra::opt( lmpInitialState, "initlmp")
            ["--initlmp"]
            ("Path to initial script to setup the system.");

    auto result = cli.parse( { argc, argv } );
    if ( !result )
    {
        spdlog::critical("Unable to parse the command line: {}.", result.errorMessage());
        exit(1);
    }

    spdlog::info("Starting the task {}.", taskName);

    auto handler = godrick::mpi::GodrickMPI();

    spdlog::info("Loading the workflow configuration {}.", configFile);
    if(handler.initFromJSON(configFile, taskName))
        spdlog::info("Configuration file loaded successfully.");
    else
    {
        spdlog::error("Something went wrong during the workflow configuration.");
        exit(-1);
    }

    // Test setup

    // Declare test motors
    motors.emplace_back(std::make_shared<BlankMotor>("testWait", 1000));
    
    std::set<atomIndexes_t> selectionMove = {1,2,3,4,5,6,7,8,9,10,11,12};
    
    motors.emplace_back(std::make_shared<MoveMotor>("testMove", selectionMove, 
        VelocityQuantity(0.001, SimUnits::LAMMPS_REAL), VelocityQuantity(0.0, SimUnits::LAMMPS_REAL), VelocityQuantity(0.0, SimUnits::LAMMPS_REAL),
        true, false, false, 
        DistanceQuantity(1.0, SimUnits::LAMMPS_REAL), DistanceQuantity(0.0, SimUnits::LAMMPS_REAL), DistanceQuantity(0.0, SimUnits::LAMMPS_REAL)));
    
    motors.emplace_back(std::make_shared<RotateMotor>("testRotate", selectionMove, 
        DistanceQuantity(0.0, SimUnits::LAMMPS_REAL), DistanceQuantity(0.0, SimUnits::LAMMPS_REAL), DistanceQuantity(0.0, SimUnits::LAMMPS_REAL),
        1.0, 0.0, 0.0, TimeQuantity(1.0, SimUnits::LAMMPS_REAL), 180));
    
    motors.emplace_back(std::make_shared<ForceMotor>("testForce", selectionMove, 
        ForceQuantity(0.001, SimUnits::LAMMPS_REAL), ForceQuantity(0.0, SimUnits::LAMMPS_REAL), ForceQuantity(0.0, SimUnits::LAMMPS_REAL),
        true, false, false, 
        DistanceQuantity(1.0, SimUnits::LAMMPS_REAL), DistanceQuantity(0.0, SimUnits::LAMMPS_REAL), DistanceQuantity(0.0, SimUnits::LAMMPS_REAL)));
    motors.emplace_back(std::make_shared<TorqueMotor>("testTorque", selectionMove, 
        TorqueQuantity(0.001, SimUnits::LAMMPS_REAL), TorqueQuantity(0.0, SimUnits::LAMMPS_REAL), TorqueQuantity(0.0, SimUnits::LAMMPS_REAL),
        90.0));

    // Make them start immediatly
    //motors[0]->startMotor();
    //motors[1]->startMotor();
    //motors[2]->startMotor();
    //motors[3]->startMotor();
    motors[4]->startMotor();

    std::vector<conduit::Node> receivedData;
    while(handler.get("atoms", receivedData) == godrick::MessageResponse::MESSAGES)
    {
        printSimulationData(receivedData);

        // Access data from the simulation
        auto & simData = receivedData[0]["simdata"];
        simIt_t simIt = simData["simIt"].as_uint64();
        atomIndexes_t* indices = simData["atomIDs"].value();
        uint64_t nbAtoms = static_cast<uint64_t>(simData["atomIDs"].dtype().number_of_elements());
        atomPositions_t* positions = simData["atomPositions"].value();

        //spdlog::info("Received simulation data Step {}", simIt);

        // Update the motor
        //motors[0]->updateState(simIt, nbAtoms, indices, positions);
        //motors[1]->updateState(simIt, nbAtoms, indices, positions);
        //motors[2]->updateState(simIt, nbAtoms, indices, positions);
        //motors[3]->updateState(simIt, nbAtoms, indices, positions);
        motors[4]->updateState(simIt, nbAtoms, indices, positions);

        // Get commands from the motor
        conduit::Node output;
        //auto cmdArray = output.add_child("lmpcmds");
        //motors[0]->appendCommandToConduitNode(output["lmpcmds"].append());
        //motors[1]->appendCommandToConduitNode(output["lmpcmds"].append());
        //motors[2]->appendCommandToConduitNode(output["lmpcmds"].append());
        //motors[3]->appendCommandToConduitNode(output["lmpcmds"].append());
        motors[4]->appendCommandToConduitNode(output["lmpcmds"].append());

        handler.push("motorscmd", output);
    }

    spdlog::info("Engine exited loop. Closing...");
    handler.close();
    spdlog::info("Engine closed. Exiting.");

    return EXIT_SUCCESS;
}