#include <radahn/motor/motor.h>

void radahn::motor::Motor::addDependency(std::shared_ptr<Motor> dependency) 
{ 
    m_dependencies.push_back(dependency);
}

bool radahn::motor::Motor::canStart() const 
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
bool radahn::motor::Motor::startMotor()
{        
    if(m_status == MotorStatus::MOTOR_WAIT)
    {
        m_status = MotorStatus::MOTOR_RUNNING;
        return true;
    }
    else
        return false;
}

bool radahn::motor::Motor::loadFromJSON(const nlohmann::json& node, uint32_t version, radahn::core::SimUnits units)
{
    (void)version;
    (void)units;
    
    if(!node.contains("name"))
    {
        spdlog::error("Name not found while trying to load a motor from json.");
        return false;
    }
    m_name = node["name"].get<std::string>();
    m_motorWriter.setName(m_name);
    m_motorWriter.setSeparator(';');

    return true;
}

void radahn::motor::Motor::writeCSVFile(const std::string& folder) const
{
    m_motorWriter.writeFile(folder);
}