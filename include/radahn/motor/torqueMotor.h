#pragma once 

#include <radahn/motor/motor.h>
#include <radahn/core/atomSet.h>
#include <radahn/core/units.h>
#include <radahn/lmp/lammpsCommandsUtils.h>

namespace radahn {

namespace motor {


class TorqueMotor : public Motor
{
public:
    TorqueMotor() : Motor(){}
    TorqueMotor(const std::string& name, const std::set<radahn::core::atomIndexes_t>& selection, 
        radahn::core::TorqueQuantity tx = radahn::core::TorqueQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::TorqueQuantity ty = radahn::core::TorqueQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL), 
        radahn::core::TorqueQuantity tz = radahn::core::TorqueQuantity(0.0, radahn::core::SimUnits::LAMMPS_REAL),
        double requestedAngle = 0.0
        ) : 
        Motor(name),
        m_currentState(selection), 
        m_tx(tx), m_ty(ty), m_tz(tz),
        m_requestedAngle(requestedAngle){}
    virtual ~TorqueMotor(){}

    virtual bool updateState(radahn::core::simIt_t it, 
        const std::vector<radahn::core::atomIndexes_t>& indices, 
        const std::vector<radahn::core::atomPositions_t>& positions,
        conduit::Node& kvs) override
        {
            if(m_status != MotorStatus::MOTOR_RUNNING)
                return false;

            m_currentState.selectAtoms(it, indices, positions);
            auto selectedPositions = m_currentState.getCurrentSelectedPositions();

            // The torque uses the center of mass to anchor the rotation axis
            // Therefor, unlike the move rotate method, we have to recompute the centroid every iteration
            // For now, we will use the geometrical center instead for tracking 
            auto geocenter = m_currentState.computePositionCenter();
            m_centroid = {geocenter[0], geocenter[1], geocenter[2]};

            if(!m_initialStateRegistered)
            {
                spdlog::info("Registering the initial state for the motor {}.", m_name);
                m_initialState = radahn::core::AtomSet(m_currentState);

                // Setup necessary varibles
                m_rotationAxis = {m_tx.m_value, m_ty.m_value, m_tz.m_value};
                m_rotationAxis = glm::normalize(m_rotationAxis);

                // Try to find an atom which is not too close of the axis
                while(m_trackedAtomIndex < m_currentState.getNbSelectedAtoms())
                {

                    // Save the first position
                    // HYPOTHESIS: the atoms are always sorted, therefor the order of the atoms is always the same
                    m_trackedPointFirstIteration = {selectedPositions[m_trackedAtomIndex*3], selectedPositions[m_trackedAtomIndex*3+1], selectedPositions[m_trackedAtomIndex*3+2]};
                    // Take two point very far away on the rotation axe to define a segment. GLM considers only a segment, not an infinite line for the projection.
                    // Without this, the result of the projection will likely end up at an extremity of the segment so we make it long enough to "simulate" a line.
                    m_trackedPointFirstIterationProjection = glm::closestPointOnLine(m_trackedPointFirstIteration, m_centroid - m_rotationAxis * 100000.0, m_centroid + m_rotationAxis * 100000.0);
                    m_firstIterationProjectionVector = m_trackedPointFirstIteration - m_trackedPointFirstIterationProjection;
                    double distanceFromAxis = glm::length(m_firstIterationProjectionVector);
                    //spdlog::info("Atom index {} Distance between the track point and the axis: {}", m_trackedAtomIndex, distanceFromAxis);
                    if(distanceFromAxis < 0.01)
                    {
                        spdlog::info("Atom index {} is too close to the axis. Changing atom.", m_trackedAtomIndex);
                        m_trackedAtomIndex++;
                    }
                    else 
                    {
                        spdlog::info("Atom index {} is far enough from the rotation axis. Keeping it.", m_trackedAtomIndex);
                        break;
                    }
                }

                // Check that we have found a proper atom
                if(m_trackedAtomIndex == m_initialState.getNbSelectedAtoms())
                {
                    spdlog::error("Was unable to find an atom not on the rotation axis. Abording.");
                    m_status = MotorStatus::MOTOR_FAILED;
                    return false;
                }
                
                
                m_firstIterationProjectionVector = glm::normalize(m_firstIterationProjectionVector);
                m_initialStateRegistered = true;
                return true;
            }
            

            

            //glm::dvec3 pointOnAxe = centroid + axe;
            glm::dvec3 trackedPointCurrent = {selectedPositions[m_trackedAtomIndex*3], selectedPositions[m_trackedAtomIndex*3+1], selectedPositions[m_trackedAtomIndex*3+2]};
            glm::dvec3 trackedPointProjection = glm::closestPointOnLine(trackedPointCurrent, m_centroid - m_rotationAxis * 10000000.0, m_centroid + m_rotationAxis * 10000000.0);
            glm::dvec3 currentIterationProjectionVector = trackedPointCurrent - trackedPointProjection;
            //double distanceFromAxis = glm::length(currentIterationProjectionVector);
            //spdlog::info("Atom index {}, Distance between the track point and the axis: {}", m_trackedAtomIndex, distanceFromAxis);
            currentIterationProjectionVector = glm::normalize(currentIterationProjectionVector);

            // Produces a result in rad between -PI and PI
            double rotationFromFirstRad = glm::orientedAngle(m_firstIterationProjectionVector, currentIterationProjectionVector, m_rotationAxis);
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
            kvs["current_total_angle_deg"] = m_totalRotationDeg;
            kvs["current_angle_deg"] = rotationFromFirstDeg;
            kvs["track_x"] = trackedPointCurrent.x;
            kvs["track_y"] = trackedPointCurrent.y;
            kvs["track_z"] = trackedPointCurrent.z;
            kvs["centroid_x"] = m_centroid.x;
            kvs["centroid_y"] = m_centroid.y;
            kvs["centroid_z"] = m_centroid.z;

            m_previousRotationAngleRad = rotationFromFirstRad;
            m_previousRotationAngleDeg = rotationFromFirstDeg;

            if (m_totalRotationDeg >= m_requestedAngle)
            {
                spdlog::info("Motor {} completed successfully.", m_name);
                m_status = MotorStatus::MOTOR_SUCCESS;
            }

            return true;
        }
        
