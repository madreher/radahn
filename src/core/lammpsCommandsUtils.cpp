#include <radahn/core/lammpsCommandsUtils.h>

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