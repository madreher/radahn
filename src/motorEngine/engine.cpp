#include <string>

#include <godrick/mpi/godrickMPI.h>

#include <lyra/lyra.hpp>
#include <spdlog/spdlog.h>

#include <conduit/conduit.hpp>

#include <radahn/core/blankMotor.h>

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
    motors.emplace_back(std::make_shared<BlankMotor>("testWait", 1000));

    std::vector<conduit::Node> receivedData;
    while(handler.get("atoms", receivedData) == godrick::MessageResponse::MESSAGES)
    {
        printSimulationData(receivedData);

        // Access data from the simulation
        auto & simData = receivedData[0]["simdata"];
        simIt_t simIt = simData["simIt"].as_uint64();

        //spdlog::info("Received simulation data Step {}", simIt);

        // Update the motor
        motors[0]->updateState(simIt);

        conduit::Node output;
        handler.push("motorscmd", output);
    }

    spdlog::info("Engine exited loop. Closing...");
    handler.close();
    spdlog::info("Engine closed. Exiting.");

    return EXIT_SUCCESS;
}