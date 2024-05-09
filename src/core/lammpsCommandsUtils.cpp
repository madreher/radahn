#include <radahn/core/lammpsCommandsUtils.h>

#include <spdlog/spdlog.h>

bool radahn::core::MoveLammpsCommand::loadFromConduit(conduit::Node& node)
{
    if(!node.has_child("cmdType"))
    {   
        spdlog::error("Trying to read a MoveLammpsCommand by the given conduit node doesn't have a type.");
        return false;
    }
    auto cmdType = node["cmdType"].to_uint32();
    if(radahn::core::SimCommandType(cmdType) != radahn::core::SimCommandType::SIM_COMMAND_LMP_MOVE)
    {
        spdlog::error("Trying to read a MoveLammpsCommand by the given conduit node doesn't have the right type.");
        return false;
    }

    auto vx = node["vx"].to_float64();   // Need to send units as well
    auto vy = node["vy"].to_float64();
    auto vz = node["vz"].to_float64();
    auto unit = node["vunits"].to_uint32();
    m_vx = VelocityQuantity(vx, SimUnits(unit));
    m_vy = VelocityQuantity(vy, SimUnits(unit));
    m_vz = VelocityQuantity(vz, SimUnits(unit));
    atomIndexes_t* selection = node["selection"].value();
    m_selection = std::span<atomIndexes_t>( selection, static_cast<size_t>(node["selection"].dtype().number_of_elements()));
    m_origin = node["origin"].to_string();

    return true;
}

std::string radahn::core::MoveLammpsCommand::writeDoCommands() const
{
    // Create the group
    std::stringstream ss;
    ss<<"group "<<m_origin<<"GRP id";
    for(auto & id : m_selection)
        ss<<" "<<id;
    ss<<"\n";

    // Create the move command
    ss<<"fix "<<m_origin<<"ID "<<m_origin<<"GRP move linear "<<m_vx.m_value<<" "<<m_vy.m_value<<" "<<m_vz.m_value<<"\n";

    return ss.str();
}

std::string radahn::core::MoveLammpsCommand::getGroupName() const
{
    std::stringstream ss;
    ss<<m_origin<<"GRP";
    return ss.str();
}

bool radahn::core::WaitLammpsCommand::loadFromConduit(conduit::Node& node)
{
    if(!node.has_child("cmdType"))
    {   
        spdlog::error("Trying to read a WaitLammpsCommand by the given conduit node doesn't have a type.");
        return false;
    }
    auto cmdType = node["cmdType"].to_uint32();
    if(radahn::core::SimCommandType(cmdType) != radahn::core::SimCommandType::SIM_COMMAND_WAIT)
    {
        spdlog::error("Trying to read a WaitLammpsCommand by the given conduit node doesn't have the right type.");
        return false;
    }

    m_origin = node["origin"].to_string();

    return true;
}

std::string radahn::core::WaitLammpsCommand::writeDoCommands() const
{
    return "";
}


bool radahn::core::LammpsCommandsUtils::loadCommandsFromConduit(conduit::Node& cmds)
{
    spdlog::info("Reading commands from lammpscommandsutils.");
    cmds.print_detailed();
    spdlog::info("End reading full commands from lammps commandsutils.");
    if(cmds.has_child("lmpcmds"))
    {
        auto cmdIter = cmds["lmpcmds"].children();
        while (cmdIter.has_next())
        {
            auto cmdNode = cmdIter.next();
            cmdNode.print_detailed();
            if(!cmdNode.has_child("cmdType"))
            {
                spdlog::error("Unable to identify the type of a command for Lammps.");
                return false;
            }
            auto cmdType = radahn::core::SimCommandType(cmdNode["cmdType"].to_uint32());
            switch(cmdType)
            {
                case radahn::core::SimCommandType::SIM_COMMAND_LMP_MOVE:
                {
                    std::shared_ptr<MoveLammpsCommand> cmd = std::make_shared<MoveLammpsCommand>();
                    if(!cmd->loadFromConduit(cmdNode))
                        return false;
                    m_cmds.push_back(cmd);
                    break;
                }
                case radahn::core::SimCommandType::SIM_COMMAND_WAIT:
                {
                    std::shared_ptr<WaitLammpsCommand> cmd = std::make_shared<WaitLammpsCommand>();
                    if(!cmd->loadFromConduit(cmdNode))
                        return false;
                    m_cmds.push_back(cmd);
                    break;
                }
                default:
                {
                    spdlog::error("Lammps received an unsupported command from the engine.");
                    return false;
                }
            }
        }
        
    }
    else
    {
        spdlog::error("Unable to find commands in the commands sent by the engine.");
        return false;
    }

    return true;
}



void radahn::core::LammpsCommandsUtils::registerMoveCommandToConduit(conduit::Node& node, const std::string& name, VelocityQuantity vx, VelocityQuantity vy, VelocityQuantity vz, const std::vector<atomIndexes_t>& selection)
{
    // Heavy syntax still required for c++20
    // for c++23, prefer using std::to_underlying
    node["cmdType"] = static_cast<std::underlying_type<radahn::core::SimCommandType>::type>(radahn::core::SimCommandType::SIM_COMMAND_LMP_MOVE);
    node["origin"] = name;
    node["vx"] = vx.m_value;   // Need to send units as well
    node["vy"] = vy.m_value;
    node["vz"] = vz.m_value;
    node["vunits"] = static_cast<std::underlying_type<radahn::core::SimUnits>::type>(vx.m_unit);
    node["selection"] = selection;

}

void radahn::core::LammpsCommandsUtils::registerWaitCommandToConduit(conduit::Node& node, const std::string& name)
{
    node["cmdType"] = static_cast<std::underlying_type<radahn::core::SimCommandType>::type>(radahn::core::SimCommandType::SIM_COMMAND_WAIT);
    node["origin"] = name;
}