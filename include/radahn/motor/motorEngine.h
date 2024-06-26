#pragma once 

#include <radahn/motor/motor.h>
#include <radahn/core/types.h>

#include <conduit/conduit.hpp>

#include <vector>
#include <list>
#include <memory>

namespace radahn {

namespace motor {

class MotorEngine 
{

public:
    MotorEngine(){}

    void loadTestMotorSetup();

    bool updateMotorsState(radahn::core::simIt_t it,
        std::vector<radahn::core::atomIndexes_t>& indices, 
        std::vector<radahn::core::atomPositions_t>& positions);

    bool getCommandsFromMotors(conduit::Node& node) const;
    bool updateMotorLists();

    bool isCompleted() const;
    void clearMotors();

    const conduit::Node& getCurrentKVS() const { return m_currentKVS; }
    conduit::Node& getCurrentKVS() { return m_currentKVS; }

    const std::vector<radahn::core::atomPositions_t>& getCurrentPositions() { return m_currentPositions; }
    radahn::core::simIt_t getCurrentIt() { return m_currentIt; }

    bool loadFromJSON(const std::string& filename);

    void convertMotorsTo(radahn::core::SimUnits destUnits);

protected:
    //std::vector<std::shared_ptr<radahn::motor::Motor>> m_motors;
    std::unordered_map<std::string, std::shared_ptr<radahn::motor::Motor>> m_motorsMap;
    std::list<std::shared_ptr<radahn::motor::Motor>> m_pendingMotors;
    std::list<std::shared_ptr<radahn::motor::Motor>> m_activeMotors;

    radahn::core::simIt_t m_currentIt;
    std::vector<radahn::core::atomIndexes_t> m_currentIndexes;
    std::vector<radahn::core::atomPositions_t> m_currentPositions;

    conduit::Node m_currentKVS;  // Data which can be used for plotting
    radahn::core::SimUnits m_currentUnits = radahn::core::SimUnits::LAMMPS_REAL;


};

} // motor

} // radahn