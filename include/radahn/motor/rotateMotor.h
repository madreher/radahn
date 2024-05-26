#pragma once 

#include <radahn/motor/motor.h>
#include <radahn/core/atomSet.h>
#include <radahn/core/units.h>
#include <radahn/lmp/lammpsCommandsUtils.h>

namespace radahn {

namespace motor {

class RotateMotor : public Motor 
{
public:
    RotateMotor() : Motor(){}
    RotateMotor(const std::string& name, const std::set<radahn::core::atomIndexes_t>& selection, 
        radahn::core::DistanceQuantity px = radahn::core::DistanceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::DistanceQuantity py = radahn::core::DistanceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::DistanceQuantity pz = radahn::core::DistanceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL),
        radahn::core::atomPositions_t ax = 0.0, radahn::core::atomPositions_t ay = 0.0, radahn::core::atomPositions_t az = 0.0,
        radahn::core::TimeQuantity period = radahn::core::TimeQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL),
        double requestedAngle = 0.0
        ) : 
        Motor(name),
        m_currentState(selection), 
        m_px(px), m_py(py), m_pz(pz),
        m_ax(ax), m_ay(ay), m_az(az),
        m_period(period), m_requestedAngle(requestedAngle){}
    virtual ~RotateMotor(){}

    virtual bool updateState(radahn::core::simIt_t it, 
        const std::vector<radahn::core::atomIndexes_t>& indices, 
        const std::vector<radahn::core::atomPositions_t>& positions,
        conduit::Node& kvs) override
        {
            if(m_status != MotorStatus::MOTOR_RUNNING)
                return false;

            kvs["progress"] = 0.0;

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
        // Heavy syntax still required for c++20
        // for c++23, prefer using std::to_underlying
        radahn::lmp::LammpsCommandsUtils::registerRotateCommandToConduit(node, m_name, m_px, m_py, m_pz, m_ax, m_ay, m_az, m_period, m_currentState.getSelectionVector());

        return true;
    }

protected:
    radahn::core::AtomSet m_currentState;
    radahn::core::DistanceQuantity m_px;      // 3D point of the axe
    radahn::core::DistanceQuantity m_py;
    radahn::core::DistanceQuantity m_pz;
    radahn::core::atomPositions_t m_ax;       // Axe vector
    radahn::core::atomPositions_t m_ay;
    radahn::core::atomPositions_t m_az;
    radahn::core::TimeQuantity m_period;      // Period of a rotation
    double m_requestedAngle;
    double m_currentAngle = 0.0;
    bool m_initialStateRegistered   = false;
    radahn::core::AtomSet m_initialState;
};    

} // core

} // radahn