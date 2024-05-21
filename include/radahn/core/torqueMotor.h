#pragma once 

#include <radahn/core/motor.h>
#include <radahn/core/atomSet.h>
#include <radahn/core/units.h>
#include <radahn/core/lammpsCommandsUtils.h>

namespace radahn {

namespace core {


class TorqueMotor : public Motor
{
public:
    TorqueMotor() : Motor(){}
    TorqueMotor(const std::string& name, const std::set<atomIndexes_t>& selection, 
        TorqueQuantity tx = TorqueQuantity(0.0, SimUnits::LAMMPS_REAL), TorqueQuantity ty = TorqueQuantity(0.0, SimUnits::LAMMPS_REAL), TorqueQuantity tz = TorqueQuantity(0.0, SimUnits::LAMMPS_REAL),
        double requestedAngle = 0.0
        ) : 
        Motor(name),
        m_currentState(selection), 
        m_tx(tx), m_ty(ty), m_tz(tz),
        m_requestedAngle(requestedAngle){}
    virtual ~TorqueMotor(){}

    virtual bool updateState(simIt_t it, 
        const std::vector<atomIndexes_t>& indices, 
        const std::vector<atomPositions_t>& positions) override
        {
            if(m_status != MotorStatus::MOTOR_RUNNING)
                return false;

            m_currentState.selectAtoms(it, indices, positions);
            if(!m_initialStateRegistered)
            {
                spdlog::info("Registering the initial state for the motor {}.", m_name);
                m_initialState = AtomSet(m_currentState);
                return true;
            }

            // Check if we have met the conditions

            return true;
        }
        
    virtual bool appendCommandToConduitNode(conduit::Node& node) override
    {
        LammpsCommandsUtils::registerAddTorqueCommandToConduit(node, m_name, m_tx, m_ty, m_tz, m_currentState.getSelectionVector());

        return true;
    }

protected:
    AtomSet m_currentState;
    TorqueQuantity m_tx;
    TorqueQuantity m_ty;
    TorqueQuantity m_tz;
    bool m_initialStateRegistered   = false;
    AtomSet m_initialState;
    double m_requestedAngle;
    double m_currentAngle = 0.0;
};

} // core

} // radahn