#include <radahn/core/blankMotor.h>
#include <radahn/core/lammpsCommandsUtils.h>

using namespace radahn::core;

bool radahn::core::BlankMotor::updateState(simIt_t it, 
        uint64_t nbAtoms,
        atomIndexes_t* indices, 
        atomPositions_t*positions)
{
    (void)nbAtoms;
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