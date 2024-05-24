#pragma once 

#include <radahn/motor/motor.h>
#include <radahn/core/atomSet.h>
#include <radahn/core/units.h>
#include <radahn/lmp/lammpsCommandsUtils.h>

namespace radahn {

namespace motor {


class TorqueMotor : public Motor
{
public:
    TorqueMotor() : Motor(){}
    TorqueMotor(const std::string& name, const std::set<radahn::core::atomIndexes_t>& selection, 
        radahn::core::TorqueQuantity tx = radahn::core::TorqueQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::TorqueQuantity ty = radahn::core::TorqueQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::TorqueQuantity tz = radahn::core::TorqueQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL),
        double requestedAngle = 0.0
        ) : 
        Motor(name),
        m_currentState(selection), 
        m_tx(tx), m_ty(ty), m_tz(tz),
        m_requestedAngle(requestedAngle){}
    virtual ~TorqueMotor(){}

    virtual bool updateState(radahn::core::simIt_t it, 
        const std::vector<radahn::core::atomIndexes_t>& indices, 
        const std::vector<radahn::core::atomPositions_t>& positions) override
        {
            if(m_status != MotorStatus::MOTOR_RUNNING)
                return false;

            m_currentState.selectAtoms(it, indices, positions);
            if(!m_initialStateRegistered)
            {
                spdlog::info("Registering the initial state for the motor {}.", m_name);
                m_initialState = radahn::core::AtomSet(m_currentState);
                return true;
            }

            // Check if we have met the conditions

            return true;
        }
        
    virtual bool appendCommandToConduitNode(conduit::Node& node) override
    {
        radahn::lmp::LammpsCommandsUtils::registerAddTorqueCommandToConduit(node, m_name, m_tx, m_ty, m_tz, m_currentState.getSelectionVector());

        return true;
    }

protected:
    radahn::core::AtomSet m_currentState;
    radahn::core::TorqueQuantity m_tx;
    radahn::core::TorqueQuantity m_ty;
    radahn::core::TorqueQuantity m_tz;
    bool m_initialStateRegistered   = false;
    radahn::core::AtomSet m_initialState;
    double m_requestedAngle;
    double m_currentAngle = 0.0;
};

} // core

} // radahn