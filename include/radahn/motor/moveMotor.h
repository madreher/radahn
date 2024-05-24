#pragma once 

#include <spdlog/spdlog.h>

#include <radahn/motor/motor.h>
#include <radahn/core/atomSet.h>
#include <radahn/core/units.h>
#include <radahn/lmp/lammpsCommandsUtils.h>

namespace radahn {

namespace motor {


class MoveMotor : public Motor
{
public:
    MoveMotor() : Motor(){}
    MoveMotor(const std::string& name, const std::set<radahn::core::atomIndexes_t>& selection, 
        radahn::core::VelocityQuantity vx = radahn::core::VelocityQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::VelocityQuantity vy = radahn::core::VelocityQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::VelocityQuantity vz = radahn::core::VelocityQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL),
        bool checkX = false, bool checkY = false, bool checkZ = false,
        radahn::core::DistanceQuantity dx = radahn::core::DistanceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::DistanceQuantity dy = radahn::core::DistanceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::DistanceQuantity dz = radahn::core::DistanceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL)
        ) : 
        Motor(name),
        m_currentState(selection), 
        m_vx(vx), m_vy(vy), m_vz(vz),
        m_checkX(checkX), m_checkY(checkY), m_checkZ(checkZ),
        m_dx(dx), m_dy(dy), m_dz(dz){}
    virtual ~MoveMotor(){}

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
                auto initialCenter =  m_currentState.computePositionCenter();
                m_initialCx = radahn::core::DistanceQuantity(initialCenter[0], radahn::core::SimUnits::LAMMPS_REAL);
                m_initialCy = radahn::core::DistanceQuantity(initialCenter[1], radahn::core::SimUnits::LAMMPS_REAL);
                m_initialCz = radahn::core::DistanceQuantity(initialCenter[2], radahn::core::SimUnits::LAMMPS_REAL);
                m_initialState = radahn::core::AtomSet(m_currentState);
                m_initialStateRegistered = true;
                return true;
            }

            // Check if we have met the conditions
            auto currentCenter = m_currentState.computePositionCenter();

            std::vector<radahn::core::atomPositions_t> distances = {0.0, 0.0, 0.0};
            distances[0] = currentCenter[0]-m_initialCx.m_value;
            distances[1] = currentCenter[1]-m_initialCy.m_value;
            distances[2] = currentCenter[2]-m_initialCz.m_value;
            spdlog::info("Current distance: {} {} {}", distances[0], distances[1], distances[2] );

            bool validX = !m_checkX || (m_dx.m_value < 0.0 && distances[0] <= m_dx.m_value) || (m_dx.m_value >= 0.0 && distances[0] >= m_dx.m_value);
            bool validY = !m_checkY || (m_dy.m_value < 0.0 && distances[1] <= m_dy.m_value) || (m_dy.m_value >= 0.0 && distances[1] >= m_dy.m_value);
            bool validZ = !m_checkZ || (m_dz.m_value < 0.0 && distances[2] <= m_dz.m_value) || (m_dz.m_value >= 0.0 && distances[2] >= m_dz.m_value);

            if (validX && validY && validZ)
            {
                spdlog::info("Motor {} completed successfully.", m_name);
                m_status = MotorStatus::MOTOR_SUCCESS;
            }

            return true;
        }
        
    virtual bool appendCommandToConduitNode(conduit::Node& node) override
    {
        // Heavy syntax still required for c++20
        // for c++23, prefer using std::to_underlying
        radahn::lmp::LammpsCommandsUtils::registerMoveCommandToConduit(node, m_name, m_vx, m_vy, m_vz, m_currentState.getSelectionVector());

        return true;
    }

protected:
    radahn::core::AtomSet m_currentState;
    radahn::core::VelocityQuantity m_vx;
    radahn::core::VelocityQuantity m_vy;
    radahn::core::VelocityQuantity m_vz;
    bool m_checkX                   = false;
    bool m_checkY                   = false;
    bool m_checkZ                   = false;
    radahn::core::DistanceQuantity m_dx;
    radahn::core::DistanceQuantity m_dy;
    radahn::core::DistanceQuantity m_dz;
    bool m_initialStateRegistered   = false;
    radahn::core::DistanceQuantity m_initialCx;          // initial center
    radahn::core::DistanceQuantity m_initialCy;
    radahn::core::DistanceQuantity m_initialCz;
    radahn::core::AtomSet m_initialState;
};

} // core

} // radahn