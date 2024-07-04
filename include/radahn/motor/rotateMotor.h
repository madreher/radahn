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
#include <radahn/lmp/lammpsCommandsUtils.h>

namespace radahn {

namespace motor {

class RotateMotor : public Motor 
{
public:
    RotateMotor() : Motor()
    {
        declareCSVWriterFieldNames();
    }
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
        m_period(period), m_requestedAngle(requestedAngle)
    {
        declareCSVWriterFieldNames();
    }
    
    virtual ~RotateMotor(){}

    virtual bool updateState(radahn::core::simIt_t it, 
        const std::vector<radahn::core::atomIndexes_t>& indices, 
        const std::vector<radahn::core::atomPositions_t>& positions,
        conduit::Node& kvs) override;
        
    virtual bool appendCommandToConduitNode(conduit::Node& node) override;

    virtual bool loadFromJSON(const nlohmann::json& node, uint32_t version, radahn::core::SimUnits units) override;

    virtual void convertSettingsTo(radahn::core::SimUnits destUnits) override;

protected:
    virtual void declareCSVWriterFieldNames() override;
    // Settings variables
    radahn::core::AtomSet m_currentState;
    radahn::core::DistanceQuantity m_px;      // 3D point of the axe
    radahn::core::DistanceQuantity m_py;
    radahn::core::DistanceQuantity m_pz;
    radahn::core::atomPositions_t m_ax;       // Axe vector
    radahn::core::atomPositions_t m_ay;
    radahn::core::atomPositions_t m_az;
    radahn::core::TimeQuantity m_period;      // Period of a rotation
    double m_requestedAngle;
    

    // Variables used internally only to track the rotation
    bool m_initialStateRegistered   = false;
    radahn::core::AtomSet m_initialState;
    glm::dvec3 m_centroid;
    glm::dvec3 m_rotationAxis;
    glm::dvec3 m_trackedPointFirstIteration;            // Tracked atom taken when the motor started. It served as a starting point to track the rotation done
    glm::dvec3 m_trackedPointFirstIterationProjection;  // Projection of the tracked point on the rotation axis based on its positions during the first iteration
    glm::dvec3 m_firstIterationProjectionVector;        // Vector between the tracked atom position during the first iteration and its projection on the rotation vector
    double m_previousRotationAngleDeg = 0;
    double m_previousRotationAngleRad = 0;
    double m_totalRotationDeg = 0;
    int32_t m_nbRotationCompleted = 0;
    size_t m_trackedAtomIndex = 0;
};    

} // core

} // radahn