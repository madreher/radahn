#pragma once 

#include <string>
#include <conduit/conduit.hpp>
#include <nlohmann/json.hpp>

#include <radahn/core/types.h>
#include <radahn/core/units.h>

namespace radahn {

namespace motor {

enum class MotorStatus : uint8_t
{
    MOTOR_WAIT = 0,
    MOTOR_RUNNING = 1,
    MOTOR_SUCCESS = 2,
    MOTOR_FAILED = 3
};

class Motor 
{
public:
    Motor() = default;
    Motor(const std::string& name) : m_name(name), m_status(MotorStatus::MOTOR_WAIT){}
    virtual ~Motor(){}

    MotorStatus getMotorStatus() const { return m_status; }
    const std::string& getMotorName() const { return m_name; }

    virtual bool updateState(radahn::core::simIt_t it,
        const std::vector<radahn::core::atomIndexes_t>& indices, 
        const std::vector<radahn::core::atomPositions_t>& positions,
        conduit::Node& kvs) = 0;
    virtual bool appendCommandToConduitNode(conduit::Node& node) = 0;
    void addDependency(std::shared_ptr<Motor> dependency) { m_dependencies.push_back(dependency); }

    bool canStart() const 
    { 
        if( m_status != MotorStatus::MOTOR_WAIT )
            return false;

        for(const auto& dependency : m_dependencies)
        {
            if(dependency->getMotorStatus() != MotorStatus::MOTOR_SUCCESS)
                return false;
        } 

        return true;
    }
    bool startMotor()
    {        
        if(m_status == MotorStatus::MOTOR_WAIT)
        {
            m_status = MotorStatus::MOTOR_RUNNING;
            return true;
        }
        else
            return false;
    }

    virtual bool loadFromJSON(const nlohmann::json& node, uint32_t version, radahn::core::SimUnits units) = 0;

protected:
    std::string m_name;
    MotorStatus m_status = MotorStatus::MOTOR_WAIT;
    std::vector<std::shared_ptr<Motor>> m_dependencies;
};

} // core

} // radahn