#include <string>
#include <unordered_map>
#include <variant>
#include <ranges>

#include <godrick/mpi/godrickMPI.h>
#include <conduit/conduit.hpp>

#include <radahn/core/types.h>
#include <radahn/lmp/lammpsCommandsUtils.h>

#include <lyra/lyra.hpp>
#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

// lammps includes
#include "lammps.h"
#include "input.h"
#include "atom.h"
#include "library.h"

using namespace LAMMPS_NS;
using namespace radahn::core;

std::vector<uint32_t> getAnchorsIds(const std::string& jsonConfigPath)
{
    json document = json::parse(std::ifstream(jsonConfigPath));

    if(!document.contains("header"))
    {
        spdlog::error("Could not find header in JSON file {}.", jsonConfigPath);
        return {};
    }

    uint32_t version = document["header"].value("version", 0u);
    if(version != 0)
    {
        spdlog::error("Unsupported version {} in JSON file {}.", version, jsonConfigPath);
        return {};
    }
    if(!document.contains("anchors"))
    {
        spdlog::info("Could not find anchors in JSON file {}.", jsonConfigPath);
        return {};
    }

    std::vector<uint32_t> ids;
    for(auto & anchor : document["anchors"])
    {
        if(!anchor.contains("selection"))
        {
            spdlog::info("Could not find selection in JSON file {}.", jsonConfigPath);
            return {};
        }
        for(auto & id: anchor["selection"])
        {
            ids.push_back(id);
        }
    }
    return ids;
}

