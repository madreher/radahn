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
        const std::vector<radahn::core::atomPositions_t>& positions,
        conduit::Node& kvs) override;
        
    virtual bool appendCommandToConduitNode(conduit::Node& node) override;
    virtual bool loadFromJSON(const nlohmann::json& node, uint32_t version, radahn::core::SimUnits units) override;

    virtual void convertSettingsTo(radahn::core::SimUnits destUnits) override;

protected:
    radahn::core::simIt_t m_nbStepsRequested = 0;
    radahn::core::simIt_t m_startStep = 0;
    radahn::core::simIt_t m_lastStep = 0;
    bool m_stepCountersSet = false;
};

}

}