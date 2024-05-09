#pragma once 

#include <radahn/core/motor.h>
#include <radahn/core/atomSet.h>
#include <radahn/core/units.h>
#include <radahn/core/lammpsCommandsUtils.h>

namespace radahn {

namespace core {


class MoveMotor : public Motor
{
public:
    MoveMotor() : Motor(){}
    MoveMotor(const std::string& name, const std::set<atomIndexes_t>& selection, 
        VelocityQuantity vx = VelocityQuantity(0.0, SimUnits::LAMMPS_REAL), VelocityQuantity vy = VelocityQuantity(0.0, SimUnits::LAMMPS_REAL), VelocityQuantity vz = VelocityQuantity(0.0, SimUnits::LAMMPS_REAL),
        bool checkX = false, bool checkY = false, bool checkZ = false,
        DistanceQuantity dx = DistanceQuantity(0.0, SimUnits::LAMMPS_REAL), DistanceQuantity dy = DistanceQuantity(0.0, SimUnits::LAMMPS_REAL), DistanceQuantity dz = DistanceQuantity(0.0, SimUnits::LAMMPS_REAL)
        ) : 
        Motor(name),
        m_currentState(selection), 
        m_vx(vx), m_vy(vy), m_vz(vz),
        m_checkX(checkX), m_checkY(checkY), m_checkZ(checkZ),
        m_dx(dx), m_dy(dy), m_dz(dz){}
    virtual ~MoveMotor(){}

    virtual bool updateState(simIt_t it, 
        uint64_t nbAtoms,
        atomIndexes_t* indices, 
        atomPositions_t* positions) override
        {
            if(m_status != MotorStatus::MOTOR_RUNNING)
                return false;

            m_currentState.selectAtoms(it, nbAtoms, indices, positions);
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
        // Heavy syntax still required for c++20
        // for c++23, prefer using std::to_underlying
        LammpsCommandsUtils::registerMoveCommandToConduit(node, m_name, m_vx, m_vy, m_vz, m_currentState.getSelectionVector());

        return true;
    }

protected:
    AtomSet m_currentState;
    VelocityQuantity m_vx;
    VelocityQuantity m_vy;
    VelocityQuantity m_vz;
    bool m_checkX                   = false;
    bool m_checkY                   = false;
    bool m_checkZ                   = false;
    DistanceQuantity m_dx;
    DistanceQuantity m_dy;
    DistanceQuantity m_dz;
    bool m_initialStateRegistered   = false;
    AtomSet m_initialState;
};

} // core

} // radahn