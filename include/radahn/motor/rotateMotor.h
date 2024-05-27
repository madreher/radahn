#pragma once 

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/closest_point.hpp>

#include <radahn/motor/motor.h>
#include <radahn/core/atomSet.h>
#include <radahn/core/units.h>
//#include <radahn/core/math.h>
#include <radahn/lmp/lammpsCommandsUtils.h>

namespace radahn {

namespace motor {

class RotateMotor : public Motor 
{
public:
    RotateMotor() : Motor(){}
    RotateMotor(const std::string& name, const std::set<radahn::core::atomIndexes_t>& selection, 
        radahn::core::DistanceQuantity px = radahn::core::DistanceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::DistanceQuantity py = radahn::core::DistanceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::DistanceQuantity pz = radahn::core::DistanceQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL),
        radahn::core::atomPositions_t ax = 0.0, radahn::core::atomPositions_t ay = 0.0, radahn::core::atomPositions_t az = 0.0,
        radahn::core::TimeQuantity period = radahn::core::TimeQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL),
        double requestedAngle = 0.0
        ) : 
        Motor(name),
        m_currentState(selection), 
        m_px(px), m_py(py), m_pz(pz),
        m_ax(ax), m_ay(ay), m_az(az),
        m_period(period), m_requestedAngle(requestedAngle){}
    virtual ~RotateMotor(){}

    virtual bool updateState(radahn::core::simIt_t it, 
        const std::vector<radahn::core::atomIndexes_t>& indices, 
        const std::vector<radahn::core::atomPositions_t>& positions,
        conduit::Node& kvs) override
        {
            if(m_status != MotorStatus::MOTOR_RUNNING)
                return false;

            glm::dvec3 centroid = {m_px.m_value, m_py.m_value, m_pz.m_value};
            glm::dvec3 axe = {m_ax, m_ay, m_az};
            axe = glm::normalize(axe);

            m_currentState.selectAtoms(it, indices, positions);
            auto selectedPositions = m_currentState.getCurrentSelectedPositions();
            if(!m_initialStateRegistered)
            {
                spdlog::info("Registering the initial state for the motor {}.", m_name);
                m_initialState = radahn::core::AtomSet(m_currentState);
                // Save the first position
                // HYPOTHESIS: the atoms are always sorted, therefor the order of the atoms is always the same
                m_trackedPointFirstIteration = {selectedPositions[0], selectedPositions[1], selectedPositions[2]};
                // Take two point very far away on the rotation axe to define a segment. GLM considers only a segment not an infinite line for the projection.
                // Without this, the result of the projection will likely end up at an extremity of the segment so we make it long enough to "simulate" a line.
                m_trackedPointFirstIterationProjection = glm::closestPointOnLine(m_trackedPointFirstIteration, centroid - axe * 100000.0, centroid + axe * 100000.0);
                m_initialStateRegistered = true;
                return true;
            }

            

            //glm::dvec3 pointOnAxe = centroid + axe;
            glm::dvec3 trackedPointCurrent = {selectedPositions[0], selectedPositions[1], selectedPositions[2]};
            glm::dvec3 trackedPointProjection = glm::closestPointOnLine(trackedPointCurrent, centroid - axe * 100000.0, centroid + axe * 100000.0);
            glm::dvec3 firstIterationProjectionVector = m_trackedPointFirstIteration - m_trackedPointFirstIterationProjection;
            glm::dvec3 currentIterationProjectionVector = trackedPointCurrent - trackedPointProjection;
            firstIterationProjectionVector = glm::normalize(firstIterationProjectionVector);
            currentIterationProjectionVector = glm::normalize(currentIterationProjectionVector);

            // Produces a result in rad between -PI and PI
            double rotationFromFirstRad = glm::orientedAngle(firstIterationProjectionVector, currentIterationProjectionVector, axe);
            if(rotationFromFirstRad < 0.0)
                rotationFromFirstRad = 2.0*M_PI + rotationFromFirstRad; // + because avgRotationRad is negative
            double rotationFromFirstDeg = rotationFromFirstRad * 180.0 / M_PI;

            // Update the rotation counter
            // Case for positive rotation
            if(rotationFromFirstDeg < 90.0 && m_previousRotationAngleDeg > 250.0)
                m_nbRotationCompleted++;
            // Case for negative rotation
            if(rotationFromFirstDeg > 250.0 && m_previousRotationAngleDeg < 90.0)
                m_nbRotationCompleted--;

            m_totalRotationDeg = static_cast<double>(m_nbRotationCompleted) * 360.0 + rotationFromFirstDeg;

            kvs["progress"] = (m_totalRotationDeg / m_requestedAngle) * 100.0;
            kvs["current_angle_deg"] = m_totalRotationDeg;
            kvs["trackX"] = trackedPointCurrent.x;
            kvs["trackY"] = trackedPointCurrent.y;
            kvs["trackZ"] = trackedPointCurrent.z;

            if (m_totalRotationDeg > m_requestedAngle)
            {
                spdlog::info("Motor {} completed successfully.", m_name);
                m_status = MotorStatus::MOTOR_SUCCESS;
            }

            // Produces a result between -PI and PI, moving to 0, 2PI
    //avgRotationRad = avgRotationRad / (double)(_tracker->getNbAtoms());
   



            // Check if we have met the conditions
            /*
            _axePoint[0] = _centroid[0] + _axe[0];
    _axePoint[1] = _centroid[1] + _axe[1];
    _axePoint[2] = _centroid[2] + _axe[2];

    tools::closestPointOnLine(_centroid, _axePoint, _origin, _projOrigin);
    tools::makeVec(_projOrigin, _origin, _vecOrigin);

    //fprintf(stderr, "Centroid: [%f %f %f]\n", _centroid[0], _centroid[1], _centroid[2]);
    _motorKeyValueStore->addCurrentValue("centroidX", _centroid[0]);
    _motorKeyValueStore->addCurrentValue("centroidY", _centroid[1]);
    _motorKeyValueStore->addCurrentValue("centroidZ", _centroid[2]);

    glm::dvec3 rotationAxis(_axe[0], _axe[1], _axe[2]);
    rotationAxis = glm::normalize(rotationAxis);
    double motor_it = std::get<double>(_motorKeyValueStore->getCurrentValue("motor_it"));


    // Computing the current state of the rotation
    double avgRotationRad = 0.0;
    glm::dvec3 origin(_centroid[0], _centroid[1], _centroid[2]);

    double sim_it = std::get<double>(globalValues->getCurrentValue("sim_it"));
    for(unsigned int i = 0; i < _tracker->getNbAtoms(); i++)
    {
        if(_tracker->getNbIterationsStored() > 1 && _tracker->getAtomIndices()[i] == _ref)
        {
            glm::dvec3 point(atomPositions[3*i], atomPositions[3*i+1], atomPositions[3*i+2]);
            glm::dvec3 firstPoint(_firstPositions[3*i], _firstPositions[3*i+1], _firstPositions[3*i+2]);
            glm::dvec3 firstProj = glm::closestPointOnLine(firstPoint, origin - rotationAxis * 100000.0, origin + rotationAxis * 100000.0);
            glm::dvec3 proj = glm::closestPointOnLine(point, origin - rotationAxis * 100000.0, origin + rotationAxis * 100000.0);
            glm::dvec3 vec1 = firstPoint - firstProj;
            glm::dvec3 vec2 = point - proj;
            vec1 = glm::normalize(vec1);
            vec2 = glm::normalize(vec2);
            //avgRotation +=  glm::orientedAngle(vec1, vec2, rotationAxis);
            //double angle = glm::angle(vec1, vec2);
            double angle = glm::orientedAngle(vec1, vec2, rotationAxis);
            avgRotationRad += angle; // rad
        }
    }

    // Produces a result between -PI and PI, moving to 0, 2PI
    //avgRotationRad = avgRotationRad / (double)(_tracker->getNbAtoms());
    if(avgRotationRad < 0.0)
        avgRotationRad = 2.0*M_PI + avgRotationRad; // + because avgRotationRad is negative
    //_motorKeyValueStore->addCurrentValue("avg_rotation_rad", avgRotationRad);


    // Computing the angle difference between the current angle and the previous one
    double previousAngleRad = (_previousAvgRotation * M_PI)/180.0;
    double speed_rad = avgRotationRad - previousAngleRad;


    // The rotation finished a cycle in positive direction
    if(avgRotationRad < M_PI/2.0 && previousAngleRad > (3.0*M_PI)/2.0)
        speed_rad = 2.0*M_PI - previousAngleRad + avgRotationRad;
    // The rotation finished a cycle in negative direction
    else if(avgRotationRad > (3.0*M_PI)/2.0 && previousAngleRad < M_PI/2.0)
        speed_rad = -(2.0*M_PI - avgRotationRad + previousAngleRad);
    double speed_deg = speed_rad * 180.0 / M_PI;

    _motorKeyValueStore->addCurrentValue("ref_rotation_speed_rad", speed_rad);
    double avgRotationDeg = avgRotationRad * 180.0 / M_PI;


    // Checking if we have finished a rotation in the positive direction
    if(avgRotationDeg < 90.0 && _previousAvgRotation > 250.0)
        _completeRotations++;
    // Checking if we have finished a rotation in the negative direction
    if(avgRotationDeg > 250.0 && _previousAvgRotation < 90.0)
        _completeRotations--;

    _sumRotation = (double)_completeRotations * 360.0 + avgRotationDeg;*/

            return true;
        }
        
