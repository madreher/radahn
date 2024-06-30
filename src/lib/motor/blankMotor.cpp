#include <radahn/motor/blankMotor.h>
#include <radahn/lmp/lammpsCommandsUtils.h>

#include <spdlog/spdlog.h>

using namespace radahn::core;

bool radahn::motor::BlankMotor::updateState(simIt_t it, 
        const std::vector<atomIndexes_t>& indices, 
        const std::vector<atomPositions_t>& positions,
        conduit::Node& kvs)
{
    (void)indices;
    (void)positions;

    if(m_status != MotorStatus::MOTOR_RUNNING)
        return false;

    if(m_stepCountersSet)
    {
        kvs["steps_left"] = it - m_lastStep;
        kvs["steps_done"] = it - m_startStep;
        kvs["progress"] = (static_cast<double>(it - m_startStep) / static_cast<double>(m_nbStepsRequested))*100.0;

        if(it >= m_lastStep)
        {
            spdlog::info("Motor {} completed successfully.", m_name);
            m_status = MotorStatus::MOTOR_SUCCESS;
        }        
        return true;
    }
    else 
    {
        kvs["steps_left"] = m_nbStepsRequested;
        kvs["steps_done"] = 0;
        kvs["progress"] = 0.0;
        m_startStep = it;
        m_lastStep = it + m_nbStepsRequested;
        m_stepCountersSet = true;
    }
    return true;
}

bool radahn::motor::BlankMotor::appendCommandToConduitNode(conduit::Node& node)
{
    radahn::lmp::LammpsCommandsUtils::registerWaitCommandToConduit(node, m_name);
    return true;
}

bool radahn::motor::BlankMotor::loadFromJSON(const nlohmann::json& node, uint32_t version, radahn::core::SimUnits units)
{
    (void)version;
    (void)units;
    
    if(!node.contains("name"))
    {
        spdlog::error("TName not found while trying to load a BlankMotor from json.");
        return false;
    }
    m_name = node["name"].get<std::string>();

    if(!node.contains("nbStepsRequested"))
    {
        spdlog::error("nbStepsRequested not found while trying to load a BlankMotor from json.");
        return false;
    }
    m_nbStepsRequested = node["nbStepsRequested"].get<radahn::core::simIt_t>();
    if(m_nbStepsRequested <= 0)
    {
        spdlog::error("nbStepsRequested must be > 0 while trying to load a BlankMotor from json.");
        return false;
    }

    return true;
}

void radahn::motor::BlankMotor::convertSettingsTo(SimUnits destUnits)
{
    (void)destUnits;
}