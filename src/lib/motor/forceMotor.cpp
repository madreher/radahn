#include <radahn/motor/forceMotor.h>

bool radahn::motor::ForceMotor::updateState(radahn::core::simIt_t it, 
    const std::vector<radahn::core::atomIndexes_t>& indices, 
    const std::vector<radahn::core::atomPositions_t>& positions,
    conduit::Node& kvs)
{
    if(m_status != MotorStatus::MOTOR_RUNNING)
        return false;

    kvs["progress"] = 0.0;

    m_currentState.selectAtoms(it, indices, positions);
    if(!m_initialStateRegistered)
    {
        spdlog::info("Registering the initial state for the motor {}.", m_name);
        auto initialCenter =  m_currentState.computePositionCenter();
        m_initialCx = radahn::core::DistanceQuantity(initialCenter[0], radahn::core::SimUnits::LAMMPS_REAL);
        m_initialCy = radahn::core::DistanceQuantity(initialCenter[1], radahn::core::SimUnits::LAMMPS_REAL);
        m_initialCz = radahn::core::DistanceQuantity(initialCenter[2], radahn::core::SimUnits::LAMMPS_REAL);
        m_initialState = radahn::core::AtomSet(m_currentState);
        m_initialStateRegistered = true;

        kvs["distanceX"] = static_cast<radahn::core::atomPositions_t>(0.0);
        kvs["distanceY"] = static_cast<radahn::core::atomPositions_t>(0.0);
        kvs["distanceZ"] = static_cast<radahn::core::atomPositions_t>(0.0);


        m_motorWriter.appendFrame(it, kvs);
        return true;
    }

    // Check if we have met the conditions
    auto currentCenter = m_currentState.computePositionCenter();

    std::vector<radahn::core::atomPositions_t> distances = {0.0, 0.0, 0.0};
    distances[0] = currentCenter[0]-m_initialCx.m_value;
    distances[1] = currentCenter[1]-m_initialCy.m_value;
    distances[2] = currentCenter[2]-m_initialCz.m_value;
    spdlog::info("Current distance: {} {} {}", distances[0], distances[1], distances[2] );
    kvs["distanceX"] = distances[0];
    kvs["distanceY"] = distances[1];
    kvs["distanceZ"] = distances[2];

    bool validX = !m_checkX || (m_dx.m_value < 0.0 && distances[0] <= m_dx.m_value) || (m_dx.m_value >= 0.0 && distances[0] >= m_dx.m_value);
    bool validY = !m_checkY || (m_dy.m_value < 0.0 && distances[1] <= m_dy.m_value) || (m_dy.m_value >= 0.0 && distances[1] >= m_dy.m_value);
    bool validZ = !m_checkZ || (m_dz.m_value < 0.0 && distances[2] <= m_dz.m_value) || (m_dz.m_value >= 0.0 && distances[2] >= m_dz.m_value);

    if (validX && validY && validZ)
    {
        spdlog::info("Motor {} completed successfully.", m_name);
        m_status = MotorStatus::MOTOR_SUCCESS;
    }

    m_motorWriter.appendFrame(it, kvs);

    return true;
}

void radahn::motor::ForceMotor::declareCSVWriterFieldNames()
{
    m_motorWriter.declareFieldNames({"distanceX", "distanceY", "distanceZ", "progress"});
}
    
bool radahn::motor::ForceMotor::appendCommandToConduitNode(conduit::Node& node)
{
    radahn::lmp::LammpsCommandsUtils::registerAddForceCommandToConduit(node, m_name, m_fx, m_fy, m_fz, m_currentState.getSelectionVector());

    return true;
}

bool radahn::motor::ForceMotor::loadFromJSON(const nlohmann::json& node, uint32_t version, radahn::core::SimUnits units)
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
    
    m_fx = radahn::core::ForceQuantity(node.value("fx", 0.0), units);
    m_fy = radahn::core::ForceQuantity(node.value("fy", 0.0), units);
    m_fz = radahn::core::ForceQuantity(node.value("fz", 0.0), units);
    m_checkX = node.value("checkX", false);
    m_checkY = node.value("checkY", false);
    m_checkZ = node.value("checkZ", false);
    m_dx = radahn::core::DistanceQuantity(node.value("dx", 0.0), units);
    m_dy = radahn::core::DistanceQuantity(node.value("dy", 0.0), units);
    m_dz = radahn::core::DistanceQuantity(node.value("dz", 0.0), units);
    return true;
}

void radahn::motor::ForceMotor::convertSettingsTo(radahn::core::SimUnits destUnits)
{
    m_fx.convertTo(destUnits);
    m_fy.convertTo(destUnits);
    m_fz.convertTo(destUnits);

    m_dx.convertTo(destUnits);
    m_dy.convertTo(destUnits);
    m_dz.convertTo(destUnits);

    m_initialCx.convertTo(destUnits);
    m_initialCy.convertTo(destUnits);
    m_initialCz.convertTo(destUnits);
}