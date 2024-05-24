#include <radahn/motor/blankMotor.h>
#include <radahn/lmp/lammpsCommandsUtils.h>

#include <spdlog/spdlog.h>

using namespace radahn::core;

bool radahn::motor::BlankMotor::updateState(simIt_t it, 
        const std::vector<atomIndexes_t>& indices, 
        const std::vector<atomPositions_t>& positions)
{
    (void)indices;
    (void)positions;

    if(m_status != MotorStatus::MOTOR_RUNNING)
        return false;

    if(m_stepCountersSet)
    {
        if(it >= m_lastStep)
        {
            spdlog::info("Motor {} completed successfully.", m_name);
            m_status = MotorStatus::MOTOR_SUCCESS;
        }        
        return true;
    }
    else 
    {
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