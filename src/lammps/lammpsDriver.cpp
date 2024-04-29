#include <string>

#include <godrick/mpi/godrickMPI.h>
#include <conduit/conduit.hpp>

#include <radahn/core/types.h>

#include <lyra/lyra.hpp>
#include <spdlog/spdlog.h>

// lammps includes
#include "lammps.h"
#include "input.h"
#include "atom.h"
#include "library.h"

using namespace LAMMPS_NS;
using namespace radahn::core;

bool executeScript(LAMMPS* lps, const std::string& scriptPath)
{
    std::ifstream fdesc(scriptPath);
    std::string line;

    // Only execute the script if we could open it
    if (fdesc.is_open())
    {
        // Execute the script
        lps->input->file(scriptPath.c_str());
    }
    else
        return false;

    fdesc.close();
    return true;
}

bool executeCommand(LAMMPS* lps, const std::string& cmd)
{
    lps->input->one(cmd);
    return true;
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

    // Setting up Lammps
    LAMMPS* lps = new LAMMPS(0, NULL, handler.getTaskCommunicator());

    executeScript(lps, lmpInitialState);

    std::vector<conduit::Node> receivedData;
    if(handler.get("in", receivedData))
    {
        spdlog::info("Received a data.");
        executeCommand(lps, "run 100");

        double simItD = lammps_get_thermo(lps, "step");
        simIt_t simIt = static_cast<simIt_t>(simItD);

        conduit::Node rootMsg;
        conduit::Node& simData = rootMsg.add_child("simdata");
        simData["simIt"] = simIt;

        handler.push("out", rootMsg, true);
    }

    handler.close();

    return EXIT_SUCCESS;
}