#include <radahn/core/blankMotor.h>
#include <radahn/core/lammpsCommandsUtils.h>

using namespace radahn::core;

bool radahn::core::BlankMotor::updateState(simIt_t it, 
        const std::vector<atomIndexes_t>& indices, 
        const std::vector<atomPositions_t>& positions)
{
    (void)indices;
    (void)positions;

    if(m_status != MotorStatus::MOTOR_RUNNING)
        return false;

    if(m_stepCountersSet)
        return it < m_lastStep;
    else 
    {
        m_startStep = it;
        m_lastStep = it + m_nbStepsRequested;
        m_stepCountersSet = true;
    }
    return true;
}

bool radahn::core::BlankMotor::appendCommandToConduitNode(conduit::Node& node)
{
    LammpsCommandsUtils::registerWaitCommandToConduit(node, m_name);
    return true;
}