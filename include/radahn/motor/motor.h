#pragma once 

#include <string>
#include <conduit/conduit.hpp>

#include <radahn/core/types.h>

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

protected:
    std::string m_name;
    MotorStatus m_status;
};

} // core

} // radahn