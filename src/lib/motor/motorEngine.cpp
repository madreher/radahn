#include <radahn/motor/motorEngine.h>

#include <radahn/motor/blankMotor.h>
#include <radahn/motor/moveMotor.h>
#include <radahn/motor/rotateMotor.h>
#include <radahn/motor/forceMotor.h>
#include <radahn/motor/torqueMotor.h>

using namespace radahn::core;
using namespace radahn::motor;

void radahn::motor::MotorEngine::loadTestMotorSetup()
{
    // Test setup

    // Declare test motors
    //m_motors.emplace_back(std::make_shared<BlankMotor>("testWait", 1000));
    
    std::set<atomIndexes_t> selectionMove = {1,2,3,4,5,6,7,8,9,10,11,12};
    
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
    m_motors.emplace_back(std::make_shared<TorqueMotor>("testTorque", selectionMove, 
        TorqueQuantity(0.001, SimUnits::LAMMPS_REAL), TorqueQuantity(0.0, SimUnits::LAMMPS_REAL), TorqueQuantity(0.0, SimUnits::LAMMPS_REAL),
        90.0));

    // Make them start immediatly
    for(auto & motor : m_motors)
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
    m_currentKVS["global"]["step"] = it;

    // Now we can update the motors with the sorted arrays
    bool result = true;
    for(auto & motor : m_motors)
        result &= motor->updateState(it, m_currentIndexes, m_currentPositions, m_currentKVS.add_child(motor->getMotorName()));

    return result;
}

bool radahn::motor::MotorEngine::getCommandsFromMotors(conduit::Node& node) const
{
    bool result = true;
    for(auto & motor : m_motors)
        result &= motor->appendCommandToConduitNode(node);
    return result;
}

bool radahn::motor::MotorEngine::isCompleted() const
{
    for(auto & motor : m_motors)
    {
        if(motor->getMotorStatus() !=  radahn::motor::MotorStatus::MOTOR_SUCCESS)
            return false;
    }

    return true;
}

void radahn::motor::MotorEngine::clearMotors()
{
    m_motors.clear();
}