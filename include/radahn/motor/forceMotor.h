#pragma once 

#include <radahn/motor/motor.h>
#include <radahn/core/atomSet.h>
#include <radahn/core/units.h>
#include <radahn/lmp/lammpsCommandsUtils.h>

namespace radahn {

namespace motor {


class ForceMotor : public Motor
{
public:
    ForceMotor() : Motor(){}
    ForceMotor(const std::string& name, const std::set<radahn::core::atomIndexes_t>& selection, 
        radahn::core::ForceQuantity fx = radahn::core::ForceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::ForceQuantity fy = radahn::core::ForceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::ForceQuantity fz = radahn::core::ForceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL),
        bool checkX = false, bool checkY = false, bool checkZ = false,
        radahn::core::DistanceQuantity dx = radahn::core::DistanceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::DistanceQuantity dy = radahn::core::DistanceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::DistanceQuantity dz = radahn::core::DistanceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL)
        ) : 
        Motor(name),
        m_currentState(selection), 
        m_fx(fx), m_fy(fy), m_fz(fz),
        m_checkX(checkX), m_checkY(checkY), m_checkZ(checkZ),
        m_dx(dx), m_dy(dy), m_dz(dz){}
    virtual ~ForceMotor(){}

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
    radahn::core::ForceQuantity m_fx;
    radahn::core::ForceQuantity m_fy;
    radahn::core::ForceQuantity m_fz;
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
    radahn::core::AtomSet m_initialState;
};

} // core

} // radahn