#pragma once 

#include <radahn/core/motor.h>
#include <radahn/core/types.h>

#include <conduit/conduit.hpp>

#include <vector>
#include <memory>

namespace radahn {

namespace motor {

class MotorEngine 
{

public:
    MotorEngine(){}

    void loadTestMotorSetup();

    bool updateMotorsState(radahn::core::simIt_t it,
        uint64_t nbAtoms,
        radahn::core::atomIndexes_t* indices, 
        radahn::core::atomPositions_t*positions);

    bool getCommandsFromMotors(conduit::Node& node) const;
    bool isCompleted() const;

protected:
    std::vector<std::shared_ptr<radahn::core::Motor>> m_motors;

    radahn::core::simIt_t m_currentIt;
    std::vector<radahn::core::atomIndexes_t> m_currentIndexes;
    std::vector<radahn::core::atomPositions_t> m_currentPositions;


};

} // motor

} // radahn