    virtual bool appendCommandToConduitNode(conduit::Node& node) override
    {
        radahn::lmp::LammpsCommandsUtils::registerAddTorqueCommandToConduit(node, m_name, m_tx, m_ty, m_tz, m_currentState.getSelectionVector());

        return true;
    }

    virtual bool loadFromJSON(const nlohmann::json& node, uint32_t version, radahn::core::SimUnits units) override
    {
        (void)version;
        
        if(!node.contains("name"))
        {
            spdlog::error("TName not found while trying to load a TorqueMotor from json.");
            return false;
        }
        m_name = node["name"].get<std::string>();
        
        if(!node.contains("selection"))
        {
            spdlog::error("Selection not found while trying to load the TorqueMotor {} from json.", m_name);
            return false;
        }
        m_currentState = radahn::core::AtomSet(node["selection"].get<std::set<radahn::core::atomIndexes_t>>());

        m_tx = radahn::core::TorqueQuantity(node.value("tx", 0.0), units);
        m_ty = radahn::core::TorqueQuantity(node.value("ty", 0.0), units);
        m_tz = radahn::core::TorqueQuantity(node.value("tz", 0.0), units);

        m_requestedAngle = node.value("requestedAngle", 0.0);
        if(m_requestedAngle <= 0.0)
        {
            spdlog::error("RequestedAngle must be positive while trying to load the RotateMotor {} from json. For a negative rotation, please flip the rotation axis.", m_name);
            return false;
        }

        return true;
    }

    virtual void convertSettingsTo(radahn::core::SimUnits destUnits) override
    {
        m_tx.convertTo(destUnits);
        m_ty.convertTo(destUnits);
        m_tz.convertTo(destUnits);
    }

protected:
    // Settings variables
    radahn::core::AtomSet m_currentState;
    radahn::core::TorqueQuantity m_tx;
    radahn::core::TorqueQuantity m_ty;
    radahn::core::TorqueQuantity m_tz;
    
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