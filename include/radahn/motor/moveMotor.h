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
        const std::vector<radahn::core::atomPositions_t>& positions,
        conduit::Node& kvs) override;
        
    virtual bool appendCommandToConduitNode(conduit::Node& node) override;

    virtual bool loadFromJSON(const nlohmann::json& node, uint32_t version, radahn::core::SimUnits units) override;

    virtual void convertSettingsTo(radahn::core::SimUnits destUnits) override;

protected:
    // Settings variables
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

    // Internal computation variables
    bool m_initialStateRegistered   = false;
    radahn::core::DistanceQuantity m_initialCx;          // initial center
    radahn::core::DistanceQuantity m_initialCy;
    radahn::core::DistanceQuantity m_initialCz;
    radahn::core::DistanceQuantity m_initialDistance;
    radahn::core::AtomSet m_initialState;
};

} // core

} // radahn