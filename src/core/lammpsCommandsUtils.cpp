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

    m_origin = node["origin"].as_char8_str();
    //spdlog::info("Reading the origin {}", m_origin);

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

bool radahn::core::AddForceLammpsCommand::loadFromConduit(conduit::Node& node)
{
    if(!node.has_child("cmdType"))
    {   
        spdlog::error("Trying to read a AddForceLammpsCommand by the given conduit node doesn't have a type.");
        return false;
    }
    auto cmdType = node["cmdType"].to_uint32();
    if(radahn::core::SimCommandType(cmdType) != radahn::core::SimCommandType::SIM_COMMAND_LMP_ADD_FORCE)
    {
        spdlog::error("Trying to read a AddForceLammpsCommand by the given conduit node doesn't have the right type.");
        return false;
    }

    auto fx = node["fx"].to_float64();   // Need to send units as well
    auto fy = node["fy"].to_float64();
    auto fz = node["fz"].to_float64();
    auto unit = node["tunits"].to_uint32();
    m_fx = ForceQuantity(fx, SimUnits(unit));
    m_fy = ForceQuantity(fy, SimUnits(unit));
    m_fz = ForceQuantity(fz, SimUnits(unit));
    
    // We have to copy the selection to a vector because the node is getting out of scope after this call
    atomIndexes_t* selection = node["selection"].value();
    auto nbAtoms = node["selection"].dtype().number_of_elements();
    auto selectionSpan = std::span<atomIndexes_t>( selection, static_cast<size_t>(nbAtoms));
    m_selection = std::vector<atomIndexes_t>(selectionSpan.begin(), selectionSpan.end());

    m_origin = node["origin"].as_char8_str();
    //spdlog::info("Reading the origin {}", m_origin);

    return true;
}

bool radahn::core::AddForceLammpsCommand::writeDoCommands(std::vector<std::string>& cmds) const
{
    // Create the group
    std::stringstream cmd1;
    cmd1<<"group "<<m_origin<<"GRP id";
    for(auto & id : m_selection)
        cmd1<<" "<<id;
    cmds.push_back(cmd1.str());

    // Create the move command
    std::stringstream cmd2;
    cmd2<<"fix "<<m_origin<<"ID "<<m_origin<<"GRP addforce "<<m_fx.m_value<<" "<<m_fy.m_value<<" "<<m_fz.m_value<<"\n";
    cmds.push_back(cmd2.str());

    return true;
}

bool radahn::core::AddForceLammpsCommand::writeUndoCommands(std::vector<std::string>& cmds) const
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

std::string radahn::core::AddForceLammpsCommand::getGroupName() const
{
    std::stringstream ss;
    ss<<m_origin<<"GRP";
    return ss.str();
}

bool radahn::core::AddTorqueLammpsCommand::loadFromConduit(conduit::Node& node)
{
    if(!node.has_child("cmdType"))
    {   
        spdlog::error("Trying to read a AddTorqueLammpsCommand by the given conduit node doesn't have a type.");
        return false;
    }
    auto cmdType = node["cmdType"].to_uint32();
    if(radahn::core::SimCommandType(cmdType) != radahn::core::SimCommandType::SIM_COMMAND_LMP_ADD_TORQUE)
    {
        spdlog::error("Trying to read a AddTorqueLammpsCommand by the given conduit node doesn't have the right type.");
        return false;
    }

    auto tx = node["tx"].to_float64();   // Need to send units as well
    auto ty = node["ty"].to_float64();
    auto tz = node["tz"].to_float64();
    auto unit = node["tunits"].to_uint32();
    m_tx = TorqueQuantity(tx, SimUnits(unit));
    m_ty = TorqueQuantity(ty, SimUnits(unit));
    m_tz = TorqueQuantity(tz, SimUnits(unit));
    
    // We have to copy the selection to a vector because the node is getting out of scope after this call
    atomIndexes_t* selection = node["selection"].value();
    auto nbAtoms = node["selection"].dtype().number_of_elements();
    auto selectionSpan = std::span<atomIndexes_t>( selection, static_cast<size_t>(nbAtoms));
    m_selection = std::vector<atomIndexes_t>(selectionSpan.begin(), selectionSpan.end());

    m_origin = node["origin"].as_char8_str();
    //spdlog::info("Reading the origin {}", m_origin);

    return true;
}