    virtual bool appendCommandToConduitNode(conduit::Node& node) override
    {
        // Heavy syntax still required for c++20
        // for c++23, prefer using std::to_underlying
        radahn::lmp::LammpsCommandsUtils::registerRotateCommandToConduit(node, m_name, m_px, m_py, m_pz, m_ax, m_ay, m_az, m_period, m_currentState.getSelectionVector());

        return true;
    }

protected:
    radahn::core::AtomSet m_currentState;
    radahn::core::DistanceQuantity m_px;      // 3D point of the axe
    radahn::core::DistanceQuantity m_py;
    radahn::core::DistanceQuantity m_pz;
    radahn::core::atomPositions_t m_ax;       // Axe vector
    radahn::core::atomPositions_t m_ay;
    radahn::core::atomPositions_t m_az;
    radahn::core::TimeQuantity m_period;      // Period of a rotation
    double m_requestedAngle;
    bool m_initialStateRegistered   = false;
    radahn::core::AtomSet m_initialState;

    // Variables used internally only to track the rotation
    glm::dvec3 m_trackedPointFirstIteration;            // Tracked atom taken when the motor started. It served as a starting point to track the rotation done
    glm::dvec3 m_trackedPointFirstIterationProjection;  // Projection of the tracked point on the rotation axis based on its positions during the first iteration
    double m_previousRotationAngleDeg = 0;
    double m_previousRotationAngleRad = 0;
    double m_totalRotationDeg = 0;
    uint32_t m_nbRotationCompleted = 0;
};    

} // core

} // radahn