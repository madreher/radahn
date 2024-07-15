#pragma once 

#include <radahn/motor/motor.h>
#include <radahn/core/types.h>
#include <radahn/core/CSVWriter.h>

#include <conduit/conduit.hpp>

#include <vector>
#include <list>
#include <memory>

namespace radahn {

namespace motor {

class MotorEngine 
{

public:
    MotorEngine() : m_globalCSV("global", ';')
    {
        // Must match the thermo input in lammpsDriver.cpp, function extractAtomInformation
        m_globalCSV.declareFieldNames({"simIt", "temp", "tot", "pot", "kin", "dt", "sim_t"});
    }

    void loadTestMotorSetup();

    void setCurrentSimulationIt(radahn::core::simIt_t it);
    void updateEngineState(radahn::core::simIt_t it,
        std::vector<radahn::core::atomIndexes_t>& indices, 
        std::vector<radahn::core::atomPositions_t>& positions);
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

    void addGlobalKVS(conduit::Node& globals);
    void commitKVSFrame();
    void saveKVSToCSV();

protected:
    //std::vector<std::shared_ptr<radahn::motor::Motor>> m_motors;
    std::unordered_map<std::string, std::shared_ptr<radahn::motor::Motor>> m_motorsMap;
    std::list<std::shared_ptr<radahn::motor::Motor>> m_pendingMotors;
    std::list<std::shared_ptr<radahn::motor::Motor>> m_activeMotors;

    radahn::core::simIt_t m_currentIt;
    std::vector<radahn::core::atomIndexes_t> m_currentIndexes;
    std::vector<radahn::core::atomPositions_t> m_currentPositions;

    conduit::Node m_currentKVS;  // Data which can be used for plotting
    radahn::core::CSVWriter m_globalCSV;
    radahn::core::SimUnits m_currentUnits = radahn::core::SimUnits::LAMMPS_REAL;

    
};

} // motor

} // radahn