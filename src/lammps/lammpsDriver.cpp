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

struct Langevin
{
    std::vector<int> indices;
    std::string name;
    double startTemp;
    double endTemp;
    radahn::core::TimeQuantity damp;
    uint64_t seed;
};

std::vector<uint32_t> getAnchorsIds(json& document)
{
    std::vector<uint32_t> ids;
    for(auto & anchor : document["anchors"])
    {
        if(!anchor.contains("selection"))
        {
            spdlog::info("Could not find selection in JSON configuration file.");
            return {};
        }
        for(auto & id: anchor["selection"])
        {
            ids.push_back(id);
        }
    }
    return ids;
}

bool executeScript(LAMMPS* lps, const std::string& scriptPath, std::ofstream& commandsHistory)
{
    std::ifstream fdesc(scriptPath);

    // Only execute the script if we could open it
    if (fdesc.is_open())
    {
        // Execute the script
        lps->input->file(scriptPath.c_str());
        commandsHistory<<fdesc.rdbuf()<<"\n";
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
        // to() is C++23, have to do the conversion manually with C++20
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
                return radahn::core::from_lmp_string(words[1]);
            }
        }
    }

    throw std::runtime_error("Unable to find the units command.");
}

bool executeCommand(LAMMPS* lps, const std::string& cmd, std::ofstream& commandsHistory)
{
    lps->input->one(cmd);
    commandsHistory<<cmd<<"\n";
    return true;
}

void extractAtomInformation(
    LAMMPS* lps,
    std::vector<atomIndexes_t>& ids,
    std::vector<atomPositions_t>& pos,
    std::vector<atomForces_t>& forces,
    std::vector<atomVelocities_t>& vel,
    std::vector<std::string>& thermoFieldsRequested,
    std::unordered_map<std::string, std::variant<double, int32_t> >& thermo
    )
{
    // Allocating the buffers according to the local proc
    uint64_t localSize = static_cast<uint64_t>(lps->atom->nlocal);
    spdlog::info("Extracting {} atoms.", localSize);
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


    /*double temp = lammps_get_thermo(lps, "temp");
    thermo.insert({"temp", temp});

    double tot = lammps_get_thermo(lps, "etotal");
    thermo.insert({"tot", tot});

    double pot = lammps_get_thermo(lps, "pe");
    thermo.insert({"pot", pot});

    double kin = lammps_get_thermo(lps, "ke");
    thermo.insert({"kin", kin});*/

    double* dt = static_cast<double*>(lammps_extract_global(lps, "dt"));
    thermo.insert({"dt", dt[0]});

    double* sim_t = static_cast<double*>(lammps_extract_global(lps, "atime"));
    thermo.insert({"sim_t", sim_t[0]});

    for(auto & field : thermoFieldsRequested)
    {
        if(field.starts_with("v_"))
        {
            std::string varName = field.substr(2, field.size()-2);
            double* dptr = static_cast<double*>(lammps_extract_variable(lps, varName.c_str(), NULL));
            if(dptr)
            {
                thermo.insert({field, *dptr});
                lammps_free(dptr);
            }
            else 
            {
                spdlog::error("Unable to find the field {} {}.", varName, field);
            }
        }
        else if(field.starts_with("c_"))
        {
            std::string varName = field.substr(2, field.size()-2);
            double* dptr = static_cast<double*>(lammps_extract_compute(lps, varName.c_str(), LMP_STYLE_GLOBAL, LMP_TYPE_SCALAR));
            if(dptr)
            {
                thermo.insert({field, *dptr});
            }
            else 
            {
                spdlog::error("Unable to find the field {} {}.", varName, field);
            }
        }
        else
        {
            double data = lammps_get_thermo(lps, field.c_str());
            thermo.insert({field, data});
        }
    }
   
}

