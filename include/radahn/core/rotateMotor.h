#pragma once 

#include <radahn/core/motor.h>
#include <radahn/core/atomSet.h>
#include <radahn/core/units.h>
#include <radahn/core/lammpsCommandsUtils.h>

namespace radahn {

namespace core {

class RotateMotor : public Motor 
{
public:
    RotateMotor() : Motor(){}
    RotateMotor(const std::string& name, const std::set<atomIndexes_t>& selection, 
        DistanceQuantity px = DistanceQuantity(0.0, SimUnits::LAMMPS_REAL), DistanceQuantity py = DistanceQuantity(0.0, SimUnits::LAMMPS_REAL), DistanceQuantity pz = DistanceQuantity(0.0, SimUnits::LAMMPS_REAL),
        atomPositions_t ax = 0.0, atomPositions_t ay = 0.0, atomPositions_t az = 0.0,
        TimeQuantity period = TimeQuantity(0.0, SimUnits::LAMMPS_REAL),
        double requestedAngle = 0.0
        ) : 
        Motor(name),
        m_currentState(selection), 
        m_px(px), m_py(py), m_pz(pz),
        m_ax(ax), m_ay(ay), m_az(az),
        m_period(period), m_requestedAngle(requestedAngle){}
    virtual ~RotateMotor(){}

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
        LammpsCommandsUtils::registerRotateCommandToConduit(node, m_name, m_px, m_py, m_pz, m_ax, m_ay, m_az, m_period, m_currentState.getSelectionVector());

        return true;
    }

protected:
    AtomSet m_currentState;
    DistanceQuantity m_px;      // 3D point of the axe
    DistanceQuantity m_py;
    DistanceQuantity m_pz;
    atomPositions_t m_ax;       // Axe vector
    atomPositions_t m_ay;
    atomPositions_t m_az;
    TimeQuantity m_period;      // Period of a rotation
    double m_requestedAngle;
    double m_currentAngle = 0.0;
    bool m_initialStateRegistered   = false;
    AtomSet m_initialState;
};    

} // core

} // radahn