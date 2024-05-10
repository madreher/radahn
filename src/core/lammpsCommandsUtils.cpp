#include <radahn/core/lammpsCommandsUtils.h>

#include <spdlog/spdlog.h>

#include <ranges>

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
    
    // We have to copy the selection to a vector because the node is getting out of scope after this call
    atomIndexes_t* selection = node["selection"].value();
    auto nbAtoms = node["selection"].dtype().number_of_elements();
    auto selectionSpan = std::span<atomIndexes_t>( selection, static_cast<size_t>(nbAtoms));
    m_selection = std::vector<atomIndexes_t>(selectionSpan.begin(), selectionSpan.end());


    for(auto i = 0; i < nbAtoms; ++i)
        spdlog::info("{}", m_selection[static_cast<size_t>(i)]);

    m_origin = node["origin"].as_char8_str();
    spdlog::info("Reading the origin {}", m_origin);

    return true;
}

bool radahn::core::MoveLammpsCommand::writeDoCommands(std::vector<std::string>& cmds) const
{
    // Create the group
    std::stringstream cmd1;
    cmd1<<"group "<<m_origin<<"GRP id";
    for(auto & id : m_selection)
        cmd1<<" "<<id;
    cmds.push_back(cmd1.str());

    // Create the move command
    std::stringstream cmd2;
    cmd2<<"fix "<<m_origin<<"ID "<<m_origin<<"GRP move linear "<<m_vx.m_value<<" "<<m_vy.m_value<<" "<<m_vz.m_value<<"\n";
    cmds.push_back(cmd2.str());

    return true;
}

bool radahn::core::MoveLammpsCommand::writeUndoCommands(std::vector<std::string>& cmds) const
{
    // Undo the fix
    std::stringstream cmd1;
    cmd1<<"unfix "<<m_origin<<"ID";
    cmds.push_back(cmd1.str());

    // Undo the grp 
    std::stringstream cmd2;
    cmd2<<"group "<<m_origin<<"GRP delete";
    cmds.push_back(cmd2.str());

    return true;
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

bool radahn::core::WaitLammpsCommand::writeDoCommands(std::vector<std::string>& cmds) const
{
    (void)cmds;
    return true;
}

bool radahn::core::WaitLammpsCommand::writeUndoCommands(std::vector<std::string>& cmds) const
{
    (void)cmds;
    return true;
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

bool radahn::core::LammpsCommandsUtils::writeDoCommands(std::vector<std::string>& cmdsStr) const
{

    cmdsStr.push_back("#### Start DO motor commands");
    // Create the free group, which is going to be all minus the MoveCommand groups
    std::vector<std::string> nonIntegrationGroup;

    // Declare all the commands for each motor
    // This include group creations + motion commands
    bool result = true;
    for(auto & cmd : m_cmds)
    {
        result &= cmd->writeDoCommands(cmdsStr);

        // Check if the group can be added to the time integration process
        if(!cmd->needMotionInteration())
            nonIntegrationGroup.push_back(cmd->getGroupName());
    }

    // Create the moveable group for the integration process
    if(nonIntegrationGroup.size() == 0)
    {
        // All the groups can be moved
        std::stringstream cmd1;
        cmd1<<"group "<<m_nonIntegrateGroupName<<" empty";
        cmdsStr.push_back(cmd1.str());
    }
    else
    {
        std::stringstream cmd1;
        cmd1<<"group "<<m_nonIntegrateGroupName<<" union ";
        for(auto & grp : nonIntegrationGroup)
            cmd1<<" "<<grp;
        cmdsStr.push_back(cmd1.str());
    }
    std::stringstream ss;
    ss<<"group "<<m_integrateGroupName<<" subtract all nonintegrateGRP";
    cmdsStr.push_back(ss.str());

    cmdsStr.push_back("#### END DO motor commands");

    return result;
}

bool radahn::core::LammpsCommandsUtils::writeUndoCommands(std::vector<std::string>& cmds) const
{
    cmds.push_back("#### Start UNDO motor commands");

    // Do this in the reverse order as do
    std::stringstream cmd1;
    cmd1<<"group "<<m_integrateGroupName<<" delete";
    cmds.push_back(cmd1.str());

    std::stringstream cmd2;
    cmd2<<"group "<<m_nonIntegrateGroupName<<" delete";
    cmds.push_back(cmd2.str());

    bool result = true;
    for(auto & cmd : m_cmds | std::views::reverse)
    {
        result &= cmd->writeUndoCommands(cmds);
    }

    cmds.push_back("#### End UNDO motor commands");

    return result;
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