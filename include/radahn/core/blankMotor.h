#pragma once 

#include <radahn/core/motor.h>
#include <set>
#include <ranges>
#include <algorithm>
#include <span>

namespace radahn {

namespace core {


class BlankMotor : public Motor 
{
public:
    BlankMotor() : Motor(){}
    BlankMotor(const std::string& name, radahn::core::simIt_t nbStepsRequested) : Motor(name), m_nbStepsRequested(nbStepsRequested), m_startStep(0), m_lastStep(0), m_stepCountersSet(false){}

    virtual bool updateState(simIt_t it, 
        uint64_t nbAtoms,
        atomIndexes_t* indices, 
        atomPositions_t* positions) override;
        
    virtual bool appendCommandToConduitNode(conduit::Node& node) override;

protected:
    radahn::core::simIt_t m_nbStepsRequested = 0;
    radahn::core::simIt_t m_startStep = 0;
    radahn::core::simIt_t m_lastStep = 0;
    bool m_stepCountersSet = false;
    std::set<atomIndexes_t> m_selection = {1, 2, 3};

};

}

}