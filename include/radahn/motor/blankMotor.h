#pragma once 

#include <radahn/motor/motor.h>
#include <set>
#include <ranges>
#include <algorithm>
#include <span>

namespace radahn {

namespace motor {


class BlankMotor : public Motor 
{
public:
    BlankMotor() : Motor(){}
    BlankMotor(const std::string& name, radahn::core::simIt_t nbStepsRequested) : Motor(name), m_nbStepsRequested(nbStepsRequested), m_startStep(0), m_lastStep(0), m_stepCountersSet(false){}

    virtual bool updateState(radahn::core::simIt_t it, 
        const std::vector<radahn::core::atomIndexes_t>& indices, 
        const std::vector<radahn::core::atomPositions_t>& positions) override;
        
    virtual bool appendCommandToConduitNode(conduit::Node& node) override;

protected:
    radahn::core::simIt_t m_nbStepsRequested = 0;
    radahn::core::simIt_t m_startStep = 0;
    radahn::core::simIt_t m_lastStep = 0;
    bool m_stepCountersSet = false;
    std::set<radahn::core::atomIndexes_t> m_selection = {1, 2, 3};

};

}

}