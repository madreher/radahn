#include <radahn/motor/moveMotor.h>

bool radahn::motor::MoveMotor::updateState(radahn::core::simIt_t it, 
const std::vector<radahn::core::atomIndexes_t>& indices, 
const std::vector<radahn::core::atomPositions_t>& positions,
conduit::Node& kvs)
{
    if(m_status != MotorStatus::MOTOR_RUNNING)
        return false;


    m_currentState.selectAtoms(it, indices, positions);
    if(!m_initialStateRegistered)
    {
        spdlog::info("Registering the initial state for the motor {}.", m_name);
        auto initialCenter =  m_currentState.computePositionCenter();
        m_initialCx = radahn::core::DistanceQuantity(initialCenter[0], radahn::core::SimUnits::LAMMPS_REAL);
        m_initialCy = radahn::core::DistanceQuantity(initialCenter[1], radahn::core::SimUnits::LAMMPS_REAL);
        m_initialCz = radahn::core::DistanceQuantity(initialCenter[2], radahn::core::SimUnits::LAMMPS_REAL);
        m_initialState = radahn::core::AtomSet(m_currentState);
        kvs["progress"] = 0.0;
        kvs["distance_x"] = 0.0;
        kvs["distance_y"] = 0.0;
        kvs["distance_z"] = 0.0;
        kvs["center_x"] = m_initialCx.m_value;
        kvs["center_y"] = m_initialCy.m_value;
        kvs["center_z"] = m_initialCz.m_value;
        m_initialStateRegistered = true;
        return true;
    }

    // Check if we have met the conditions
    auto currentCenter = m_currentState.computePositionCenter();

    std::vector<radahn::core::atomPositions_t> distances = {0.0, 0.0, 0.0};
    distances[0] = currentCenter[0]-m_initialCx.m_value;    // Distance done since the start
    distances[1] = currentCenter[1]-m_initialCy.m_value;
    distances[2] = currentCenter[2]-m_initialCz.m_value;
    spdlog::info("Current distance: {} {} {}", distances[0], distances[1], distances[2] );

    kvs["distance_x"] = distances[0];
    kvs["distance_y"] = distances[1];
    kvs["distance_z"] = distances[2];
    kvs["center_x"] = m_initialCx.m_value;
    kvs["center_y"] = m_initialCy.m_value;
    kvs["center_z"] = m_initialCz.m_value;

    // Progress calculation
    double progress = 0.0;
    double totalProgress = 0.0;
    if(m_checkX)
    {
        totalProgress += 100.0;
        progress += (distances[0] / m_dx.m_value) * 100.0; // Can be negative if the distance is in the other direction than the requested distance
    }
    if(m_checkY)
    {
        totalProgress += 100.0;
        progress += (distances[1] / m_dy.m_value) * 100.0; 
    }
    if(m_checkZ)
    {
        totalProgress += 100.0;
        progress += (distances[2] / m_dz.m_value) * 100.0; 
    }
    
    kvs["progress"] = (progress / totalProgress) * 100.0;

    bool validX = !m_checkX || (m_dx.m_value < 0.0 && distances[0] <= m_dx.m_value) || (m_dx.m_value >= 0.0 && distances[0] >= m_dx.m_value);
    bool validY = !m_checkY || (m_dy.m_value < 0.0 && distances[1] <= m_dy.m_value) || (m_dy.m_value >= 0.0 && distances[1] >= m_dy.m_value);
    bool validZ = !m_checkZ || (m_dz.m_value < 0.0 && distances[2] <= m_dz.m_value) || (m_dz.m_value >= 0.0 && distances[2] >= m_dz.m_value);

    if (validX && validY && validZ)
    {
        spdlog::info("Motor {} completed successfully.", m_name);
        m_status = MotorStatus::MOTOR_SUCCESS;
    }

    return true;
}
    
bool radahn::motor::MoveMotor::appendCommandToConduitNode(conduit::Node& node)
{
    radahn::lmp::LammpsCommandsUtils::registerMoveCommandToConduit(node, m_name, m_vx, m_vy, m_vz, m_currentState.getSelectionVector());

    return true;
}

bool radahn::motor::MoveMotor::loadFromJSON(const nlohmann::json& node, uint32_t version, radahn::core::SimUnits units)
{
    if(!Motor::loadFromJSON(node, version, units))
    {
        return false;
    }
    
    if(!node.contains("selection"))
    {
        spdlog::error("Selection not found while trying to load the ForceMotor {} from json.", m_name);
        return false;
    }
    m_currentState = radahn::core::AtomSet(node["selection"].get<std::set<radahn::core::atomIndexes_t>>());

    m_vx = radahn::core::VelocityQuantity(node.value("vx", 0.0), units);
    m_vy = radahn::core::VelocityQuantity(node.value("vy", 0.0), units);
    m_vz = radahn::core::VelocityQuantity(node.value("vz", 0.0), units);

    m_checkX = node.value("checkX", false);
    m_checkY = node.value("checkY", false);
    m_checkZ = node.value("checkZ", false);
    m_dx = radahn::core::DistanceQuantity(node.value("dx", 0.0), units);
    m_dy = radahn::core::DistanceQuantity(node.value("dy", 0.0), units);
    m_dz = radahn::core::DistanceQuantity(node.value("dz", 0.0), units);
    return true;
}

void radahn::motor::MoveMotor::convertSettingsTo(radahn::core::SimUnits destUnits)
{
    m_vx.convertTo(destUnits);
    m_vy.convertTo(destUnits);
    m_vz.convertTo(destUnits);

    m_dx.convertTo(destUnits);
    m_dy.convertTo(destUnits);
    m_dz.convertTo(destUnits);

    m_initialCx.convertTo(destUnits);
    m_initialCy.convertTo(destUnits);
    m_initialCz.convertTo(destUnits);
}