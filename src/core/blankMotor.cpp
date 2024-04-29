#include <radahn/core/blankMotor.h>
#include <radahn/core/simCommands.h>

using namespace radahn::core;

bool radahn::core::BlankMotor::updateState(simIt_t it)
{
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
    conduit::Node& cmd = node.append();
    cmd["cmdType"] = static_cast<uint32_t>(SimCommandType::SIM_COMMAND_WAIT);
    return true;
}