void sendLammpsData(LAMMPS* lps, uint8_t simUnitValue, godrick::mpi::GodrickMPI& handler, const std::string& phase, std::vector<std::string>& thermoFields)
{
    double simItD = lammps_get_thermo(lps, "step");
    simIt_t simIt = static_cast<simIt_t>(simItD);

    // Extracting atom information
    std::vector<atomIndexes_t> ids;
    std::vector<atomPositions_t> pos;
    std::vector<atomForces_t> forces;
    std::vector<atomVelocities_t> vel;
    std::unordered_map<std::string, std::variant<double, int32_t> > thermos;
    extractAtomInformation(lps, ids, pos, forces, vel, thermoFields, thermos);

    conduit::Node rootMsg;
    conduit::Node& simData = rootMsg.add_child("simdata");
    simData["simIt"] = simIt;
    simData["atomIDs"] = ids;
    simData["atomPositions"] = pos;
    simData["atomForces"] = forces;
    simData["atomVelocities"] = vel;
    simData["units"] = simUnitValue;
    simData["phase"] = std::string(phase); // NVT/NVE

    conduit::Node& thermosData = rootMsg.add_child("thermos");
    for(auto & t : thermos)
    {
        if(std::holds_alternative<double>(t.second))
            thermosData[t.first] = std::get<double>(t.second);
        else
            thermosData[t.first] = std::get<int32_t>(t.second);
    }
    handler.push("atoms", rootMsg, true);
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    std::string taskName;
    std::string configFile;
    std::string lmpInitialState;
    std::string lmpConfigFile;
    uint32_t intervalSteps = 100;
    uint64_t currentStep = 0;
    std::string ffType = "";

    // NVT 
    std::string nvtType{"none"}; // none, createVelocity, nvtPhase
    bool enableNVT = false;

    // Settings nvtPhase
    uint64_t nbNVTSteps = 0;
    radahn::core::TimeQuantity damp{0, radahn::core::SimUnits::LAMMPS_METAL};
    double startTemp = 0.0; // K
    double endTemp = 0.0;
    uint64_t seedNVT = 123456789;

    // Settings createVelocity
    double tempCreateVel = 0.0; // K
    uint64_t seedCreateVel = 123456789;

    // NVE
    uint64_t maxNVESteps  = 1000;
    std::vector<Langevin> thermostats;
    

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
        | lyra::opt( lmpConfigFile, "lmpconfig")
            ["--lmpconfig"]
            ("Path to the configuration file for Lammps.")
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

    // Creating the log file for the commands
    std::ofstream logFile("full.log.lammps");

    //std::stringstream logFile;
    executeScript(lps, lmpInitialState, logFile);
    auto simUnitStyle = getUnitStyle(lmpInitialState);
    auto simUnitValue = static_cast<std::underlying_type<radahn::core::SimUnits>::type>(simUnitStyle);

    // Declare the global groups if provided
    bool hasPermanentAnchor = false;
    const std::string permanentAnchorName = "permanentAnchor";


    if(lmpConfigFile != "")
    {
        //executeScript(lps, lmpGroups);
        spdlog::info("Loading config from {}.", lmpConfigFile);

        json document = json::parse(std::ifstream(lmpConfigFile));

        if(!document.contains("header"))
        {
            spdlog::critical("Could not find header in JSON file {}.", lmpConfigFile);
            exit(-1);
        }

        uint32_t version = document["header"].value("version", 0u);
        if(version != 0)
        {
            spdlog::critical("Unsupported version {} in JSON file {}.", version, lmpConfigFile);
            exit(-1);
        }

        if(!document["header"].contains("units"))
        {
            spdlog::critical("Unable to detect the units in the header in the JSON file {}.", lmpConfigFile);
            exit(-1);
        }
        std::string unitConfig = document["header"]["units"].get<std::string>();
        auto configSimUnit = radahn::core::from_string(unitConfig);

        if(!document["header"].contains("fftype"))
        {
            spdlog::critical("Unable to detect the type of forcefield used in the header in the JSON file {}.", lmpConfigFile);
            exit(-1);
        }
        ffType = document["header"]["fftype"].get<std::string>();
        std::set<std::string> supportedFFTypes{"airebo", "rebo", "airebo-m", "reax"};
        if(supportedFFTypes.count(ffType) == 0)
        {
            spdlog::critical("Received an unsupported type of forcefield \"{}\". Abording.", ffType);
            exit(-1);
        }

        if(document.contains("anchors"))
        {
            // Check for anchors
            auto anchorIDS = getAnchorsIds(document);
            if(anchorIDS.size() > 0)
            {
                std::stringstream commandGroup;
                commandGroup << "group " << permanentAnchorName << " id";
                for(auto & id : anchorIDS)
                {
                    commandGroup << " " << id;
                }
                executeCommand(lps, commandGroup.str(), logFile);
                hasPermanentAnchor = true;
            }
        }

        if(document.contains("thermostats"))
        {
            for(auto & thermostatNode : document["thermostats"])
            {
                if(!thermostatNode.contains("type"))
                {
                    spdlog::error("Unable to find the type of a thermostat. Abording.");
                    exit(-1);
                }

                auto thermostatType = thermostatNode["type"].get<std::string>();
                if(thermostatType.compare("langevin") == 0)
                {
                    Langevin thermostat;
                    for(auto & id: thermostatNode["selection"])
                    {
                        thermostat.indices.push_back(id);
                    }
                    thermostat.name = thermostatNode.value("name", "defaultLangevin");
                    thermostat.startTemp = thermostatNode.value("startTemp", 300.0);
                    thermostat.endTemp = thermostatNode.value("endTemp", 300.0);
                    thermostat.damp = radahn::core::TimeQuantity(thermostatNode.value("damp", 300.0), configSimUnit);
                    thermostat.damp.convertTo(simUnitStyle);
                    thermostat.seed = thermostatNode.value("seed", 1234u);
                    thermostats.push_back(std::move(thermostat));
                }

            }
        }

        // check for NVT
        if(document.contains("nvtConfig"))
        {
            auto & configNVT = document["nvtConfig"];
            if(!configNVT.contains("type"))
            {
                spdlog::error("Thermalization requested but unable to find the type in \"nvtConfig\". Abording.");
                exit(-1);
            }

            nvtType = configNVT["type"].get<std::string>();
            if(nvtType.compare("nvtPhase") == 0)
            {
                nbNVTSteps = configNVT.value("steps", 1000u);
                startTemp = configNVT.value("startTemp", 0.0);
                endTemp = configNVT.value("endTemp", 300.0);
                damp.m_value = configNVT.value("damp", 1000.0);
                damp.m_unit = configSimUnit;
                damp.convertTo(simUnitStyle);
                seedNVT = configNVT.value("seed", 123456789u);
                spdlog::info("Detecting a request for a NVT phase for {} steps.", nbNVTSteps);
                enableNVT = true;
            }
            else if(nvtType.compare("createVelocity") == 0)
            {
                tempCreateVel = configNVT.value("temp", 300.0);
                seedCreateVel = configNVT.value("seed", 123456789u);
                enableNVT = true;
            }
            else 
            {
                spdlog::error("Thermalization requested but unable to recognize the type {} in \"nvtConfig\". Abording.", nvtType);
                exit(-1);
            }
        }
    }
    logFile.flush();

    // NVT Section
    currentStep = 0;
    if(enableNVT && nvtType.compare("nvtPhase") == 0)
    {
        executeCommand(lps, "#### PHASE NVT Start from Timestep " + std::to_string(currentStep) + " #####################################", logFile);

        // We first need to initialize the velocities before doing the NVT loop
        if(hasPermanentAnchor)
        {
            std::string cmdFreeGroup = "group freeGlobal subtract all " + permanentAnchorName;
            executeCommand(lps, cmdFreeGroup, logFile);

            std::stringstream cmdCreateVelocities;
            // https://docs.lammps.org/velocity.html
            cmdCreateVelocities<<"velocity freeGlobal create "<<startTemp<<" "<<seedNVT;
            executeCommand(lps, cmdCreateVelocities.str(), logFile);

            //std::string cmdFreeUngroup = "group freeGlobal delete";
            //executeCommand(lps, cmdFreeUngroup, logFile);

            // Create the NVT fix 
            std::stringstream cmdFixLangevin;
            // https://docs.lammps.org/fix_langevin.html
            // fix ID group-ID langevin Tstart Tstop damp seed keyword values ...
            cmdFixLangevin<<"fix nvtlangevin freeGlobal langevin";
            cmdFixLangevin<< " "<<startTemp;
            cmdFixLangevin<< " "<<endTemp;
            cmdFixLangevin<< " "<<damp.m_value; // Was converted to the right unit when loaded
            cmdFixLangevin<< " "<<seedNVT;
            executeCommand(lps, cmdFixLangevin.str(), logFile);

            std::stringstream cmdFixNVT;
            cmdFixNVT<<"fix nvtnve freeGlobal nve";
            executeCommand(lps, cmdFixNVT.str(), logFile);
        }
        else 
        {
            std::stringstream cmdCreateVelocities;
            // https://docs.lammps.org/velocity.html
            cmdCreateVelocities<<"velocity all create "<<startTemp<<" "<<seedNVT;
            executeCommand(lps, cmdCreateVelocities.str(), logFile);

            // Create the NVT fix 
            std::stringstream cmdFixLangevin;
            // https://docs.lammps.org/fix_langevin.html
            // fix ID group-ID langevin Tstart Tstop damp seed keyword values ...
            cmdFixLangevin<<"fix nvtlangevin all langevin";
            cmdFixLangevin<< " "<<startTemp;
            cmdFixLangevin<< " "<<endTemp;
            cmdFixLangevin<< " "<<damp.m_value; // Was converted to the right unit when loaded
            cmdFixLangevin<< " "<<seedNVT;
            executeCommand(lps, cmdFixLangevin.str(), logFile);

            std::stringstream cmdFixNVT;
            cmdFixNVT<<"fix nvtnve all nve";
            executeCommand(lps, cmdFixNVT.str(), logFile);
        }

        // Flushing the commands we have executed to file
        // This is costly, but during the debugging stage where the simulation might break, it's a necessary cost.
        logFile.flush();
        
        std::vector<std::string> thermoFieldsNVT({"step", "time", "etotal", "pe", "epair"});
        // DEBUG FOR KEVIN. THIS IS SETUP In THE INPUT
        if(ffType.compare("airebo") == 0)
        {
            thermoFieldsNVT.push_back("v_REBO");
            thermoFieldsNVT.push_back("v_LJ");
            thermoFieldsNVT.push_back("v_TORSION");
        }

        // At this point, everything is declared, we just have to call run
        std::vector<conduit::Node> receivedData;
        while(currentStep < nbNVTSteps)
        {
            executeCommand(lps, "#### LOOP NVT Start from Timestep " + std::to_string(currentStep) + " #####################################", logFile);
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

            // During the NVT phase, we don't expect anything from the motor engine, no need to check the message content further.

            // Advance the simulation
            executeCommand(lps, "run " + std::to_string(intervalSteps), logFile);

            // Sending the simulation data 
            sendLammpsData(lps, simUnitValue, handler, "NVT", thermoFieldsNVT);

            // Not using simIt to avoid potential rounding errors from double to uint64
            currentStep += intervalSteps; 

            executeCommand(lps, "#### LOOP NVT End at Timestep " + std::to_string(currentStep) + " #########################################", logFile);

            // Flushing the commands we have executed to file
            // This is costly, but during the debugging stage where the simulation might break, it's a necessary cost.
            logFile.flush();
        }

        // NVT phase is complete, cleaning things up
        if(hasPermanentAnchor)
        {
            std::string unfixNVE{"unfix nvtnve"};
            executeCommand(lps, unfixNVE, logFile);

            std::string unfixlangevin{"unfix nvtlangevin"};
            executeCommand(lps, unfixlangevin, logFile);

            std::string ungroup{"group freeGlobal delete"};
            executeCommand(lps, ungroup, logFile);
        }
        else 
        {
            std::string unfixNVE{"unfix nvtnve"};
            executeCommand(lps, unfixNVE, logFile);

            std::string unfixlangevin{"unfix nvtlangevin"};
            executeCommand(lps, unfixlangevin, logFile);
        }

        executeCommand(lps, "#### PHASE NVT END at Timestep " + std::to_string(currentStep) + " #####################################", logFile);
    }
    else if(enableNVT && nvtType.compare("createVelocity") == 0)
    {
        // We first need to initialize the velocities before doing the NVT loop
        if(hasPermanentAnchor)
        {
            executeCommand(lps, "#### LOOP NVT Start from Timestep " + std::to_string(currentStep) + " #####################################", logFile);
            
            std::string cmdFreeGroup = "group freeGlobal subtract all " + permanentAnchorName;
            executeCommand(lps, cmdFreeGroup, logFile);

            std::stringstream cmdCreateVelocities;
            // https://docs.lammps.org/velocity.html
            cmdCreateVelocities<<"velocity freeGlobal create "<<tempCreateVel<<" "<<seedCreateVel;
            executeCommand(lps, cmdCreateVelocities.str(), logFile);

            std::string ungroup{"group freeGlobal delete"};
            executeCommand(lps, ungroup, logFile);

            executeCommand(lps, "#### PHASE NVT END at Timestep " + std::to_string(currentStep) + " #####################################", logFile);
        }
        else 
        {
            executeCommand(lps, "#### LOOP NVT Start from Timestep " + std::to_string(currentStep) + " #####################################", logFile);

            std::stringstream cmdCreateVelocities;
            // https://docs.lammps.org/velocity.html
            cmdCreateVelocities<<"velocity all create "<<tempCreateVel<<" "<<seedCreateVel;
            executeCommand(lps, cmdCreateVelocities.str(), logFile);

            executeCommand(lps, "#### PHASE NVT END at Timestep " + std::to_string(currentStep) + " #####################################", logFile);
        }
    }
    // Flushing the commands we have executed to file
    logFile.flush();

    // PRE NVE Section
    std::vector<std::string> thermoFields({"step", "time", "etotal", "pe", "epair"});
    std::vector<std::string> thermoGroups;
    executeCommand(lps, "#### PRE NVE Start from Timestep " + std::to_string(currentStep) + " #####################################", logFile);
    if(thermostats.size() > 0)
    {
        for(auto & thermostat : thermostats)
        {
            std::stringstream cmdGroup;
            cmdGroup<<"group "<<thermostat.name<<" id";
            for(auto & id : thermostat.indices)
                cmdGroup<<" "<<id;
            executeCommand(lps, cmdGroup.str(), logFile);

            thermoGroups.push_back(thermostat.name);

            std::stringstream cmdComputeTemp;
            cmdComputeTemp<<"compute temp_"<<thermostat.name<<" "<<thermostat.name<<" temp";
            executeCommand(lps, cmdComputeTemp.str(), logFile);
            thermoFields.push_back("c_temp_" + thermostat.name);

            std::stringstream cmdComputeKe;
            cmdComputeKe<<"compute ke_"<<thermostat.name<<" "<<thermostat.name<<" ke";
            executeCommand(lps,cmdComputeKe.str(), logFile);
            thermoFields.push_back("c_ke_" + thermostat.name);

            std::stringstream cmdFixLangevin;
            cmdFixLangevin<<"fix lgv"<<thermostat.name<<" "<<thermostat.name<<" langevin "<<thermostat.startTemp<<" "<<thermostat.endTemp<<" "<<thermostat.damp.m_value<<" "<<thermostat.seed;
            executeCommand(lps,cmdFixLangevin.str(), logFile);
        }  
    }

    std::stringstream cmdGroupMobile;
    if(hasPermanentAnchor)
        cmdGroupMobile<<"group mobileAtoms subtract all "<<permanentAnchorName;
    else
        cmdGroupMobile<<"group mobileAtoms union all";
    executeCommand(lps, cmdGroupMobile.str(), logFile);
    std::string cmdMobileComputeTemp{"compute temp_mobileAtoms mobileAtoms temp"};
    executeCommand(lps, cmdMobileComputeTemp, logFile);
    thermoFields.push_back("c_temp_mobileAtoms");
    std::string cmdMobileComputeKE{"compute ke_mobileAtoms mobileAtoms ke"};
    executeCommand(lps, cmdMobileComputeKE, logFile);
    thermoFields.push_back("c_ke_mobileAtoms");

    std::stringstream cmdGroupThermalized;
    if(thermoGroups.size() > 0)
    {
        cmdGroupThermalized<<"group thermalizedAtoms union";
        for(auto & grp : thermoGroups)
            cmdGroupThermalized<<" "<<grp;
    }
    else
    {
        cmdGroupThermalized<<"group thermalizedAtoms empty";
    }
    executeCommand(lps, cmdGroupThermalized.str(), logFile);
    std::string cmdThermalizedComputeTemp{"compute temp_thermalizedAtoms thermalizedAtoms temp"};
    executeCommand(lps, cmdThermalizedComputeTemp, logFile);
    thermoFields.push_back("c_temp_thermalizedAtoms");
    std::string cmdThermalizedComputeKE{"compute ke_thermalizedAtoms thermalizedAtoms ke"};
    executeCommand(lps, cmdThermalizedComputeKE, logFile);
    thermoFields.push_back("c_ke_thermalizedAtoms");

    std::string cmdGroupNonThermalized{"group nonthermalizedAtoms subtract mobileAtoms thermalizedAtoms"};
    executeCommand(lps, cmdGroupNonThermalized, logFile);
    std::string cmdNonThermalizedComputeTemp{"compute temp_nonthermalizedAtoms nonthermalizedAtoms temp"};
    executeCommand(lps, cmdNonThermalizedComputeTemp, logFile);
    thermoFields.push_back("c_temp_nonthermalizedAtoms");
    std::string cmdNonThermalizedComputeKE{"compute ke_nonthermalizedAtoms nonthermalizedAtoms ke"};
    executeCommand(lps, cmdNonThermalizedComputeKE, logFile);
    thermoFields.push_back("c_ke_nonthermalizedAtoms");

    if(hasPermanentAnchor)
    {
        std::stringstream cmdAnchorComputeTemp;
        cmdAnchorComputeTemp<<"compute temp_"<<permanentAnchorName<<" "<<permanentAnchorName<<" temp";
        executeCommand(lps, cmdAnchorComputeTemp.str(), logFile);
        thermoFields.push_back("c_temp_" + permanentAnchorName);

        std::stringstream cmdAnchorComputeKE;
        cmdAnchorComputeKE<<"compute ke_"<<permanentAnchorName<<" "<<permanentAnchorName<<" ke";
        executeCommand(lps, cmdAnchorComputeKE.str(), logFile);
        thermoFields.push_back("c_ke_" + permanentAnchorName);
    }

    // DEBUG FOR KEVIN. THIS IS SETUP In THE INPUT
    if(ffType.compare("airebo") == 0)
    {
        thermoFields.push_back("v_REBO");
        thermoFields.push_back("v_LJ");
        thermoFields.push_back("v_TORSION");
    }

    executeCommand(lps, "thermo 50", logFile);
    std::stringstream cmdThermoStyle;
    cmdThermoStyle<<"thermo_style custom";
    for(auto & field : thermoFields)
        cmdThermoStyle<<" "<<field;
    executeCommand(lps, cmdThermoStyle.str(), logFile);
    executeCommand(lps, "thermo_modify lost error flush yes", logFile);

    executeCommand(lps, "#### PRE NVE End from Timestep " + std::to_string(currentStep) + " #####################################", logFile);

    // NVE Section
    std::vector<conduit::Node> receivedData;
    uint64_t currentNVEStep = 0;
    while(currentNVEStep < maxNVESteps)
    {
        executeCommand(lps, "#### LOOP NVE Start from Timestep " + std::to_string(currentStep) + " #####################################", logFile);
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


        // Gathering the commands we will need to execute
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

        // All the commands are registed to the util object, now we can generate the correspinding Lammps commands


        // Create the commands for the motors
        std::vector<std::string> doCommands;
        cmdUtil.writeDoCommands(doCommands);
        for(auto & cmd : doCommands)
            executeCommand(lps, cmd, logFile);

        // Create the time integration command
        executeCommand(lps, "#### Start INTEGRATION ", logFile);
        std::stringstream cmdFixNVE;
        cmdFixNVE<<"fix NVE "<<cmdUtil.getIntegrationGroup()<<" nve";
        executeCommand(lps, cmdFixNVE.str(), logFile);

        // Advance the simulation
        executeCommand(lps, "run " + std::to_string(intervalSteps), logFile);

        // Undo the time integration
        std::stringstream cmdUnfixNVE;
        cmdUnfixNVE<<"unfix NVE";
        executeCommand(lps, cmdUnfixNVE.str(), logFile);
        executeCommand(lps, "#### End INTEGRATION ", logFile);

        // Undo the motors commands
        std::vector<std::string> undoCommands;
        cmdUtil.writeUndoCommands(undoCommands);
        for(auto & cmd : undoCommands)
            executeCommand(lps, cmd, logFile);

        // Sending the simulation data 
        sendLammpsData(lps, simUnitValue, handler, "NVE", thermoFields);

        // Not using simIt to avoid potential rounding errors from double to uint64
        currentStep += intervalSteps; 
        currentNVEStep += intervalSteps;

        executeCommand(lps, "#### LOOP NVE End at Timestep " + std::to_string(currentStep) + " #########################################", logFile);

        // Flushing the commands we have executed to file
        logFile.flush();
    }
    spdlog::info("Lammps done. Closing godrick...");

    handler.close();

    spdlog::info("Lammps closing done. Exiting.");

    return EXIT_SUCCESS;
}