bool radahn::core::AddTorqueLammpsCommand::writeDoCommands(std::vector<std::string>& cmds) const
{
    // Create the group
    std::stringstream cmd1;
    cmd1<<"group "<<m_origin<<"GRP id";
    for(auto & id : m_selection)
        cmd1<<" "<<id;
    cmds.push_back(cmd1.str());

    // Create the move command
    std::stringstream cmd2;
    cmd2<<"fix "<<m_origin<<"ID "<<m_origin<<"GRP addtorque "<<m_tx.m_value<<" "<<m_ty.m_value<<" "<<m_tz.m_value<<"\n";
    cmds.push_back(cmd2.str());

    return true;
}

bool radahn::core::AddTorqueLammpsCommand::writeUndoCommands(std::vector<std::string>& cmds) const
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

std::string radahn::core::AddTorqueLammpsCommand::getGroupName() const
{
    std::stringstream ss;
    ss<<m_origin<<"GRP";
    return ss.str();
}

bool radahn::core::RotateLammpsCommand::loadFromConduit(conduit::Node& node)
{
    if(!node.has_child("cmdType"))
    {   
        spdlog::error("Trying to read a RotateLammpsCommand by the given conduit node doesn't have a type.");
        return false;
    }
    auto cmdType = node["cmdType"].to_uint32();
    if(radahn::core::SimCommandType(cmdType) != radahn::core::SimCommandType::SIM_COMMAND_LMP_ROTATE)
    {
        spdlog::error("Trying to read a RotateLammpsCommand by the given conduit node doesn't have the right type.");
        return false;
    }

    auto px = node["px"].to_float64();   // Need to send units as well
    auto py = node["py"].to_float64();
    auto pz = node["pz"].to_float64();
    auto unit = node["punits"].to_uint32();
    m_px = DistanceQuantity(px, SimUnits(unit));
    m_py = DistanceQuantity(py, SimUnits(unit));
    m_pz = DistanceQuantity(pz, SimUnits(unit));

    m_ax = node["ax"].to_float64();
    m_ay = node["ay"].to_float64();
    m_az = node["az"].to_float64();

    auto period = node["period"].to_float64();
    auto periodUnit = node["periodunits"].to_uint32();
    m_period = TimeQuantity(period, SimUnits(periodUnit));
    
    // We have to copy the selection to a vector because the node is getting out of scope after this call
    atomIndexes_t* selection = node["selection"].value();
    auto nbAtoms = node["selection"].dtype().number_of_elements();
    auto selectionSpan = std::span<atomIndexes_t>( selection, static_cast<size_t>(nbAtoms));
    m_selection = std::vector<atomIndexes_t>(selectionSpan.begin(), selectionSpan.end());

    m_origin = node["origin"].as_char8_str();
    //spdlog::info("Reading the origin {}", m_origin);

    return true;
}

bool radahn::core::RotateLammpsCommand::writeDoCommands(std::vector<std::string>& cmds) const
{
    // Create the group
    std::stringstream cmd1;
    cmd1<<"group "<<m_origin<<"GRP id";
    for(auto & id : m_selection)
        cmd1<<" "<<id;
    cmds.push_back(cmd1.str());

    // Create the move command
    std::stringstream cmd2;
    cmd2<<"fix "<<m_origin<<"ID "<<m_origin<<"GRP move rotate "<<m_px.m_value<<" "<<m_py.m_value<<" "<<m_pz.m_value<<" "<<m_ax<<" "<<m_ay<<" "<<m_az<<" "<<m_period.m_value<<"\n";
    cmds.push_back(cmd2.str());

    return true;
}

bool radahn::core::RotateLammpsCommand::writeUndoCommands(std::vector<std::string>& cmds) const
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

