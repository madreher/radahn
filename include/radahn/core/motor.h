#pragma once 

#include <string>
#include <conduit/conduit.hpp>

#include <radahn/core/types.h>

namespace radahn {

namespace core {

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

    virtual bool updateState(simIt_t it) = 0;
    virtual bool appendCommandToConduitNode(conduit::Node& node) = 0;

protected:
    std::string m_name;
    MotorStatus m_status;
};

} // core

} // radahn