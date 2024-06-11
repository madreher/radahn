#include <radahn/motor/motorEngine.h>

#include <radahn/motor/blankMotor.h>
#include <radahn/motor/moveMotor.h>
#include <radahn/motor/rotateMotor.h>
#include <radahn/motor/forceMotor.h>
#include <radahn/motor/torqueMotor.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace radahn::core;
using namespace radahn::motor;

void radahn::motor::MotorEngine::loadTestMotorSetup()
{
    // Test setup

    // Declare test motors
    m_motorsMap["testWait"] = std::make_shared<BlankMotor>("testWait", 30000);
    m_activeMotors.push_back(m_motorsMap["testWait"]);
    
    //std::set<atomIndexes_t> selectionMove = {1,2,3,4,5,6,7,8,9,10,11,12};
    
    //m_motors.emplace_back(std::make_shared<MoveMotor>("testMove", selectionMove, 
    //    VelocityQuantity(0.0001, SimUnits::LAMMPS_REAL), VelocityQuantity(0.0, SimUnits::LAMMPS_REAL), VelocityQuantity(0.0, SimUnits::LAMMPS_REAL),
    //    true, false, false, 
    //    DistanceQuantity(1.0, SimUnits::LAMMPS_REAL), DistanceQuantity(0.0, SimUnits::LAMMPS_REAL), DistanceQuantity(0.0, SimUnits::LAMMPS_REAL)));
    
    //m_motors.emplace_back(std::make_shared<RotateMotor>("testRotate", selectionMove, 
    //    DistanceQuantity(52.0, SimUnits::LAMMPS_REAL), DistanceQuantity(52.0, SimUnits::LAMMPS_REAL), DistanceQuantity(50.0, SimUnits::LAMMPS_REAL),
    //    1.0, 0.0, 0.0, TimeQuantity(10000.0, SimUnits::LAMMPS_REAL), 180));
    
    //m_motors.emplace_back(std::make_shared<ForceMotor>("testForce", selectionMove, 
    //    ForceQuantity(0.001, SimUnits::LAMMPS_REAL), ForceQuantity(0.0, SimUnits::LAMMPS_REAL), ForceQuantity(0.0, SimUnits::LAMMPS_REAL),
    //    true, false, false, 
    //    DistanceQuantity(1.0, SimUnits::LAMMPS_REAL), DistanceQuantity(0.0, SimUnits::LAMMPS_REAL), DistanceQuantity(0.0, SimUnits::LAMMPS_REAL)));
    //m_motors.emplace_back(std::make_shared<TorqueMotor>("testTorque", selectionMove, 
    //    TorqueQuantity(0.001, SimUnits::LAMMPS_REAL), TorqueQuantity(0.0, SimUnits::LAMMPS_REAL), TorqueQuantity(0.0, SimUnits::LAMMPS_REAL),
    //    90.0));

    // Make them start immediatly
    for(auto & motor : m_activeMotors)
        motor->startMotor();
}

bool radahn::motor::MotorEngine::updateMotorsState(simIt_t it,
    std::vector<atomIndexes_t>& indices, 
    std::vector<atomPositions_t>& positions)
{
    // First, we need to first the received positions
    // HYPOTHESIS: We receive ALL the atom information, not just a sub-selection!!!
    size_t nbAtoms = indices.size();
    m_currentIndexes.resize(3*nbAtoms);
    m_currentPositions.resize(3*nbAtoms);

    for(size_t i = 0; i < nbAtoms; ++i)
    {
        // Lammps indices are 1-based
        int64_t atomID = static_cast<int64_t>(indices[i]);
        uint64_t newIndex = static_cast<uint64_t>(atomID-1);
        m_currentIndexes[newIndex] = indices[i];
        m_currentPositions[3*newIndex] = positions[3*i];
        m_currentPositions[3*newIndex+1] = positions[3*i+1];
        m_currentPositions[3*newIndex+2] = positions[3*i+2];
    }

    // Initialize the data for the current iteration
    m_currentKVS = conduit::Node();
    //m_currentKVS["global"]["step"] = it;

    // Now we can update the motors with the sorted arrays
    bool result = true;
    for(auto & motor : m_activeMotors)
        result &= motor->updateState(it, m_currentIndexes, m_currentPositions, m_currentKVS.add_child(motor->getMotorName()));

    m_currentIt = it;

    return result;
}

bool radahn::motor::MotorEngine::getCommandsFromMotors(conduit::Node& node) const
{
    bool result = true;
    for(auto & motor : m_activeMotors)
        result &= motor->appendCommandToConduitNode(node);
    return result;
}

bool radahn::motor::MotorEngine::isCompleted() const
{
    for(auto & [name, motor] : m_motorsMap)
    {
        if(motor->getMotorStatus() !=  radahn::motor::MotorStatus::MOTOR_SUCCESS)
            return false;
    }

    return true;
}

void radahn::motor::MotorEngine::clearMotors()
{
    m_motorsMap.clear();
    m_activeMotors.clear();
    m_pendingMotors.clear();
}

bool radahn::motor::MotorEngine::loadFromJSON(const std::string& filename)
{
    json document = json::parse(std::ifstream(filename));
    if(!document.contains("motors"))
    {
        spdlog::error("Could not find motors in JSON file {}.", filename);
        return false;
    }

    for(auto & motor : document["motors"])
    {
        if(!motor.contains("type"))
        {
            spdlog::error("Could not find motor type for a motror in JSON file {}.", filename);
            return false;
        }

        std::string type = motor["type"];
        std::shared_ptr<Motor> motorPtr;
        if(type == "blank")
        {
            motorPtr = std::make_shared<BlankMotor>();
        }
        else if(type == "move")
        {
            motorPtr = std::make_shared<MoveMotor>();
        }
        else if(type == "rotate")
        {
            motorPtr = std::make_shared<RotateMotor>();
        }
        else if(type == "force")
        {
            motorPtr = std::make_shared<ForceMotor>();
        }
        else if(type == "torque")
        {
            motorPtr = std::make_shared<TorqueMotor>();
        }
        else
        {
            spdlog::error("Unknown motor type {} in JSON file {}.", type, filename);
            return false;
        }

        if(!motorPtr->loadFromJSON(motor, 0, radahn::core::SimUnits::LAMMPS_REAL))
        {
            spdlog::error("Could not load motor {} from JSON file {}.", type, filename);
            return false;
        }
        else
        {
            m_motorsMap[motorPtr->getMotorName()] = motorPtr;
            m_pendingMotors.push_back(motorPtr);
        }    
    }

    // Process the dependencies
    for(auto & motor : document["motors"])
    {
        if(motor.contains("dependencies"))
        {
            for(auto & dependency : motor["dependencies"])
            {
                if(m_motorsMap.find(dependency) == m_motorsMap.end())
                {
                    spdlog::error("Could not find dependency {} in JSON file {}.", dependency, filename);
                    return false;
                }
                m_motorsMap[motor["name"]]->addDependency(m_motorsMap[dependency]);
            }
        }
    }

    // All the motors are loaded, filling the active list
    for(auto & motor : m_pendingMotors)
    {
        if(motor->canStart())
        {
            motor->startMotor();
            m_activeMotors.push_back(motor);
        }
    }

    // Remove the motors which have been started
    for(auto it = m_pendingMotors.begin(); it != m_pendingMotors.end();)
    {
        if((*it)->getMotorStatus() == radahn::motor::MotorStatus::MOTOR_RUNNING)
            it = m_pendingMotors.erase(it);
        else
            it++;
    }
    return true;
}