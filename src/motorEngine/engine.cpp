#include <string>

#include <godrick/mpi/godrickMPI.h>

#include <lyra/lyra.hpp>
#include <spdlog/spdlog.h>

#include <conduit/conduit.hpp>

#include <radahn/motor/motorEngine.h>
#include <radahn/lmp/lammpsCommandsUtils.h>

using namespace radahn::core;
using namespace radahn::motor;

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

simIt_t mergeInputData(std::vector<conduit::Node>& receivedData, std::vector<atomIndexes_t>& outIndices, std::vector<atomPositions_t>& outPositions)
{
    // Get the total nb of atoms
    conduit::index_t totalNbAtoms = 0;
    simIt_t simIt = 0;

    for(size_t i = 0; i < receivedData.size(); ++i)
    {
        auto & simData = receivedData[i]["simdata"];
        totalNbAtoms += simData["atomIDs"].dtype().number_of_elements();
    }

    // Prepare the vectors
    outIndices.reserve(static_cast<size_t>(totalNbAtoms));
    outPositions.reserve(static_cast<size_t>(totalNbAtoms*3));

    // Copy the data to the vectors
    for(size_t i = 0; i < receivedData.size(); ++i)
    {
        auto & simData = receivedData[i]["simdata"];
        simIt = simData["simIt"].as_uint64();
        atomIndexes_t* indices = simData["atomIDs"].value();
        uint64_t nbAtoms = static_cast<uint64_t>(simData["atomIDs"].dtype().number_of_elements());
        atomPositions_t* positions = simData["atomPositions"].value();

        outIndices.insert(outIndices.end(), indices, indices + nbAtoms);
        outPositions.insert(outPositions.end(), positions, positions + 3*nbAtoms);
    }

    return simIt;
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    std::string taskName;
    std::string configFile;
    std::string lmpInitialState;
    std::string motorConfig;
    bool useTestMotors = false;
    bool forceMaxSteps = false;

    auto cli = lyra::cli()
        | lyra::opt( taskName, "name" )
            ["--name"]
            ("Name of the task corresponding to the python workflow definition.")
        | lyra::opt( configFile, "config" )
            ["--config"]
            ("Path to the config file.")
        | lyra::opt( lmpInitialState, "initlmp")
            ["--initlmp"]
            ("Path to initial script to setup the system.")
        | lyra::opt( motorConfig, "motors")
            ["--motors"]
            ("Path to the definition of the motors to use.")
        | lyra::opt( useTestMotors)
            ["--testmotors"]
            ("Use the test motor setup.")
        | lyra::opt( forceMaxSteps)
            ["--forcemaxsteps"]
            ("Continue the simulation until the maximum number of steps given, even if all the motors have completed.");

    auto result = cli.parse( { argc, argv } );
    if ( !result )
    {
        spdlog::critical("Unable to parse the command line: {}.", result.errorMessage());
        exit(1);
    }

    if(useTestMotors && !motorConfig.empty())
    {
        spdlog::critical("You cannot use the test motors and the motor config at the same time.");
        exit(1);
    }

    if(!useTestMotors && motorConfig.empty() && !forceMaxSteps)
    {
        spdlog::critical("Motor configuration not provided, not using the test motors and not forcing the maximum number of steps. Nothing to do.");
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

    // Create the engine and add a test set of motors
    auto engine = radahn::motor::MotorEngine();

    if(useTestMotors)
    {
        spdlog::info("Loading the test motor setup.");
        engine.loadTestMotorSetup();
    }
    else if(!motorConfig.empty())
    {
        spdlog::info("Loading the motor setup {}.", motorConfig);
        engine.loadFromJSON(motorConfig);    
    }

    std::vector<conduit::Node> receivedData;
    bool unitSet = false;


    while(handler.get("atoms", receivedData) == godrick::MessageResponse::MESSAGES)
    {
        // Debug
        //printSimulationData(receivedData);

        // Merge all the data into individual arrays instead of partial arrays
        // This is necessary when Lammps is running on multiple MPI processes, we receive as many 
        // messages as Lammps MPI processes
        // This cause a double memory footprint but avoid having to deal with partial arrays everywhere
        std::vector<atomIndexes_t> fullIndices;
        std::vector<atomPositions_t> fullPositions;
        auto receivedIt = mergeInputData(receivedData, fullIndices, fullPositions);

        // Switch the motors settings to the simulation settings
        if(!unitSet)
        {

            auto unitSim = radahn::core::SimUnits(receivedData[0]["simdata"]["units"].as_uint8());
            engine.convertMotorsTo(unitSim);
            unitSet = true;
        }


        spdlog::info("Received simulation data Step {}", receivedIt);
        //for(uint64_t i = 0; i < fullIndices.size(); ++i)
        //    spdlog::info("{} {} {}", fullPositions[3*i], fullPositions[3*i+1], fullPositions[3*i+2]);


        // Check in which phase we are
        auto phase = receivedData[0]["simdata"]["phase"].as_string();
        //std::string phase{"NVE"};
        if(phase.compare("NVT") == 0)
        {
            // During the NVT phase, we don't execute the motors yet. 
            // We only update the state of the engine, but not the motors
            engine.updateEngineState(receivedIt, fullIndices, fullPositions);

            // Sending an empty message to keep the loop going.
            conduit::Node cmdOutput;
            handler.push("motorscmd", cmdOutput);

            // This is kinda dangerous because the push operation may modify the Node
            // In this case it's fine because it's the instruction before the next iteration
            engine.addGlobalKVS(receivedData[0]["thermos"]);    // All the nodes have the same thermo info, no need to check all the inputs
            engine.commitKVSFrame();
            conduit::Node& temporalData = engine.getCurrentKVS();
            
            //temporalData["global"] = receivedData[0]["thermos"];
            handler.push("kvs", temporalData);


            // Send the atom positions to the outside 
            conduit::Node atoms;
            atoms["positions"] = engine.getCurrentPositions();
            atoms["simIt"] = engine.getCurrentIt();
            handler.push("atoms", atoms);
        }
        else if (phase.compare("NVE") == 0)
        {
            engine.updateMotorsState(receivedIt, fullIndices, fullPositions);

            if(engine.isCompleted())
            {
                if(forceMaxSteps)
                {
                    // Sending a blank command in this case to keep the loop going. The Lammps
                    // component will send a terminate message when the maximum number of steps has been reached.
                    conduit::Node output;
                    radahn::lmp::LammpsCommandsUtils::registerWaitCommandToConduit(output["lmpcmds"].append(), "motorEngine");
                    handler.push("motorscmd", output);
                }
                else
                {
                    spdlog::info("Motor engine has completed. Exiting the main loop.");
                    engine.getCurrentKVS().print();
                    break;
                }
            }
            else 
            {
                // Get commands from the motor
                conduit::Node output;
                engine.getCommandsFromMotors(output["lmpcmds"].append());

                handler.push("motorscmd", output);
            }

            // This is kinda dangerous because the push operation may modify the Node
            // In this case it's fine because it's the instruction before the next iteration
            engine.addGlobalKVS(receivedData[0]["thermos"]);    // All the nodes have the same thermo info, no need to check all the inputs
            engine.commitKVSFrame();
            conduit::Node& temporalData = engine.getCurrentKVS();
            
            //temporalData["global"] = receivedData[0]["thermos"];
            handler.push("kvs", temporalData);


            // Send the atom positions to the outside 
            conduit::Node atoms;
            atoms["positions"] = engine.getCurrentPositions();
            atoms["simIt"] = engine.getCurrentIt();
            handler.push("atoms", atoms);

            // Iterations is finished, processing the motor state and prepare the motor lists for the next iteration
            engine.updateMotorLists();
        }
        else 
        {
            spdlog::critical("Received a simulation data from an unknown phase {}. Abording.", phase);
            break;
        }

        receivedData.clear();

        //engine.getCurrentKVS().print();

        
    }

    //spdlog::info("Cleaning the motor engine...");
    //engine.clearMotors();
    //spdlog::info("Motor engine cleaned.");
    engine.saveKVSToCSV();

    spdlog::info("Engine exited loop. Closing...");
    handler.close();
    spdlog::info("Engine closed. Exiting.");

    return EXIT_SUCCESS;
}