std::string radahn::core::RotateLammpsCommand::getGroupName() const
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
    //spdlog::info("Reading commands from lammpscommandsutils.");
    //cmds.print_detailed();
    //spdlog::info("End reading full commands from lammps commandsutils.");
    if(cmds.has_child("lmpcmds"))
    {
        auto cmdIter = cmds["lmpcmds"].children();
        while (cmdIter.has_next())
        {
            auto cmdNode = cmdIter.next();
            //cmdNode.print_detailed();
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
                case radahn::core::SimCommandType::SIM_COMMAND_LMP_ROTATE:
                {
                    std::shared_ptr<RotateLammpsCommand> cmd = std::make_shared<RotateLammpsCommand>();
                    if(!cmd->loadFromConduit(cmdNode))
                        return false;
                    m_cmds.push_back(cmd);
                    break;
                }
                case radahn::core::SimCommandType::SIM_COMMAND_LMP_ADD_FORCE:
                {
                    std::shared_ptr<AddForceLammpsCommand> cmd = std::make_shared<AddForceLammpsCommand>();
                    if(!cmd->loadFromConduit(cmdNode))
                        return false;
                    m_cmds.push_back(cmd);
                    break;
                }
                case radahn::core::SimCommandType::SIM_COMMAND_LMP_ADD_TORQUE:
                {
                    std::shared_ptr<AddTorqueLammpsCommand> cmd = std::make_shared<AddTorqueLammpsCommand>();
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
        if(!cmd->needMotionIntegration())
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

void radahn::core::LammpsCommandsUtils::registerAddForceCommandToConduit(conduit::Node& node, const std::string& name, ForceQuantity fx, ForceQuantity fy, ForceQuantity fz, const std::vector<atomIndexes_t>& selection)
{
    // Heavy syntax still required for c++20
    // for c++23, prefer using std::to_underlying
    node["cmdType"] = static_cast<std::underlying_type<radahn::core::SimCommandType>::type>(radahn::core::SimCommandType::SIM_COMMAND_LMP_ADD_FORCE);
    node["origin"] = name;
    node["fx"] = fx.m_value;   // Need to send units as well
    node["fy"] = fy.m_value;
    node["fz"] = fz.m_value;
    node["funits"] = static_cast<std::underlying_type<radahn::core::SimUnits>::type>(fx.m_unit);
    node["selection"] = selection;
}

void radahn::core::LammpsCommandsUtils::registerAddTorqueCommandToConduit(conduit::Node& node, const std::string& name, TorqueQuantity tx, TorqueQuantity ty, TorqueQuantity tz, const std::vector<atomIndexes_t>& selection)
{
    // Heavy syntax still required for c++20
    // for c++23, prefer using std::to_underlying
    node["cmdType"] = static_cast<std::underlying_type<radahn::core::SimCommandType>::type>(radahn::core::SimCommandType::SIM_COMMAND_LMP_ADD_TORQUE);
    node["origin"] = name;
    node["tx"] = tx.m_value;   // Need to send units as well
    node["ty"] = ty.m_value;
    node["tz"] = tz.m_value;
    node["tunits"] = static_cast<std::underlying_type<radahn::core::SimUnits>::type>(tx.m_unit);
    node["selection"] = selection;
}

void radahn::core::LammpsCommandsUtils::registerRotateCommandToConduit(
        conduit::Node& node, 
        const std::string& name, 
        DistanceQuantity px, 
        DistanceQuantity py, 
        DistanceQuantity pz, 
        atomPositions_t ax,     
        atomPositions_t ay,
        atomPositions_t az,
        TimeQuantity period,     
        const std::vector<atomIndexes_t>& selection)
{
    // Heavy syntax still required for c++20
    // for c++23, prefer using std::to_underlying
    node["cmdType"] = static_cast<std::underlying_type<radahn::core::SimCommandType>::type>(radahn::core::SimCommandType::SIM_COMMAND_LMP_ROTATE);
    node["origin"] = name;
    node["px"] = px.m_value;   // Need to send units as well
    node["py"] = py.m_value;
    node["pz"] = pz.m_value;
    node["punits"] = static_cast<std::underlying_type<radahn::core::SimUnits>::type>(px.m_unit);
    node["ax"] = ax;   // Need to send units as well
    node["ay"] = ay;
    node["az"] = az;
    node["period"] = period.m_value;
    node["periodunits"] = static_cast<std::underlying_type<radahn::core::SimUnits>::type>(period.m_unit);
    node["selection"] = selection;
}

void radahn::core::LammpsCommandsUtils::registerWaitCommandToConduit(conduit::Node& node, const std::string& name)
{
    node["cmdType"] = static_cast<std::underlying_type<radahn::core::SimCommandType>::type>(radahn::core::SimCommandType::SIM_COMMAND_WAIT);
    node["origin"] = name;
}