bool executeScript(LAMMPS* lps, const std::string& scriptPath)
{
    std::ifstream fdesc(scriptPath);

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

std::vector<std::string> splitString(std::string const & line, char sep)
{
    auto r = line
        | std::ranges::views::split(sep)
        | std::ranges::views::transform([](auto &&rng) {
                return std::string_view(&*rng.begin(), std::size_t(std::ranges::distance(rng)));
                })
        | std::ranges::views::filter( [](auto && s) { return !s.empty(); });
        // to is C++23, have to do the conversion manually
        //| std::ranges::to<std::vector<std::string>>();
        return std::vector<std::string>(r.begin(), r.end());
}

SimUnits getUnitStyle(const std::string& scriptPath)
{
    std::ifstream fdesc(scriptPath);
    std::string line;
    if (fdesc.is_open())
    {
        while(std::getline(fdesc, line))
        {
            // Look if the command line has unit in it 
            if(line.starts_with("units"))
            {
                auto words = splitString(line, ' ');
                if(words.size() != 2)
                {
                    spdlog::error("Found the units command but unable to parse it properly.");
                    throw std::runtime_error("Found the units command but unable to parse it properly.");
                }
                return radahn::core::from_string(words[1]);
            }
        }
    }

    throw std::runtime_error("Unable to find the units command.");
}

bool executeCommand(LAMMPS* lps, const std::string& cmd)
{
    lps->input->one(cmd);
    return true;
}

void extractAtomInformation(
    LAMMPS* lps,
    std::vector<atomIndexes_t>& ids,
    std::vector<atomPositions_t>& pos,
    std::vector<atomForces_t>& forces,
    std::vector<atomVelocities_t>& vel,
    std::unordered_map<std::string, std::variant<double, int32_t> >& thermo
    )
{
    // Allocating the buffers according to the local proc
    uint64_t localSize = static_cast<uint64_t>(lps->atom->nlocal);
    pos.resize(localSize * 3);
    forces.resize(localSize * 3);
    vel.resize(localSize * 3);
    ids.resize(localSize);

    int* id = static_cast<int*>(lps->atom->extract("id"));
    double** x = static_cast<double**>(lps->atom->extract("x"));
    double** f = static_cast<double**>(lps->atom->extract("f"));
    double** v = static_cast<double**>(lps->atom->extract("v"));

    for(size_t i = 0; i < localSize; ++i)
    {
        ids[i] = static_cast<atomIndexes_t>(id[i]);
        pos[3*i] = x[i][0];
        pos[3*i+1] = x[i][1];
        pos[3*i+2] = x[i][2];
        forces[3*i] = f[i][0];
        forces[3*i+1] = f[i][1];
        forces[3*i+2] = f[i][2];
        vel[3*i] = v[i][0];
        vel[3*i+1] = v[i][1];
        vel[3*i+2] = v[i][2];
    }

    int32_t* simIt32 = static_cast<int32_t*>(lammps_extract_global(lps, "ntimestep"));
    thermo.insert({"simIt", static_cast<int32_t>(simIt32[0])});


    double temp = lammps_get_thermo(lps, "temp");
    thermo.insert({"temp", temp});

    double tot = lammps_get_thermo(lps, "etotal");
    thermo.insert({"tot", tot});

    double pot = lammps_get_thermo(lps, "pe");
    thermo.insert({"pot", pot});

    double kin = lammps_get_thermo(lps, "ke");
    thermo.insert({"kin", kin});

    double* dt = static_cast<double*>(lammps_extract_global(lps, "dt"));
    thermo.insert({"dt", dt[0]});

    double* sim_t = static_cast<double*>(lammps_extract_global(lps, "atime"));
    thermo.insert({"sim_t", sim_t[0]});
   
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    std::string taskName;
    std::string configFile;
    std::string lmpInitialState;
    std::string lmpGroupsFile;
    uint64_t maxNVESteps  = 1000;
    uint32_t intervalSteps = 100;
    uint64_t currentStep = 0;

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
        | lyra::opt( maxNVESteps, "maxnvesteps")
            ["--maxnvesteps"]
            ("Total number of steps to run the NVE.")
        | lyra::opt( intervalSteps, "interalsteps")
            ["--intervalsteps"]
            ("Number of steps to run between checking the inputs.")
        | lyra::opt( lmpGroupsFile, "lmpgroups")
            ["--lmpgroups"]
            ("Path to the definition of the groups to use.")
        ;

    auto result = cli.parse( { argc, argv } );
    if ( !result )
    {
        spdlog::critical("Unable to parse the command line: {}.", result.errorMessage());
        exit(1);
    }

    spdlog::info("Starting the task {}.", taskName);
    spdlog::info("Running the Lammps simulation for {} steps with an output frequency of {} steps.", maxNVESteps, intervalSteps);

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
    //int rank = handler.getTaskRank();

    executeScript(lps, lmpInitialState);
    auto simUnitStyle = getUnitStyle(lmpInitialState);
    auto simUnitValue = static_cast<std::underlying_type<radahn::lmp::SimCommandType>::type>(simUnitStyle);

    // Declare the global groups if provided
    bool hasPermanentAnchor = false;
    const std::string permanentAnchorName = "permanentAnchor";
    if(lmpGroupsFile != "")
    {
        //executeScript(lps, lmpGroups);
        spdlog::info("Loading groups from {}.", lmpGroupsFile);
        auto anchorIDS = getAnchorsIds(lmpGroupsFile);
        if(anchorIDS.size() > 0)
        {
            std::stringstream commandGroup;
            commandGroup << "group " << permanentAnchorName << " id";
            for(auto & id : anchorIDS)
            {
                commandGroup << " " << id;
            }
            executeCommand(lps, commandGroup.str());
            hasPermanentAnchor = true;
        }
    }

    std::vector<conduit::Node> receivedData;
    while(currentStep < maxNVESteps)
    {
        executeCommand(lps, "#### LOOP Start from Timestep " + std::to_string(currentStep) + " #####################################");
        auto resultReceive = handler.get("in", receivedData);
        if( resultReceive == godrick::MessageResponse::TERMINATE )
        {
            spdlog::info("Lammps received a terminate message from the engine. Exiting the loop.");
            break;
        }
        if (resultReceive == godrick::MessageResponse::ERROR)
        {
            spdlog::info("Lammps task received an error message. Abording.");
            break;
        }

        if(resultReceive == godrick::MessageResponse::TOKEN)
        {
            spdlog::info("Lammps received a token.");
        }

        auto cmdUtil = radahn::lmp::LammpsCommandsUtils();
        if(hasPermanentAnchor)
            cmdUtil.declarePermanentAnchorGroup(permanentAnchorName);

        if(resultReceive == godrick::MessageResponse::MESSAGES)
        {
            spdlog::info("Lammps received a regular message.");

            // Check that we have lammps commands
            if(receivedData[0].has_child("lmpcmds"))
            {
                //auto cmdUtil = radahn::core::LammpsCommandsUtils();
                if(!cmdUtil.loadCommandsFromConduit(receivedData[0]))
                {
                    spdlog::error("Something went wrong when try to parse the lammps commands. Abording the simulation loop.");
                    break;
                }                
            }
        }


        // Create the commands for the motors
        std::vector<std::string> doCommands;
        cmdUtil.writeDoCommands(doCommands);
        for(auto & cmd : doCommands)
            executeCommand(lps, cmd);

        // Create the time integration command
        executeCommand(lps, "#### Start INTEGRATION ");
        std::stringstream cmdFixNVE;
        cmdFixNVE<<"fix NVE "<<cmdUtil.getIntegrationGroup()<<" nve";
        executeCommand(lps, cmdFixNVE.str());

        // Advance the simulation
        executeCommand(lps, "run " + std::to_string(intervalSteps));

        // Undo the time integration
        std::stringstream cmdUnfixNVE;
        cmdUnfixNVE<<"unfix NVE";
        executeCommand(lps, cmdUnfixNVE.str());
        executeCommand(lps, "#### End INTEGRATION ");

        // Undo the motors commands
        std::vector<std::string> undoCommands;
        cmdUtil.writeUndoCommands(undoCommands);
        for(auto & cmd : undoCommands)
            executeCommand(lps, cmd);


        double simItD = lammps_get_thermo(lps, "step");
        simIt_t simIt = static_cast<simIt_t>(simItD);

        // Not using simIt to avoid potential rounding errors from double to uint64
        currentStep += intervalSteps; 

        // Extracting atom information
        std::vector<atomIndexes_t> ids;
        std::vector<atomPositions_t> pos;
        std::vector<atomForces_t> forces;
        std::vector<atomVelocities_t> vel;
        std::unordered_map<std::string, std::variant<double, int32_t> > thermos;
        extractAtomInformation(lps, ids, pos, forces, vel, thermos);

        /*if(ids.size() > 0)
        {
            for(size_t i = 0; i < ids.size(); ++i)
                spdlog::info("Rank {} Step {}, Atom {}, Position [{} {} {}]", rank, currentStep, ids[i], pos[3*i], pos[3*i+1], pos[3*i+2]);
        }
        else
            spdlog::info("Rank {} Step {} doesn't have any atoms.", rank, currentStep);*/


        conduit::Node rootMsg;
        conduit::Node& simData = rootMsg.add_child("simdata");
        simData["simIt"] = simIt;
        simData["atomIDs"] = ids;
        simData["atomPositions"] = pos;
        simData["atomForces"] = forces;
        simData["atomVelocities"] = vel;
        simData["units"] = simUnitValue;

        conduit::Node& thermosData = rootMsg.add_child("thermos");
        for(auto & t : thermos)
        {
            if(std::holds_alternative<double>(t.second))
                thermosData[t.first] = std::get<double>(t.second);
            else
                thermosData[t.first] = std::get<int32_t>(t.second);
        }
        handler.push("atoms", rootMsg, true);

        executeCommand(lps, "#### LOOP End at Timestep " + std::to_string(currentStep) + " #########################################");
    }
    spdlog::info("Lammps done. Closing godrick...");

    handler.close();

    spdlog::info("Lammps closing done. Exiting.");

    return EXIT_SUCCESS;
}