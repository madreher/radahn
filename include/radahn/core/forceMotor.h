#pragma once 

#include <radahn/core/motor.h>
#include <radahn/core/atomSet.h>
#include <radahn/core/units.h>
#include <radahn/core/lammpsCommandsUtils.h>

namespace radahn {

namespace core {


class ForceMotor : public Motor
{
public:
    ForceMotor() : Motor(){}
    ForceMotor(const std::string& name, const std::set<atomIndexes_t>& selection, 
        ForceQuantity fx = ForceQuantity(0.0, SimUnits::LAMMPS_REAL), ForceQuantity fy = ForceQuantity(0.0, SimUnits::LAMMPS_REAL), ForceQuantity fz = ForceQuantity(0.0, SimUnits::LAMMPS_REAL),
        bool checkX = false, bool checkY = false, bool checkZ = false,
        DistanceQuantity dx = DistanceQuantity(0.0, SimUnits::LAMMPS_REAL), DistanceQuantity dy = DistanceQuantity(0.0, SimUnits::LAMMPS_REAL), DistanceQuantity dz = DistanceQuantity(0.0, SimUnits::LAMMPS_REAL)
        ) : 
        Motor(name),
        m_currentState(selection), 
        m_fx(fx), m_fy(fy), m_fz(fz),
        m_checkX(checkX), m_checkY(checkY), m_checkZ(checkZ),
        m_dx(dx), m_dy(dy), m_dz(dz){}
    virtual ~ForceMotor(){}

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
                auto initialCenter =  m_currentState.computePositionCenter();
                m_initialCx = DistanceQuantity(initialCenter[0], radahn::core::SimUnits::LAMMPS_REAL);
                m_initialCy = DistanceQuantity(initialCenter[1], radahn::core::SimUnits::LAMMPS_REAL);
                m_initialCz = DistanceQuantity(initialCenter[2], radahn::core::SimUnits::LAMMPS_REAL);
                m_initialState = AtomSet(m_currentState);
                m_initialStateRegistered = true;
                return true;
            }

            // Check if we have met the conditions
            auto currentCenter = m_currentState.computePositionCenter();

            std::vector<atomPositions_t> distances = {0.0, 0.0, 0.0};
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
        LammpsCommandsUtils::registerAddForceCommandToConduit(node, m_name, m_fx, m_fy, m_fz, m_currentState.getSelectionVector());

        return true;
    }

protected:
    AtomSet m_currentState;
    ForceQuantity m_fx;
    ForceQuantity m_fy;
    ForceQuantity m_fz;
    bool m_checkX                   = false;
    bool m_checkY                   = false;
    bool m_checkZ                   = false;
    DistanceQuantity m_dx;
    DistanceQuantity m_dy;
    DistanceQuantity m_dz;
    bool m_initialStateRegistered   = false;
    DistanceQuantity m_initialCx;          // initial center
    DistanceQuantity m_initialCy;
    DistanceQuantity m_initialCz;
    AtomSet m_initialState;
};

} // core

} // radahn