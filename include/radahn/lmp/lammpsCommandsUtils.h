#pragma once 

#include <cstdint>
#include <span>
#include <memory>

#include <radahn/core/units.h>
#include <radahn/core/types.h>

#include <conduit/conduit.hpp>

namespace radahn {

namespace lmp {

enum class SimCommandType : uint32_t
{
    SIM_COMMAND_WAIT = 0,
    SIM_COMMAND_LMP_ROTATE = 1,
    SIM_COMMAND_LMP_MOVE = 2,
    SIM_COMMAND_LMP_ADD_FORCE = 3,
    SIM_COMMAND_LMP_ADD_TORQUE = 4
};

class LammpsCommand
{
public:
    LammpsCommand(){}
    virtual ~LammpsCommand(){}
    virtual bool loadFromConduit(conduit::Node& node) = 0;
    virtual bool writeDoCommands(std::vector<std::string>& cmds) const = 0;
    virtual bool writeUndoCommands(std::vector<std::string>& cmds) const = 0;
    virtual std::string getGroupName() const { return std::string(""); }
    virtual bool needMotionIntegration() const { return true; }
};

class MoveLammpsCommand : public LammpsCommand
{
public:
    radahn::core::VelocityQuantity m_vx;
    radahn::core::VelocityQuantity m_vy;
    radahn::core::VelocityQuantity m_vz;
    std::string m_origin;
    std::vector<radahn::core::atomIndexes_t> m_selection;

    MoveLammpsCommand() : LammpsCommand(){}
    virtual bool loadFromConduit(conduit::Node& node) override;
    virtual bool writeDoCommands(std::vector<std::string>& cmds) const override;
    virtual bool writeUndoCommands(std::vector<std::string>& cmds) const override;
    virtual std::string getGroupName() const override;
    virtual bool needMotionIntegration() const override { return false; }

};

class RotateLammpsCommand : public LammpsCommand
{
public:
    radahn::core::DistanceQuantity m_px;      // 3D point of the axe
    radahn::core::DistanceQuantity m_py;
    radahn::core::DistanceQuantity m_pz;
    radahn::core::atomPositions_t m_ax;       // Axe vector
    radahn::core::atomPositions_t m_ay;
    radahn::core::atomPositions_t m_az;
    std::string m_origin;
    radahn::core::TimeQuantity m_period;      // Period of the rotation
    std::vector<radahn::core::atomIndexes_t> m_selection;
    
    RotateLammpsCommand() : LammpsCommand(){}
    virtual bool loadFromConduit(conduit::Node& node) override;
    virtual bool writeDoCommands(std::vector<std::string>& cmds) const override;
    virtual bool writeUndoCommands(std::vector<std::string>& cmds) const override;
    virtual std::string getGroupName() const override;
    virtual bool needMotionIntegration() const override { return false; }
};

class AddForceLammpsCommand : public LammpsCommand
{
public:
    radahn::core::ForceQuantity m_fx;
    radahn::core::ForceQuantity m_fy;
    radahn::core::ForceQuantity m_fz;
    std::string m_origin;
    std::vector<radahn::core::atomIndexes_t> m_selection;

    AddForceLammpsCommand() : LammpsCommand(){}
    virtual bool loadFromConduit(conduit::Node& node) override;
    virtual bool writeDoCommands(std::vector<std::string>& cmds) const override;
    virtual bool writeUndoCommands(std::vector<std::string>& cmds) const override;
    virtual std::string getGroupName() const override;
};

class AddTorqueLammpsCommand : public LammpsCommand
{
public:
    radahn::core::TorqueQuantity m_tx;
    radahn::core::TorqueQuantity m_ty;
    radahn::core::TorqueQuantity m_tz;
    std::string m_origin;
    std::vector<radahn::core::atomIndexes_t> m_selection;

    AddTorqueLammpsCommand() : LammpsCommand(){}
    virtual bool loadFromConduit(conduit::Node& node) override;
    virtual bool writeDoCommands(std::vector<std::string>& cmds) const override;
    virtual bool writeUndoCommands(std::vector<std::string>& cmds) const override;
    virtual std::string getGroupName() const override;
};

class WaitLammpsCommand : public LammpsCommand
{
public:
    std::string m_origin;

    WaitLammpsCommand() : LammpsCommand(){}
    virtual bool loadFromConduit(conduit::Node& node) override;
    virtual bool writeDoCommands(std::vector<std::string>& cmds) const override;
    virtual bool writeUndoCommands(std::vector<std::string>& cmds) const override;
};

class LammpsCommandsUtils 
{
public:
    LammpsCommandsUtils(){}
    std::string getIntegrationGroup() const { return m_integrateGroupName; }
    void declarePermanentAnchorGroup(const std::string& groupName) { 
        m_permanentAnchorName = groupName; 
        m_hasPermanentAnchor = true;
    }

    bool loadCommandsFromConduit(conduit::Node& cmds);
    bool writeDoCommands(std::vector<std::string>& cmds) const;
    bool writeUndoCommands(std::vector<std::string>& cmds) const;
    

    static void registerMoveCommandToConduit(
        conduit::Node& node, 
        const std::string& name, 
        radahn::core::VelocityQuantity vx, 
        radahn::core::VelocityQuantity vy, 
        radahn::core::VelocityQuantity vz, 
        const std::vector<radahn::core::atomIndexes_t>& selection);
    static void registerAddForceCommandToConduit(
        conduit::Node& node, 
        const std::string& name, 
        radahn::core::ForceQuantity fx, 
        radahn::core::ForceQuantity fy, 
        radahn::core::ForceQuantity fz, 
        const std::vector<radahn::core::atomIndexes_t>& selection);
    static void registerAddTorqueCommandToConduit(
        conduit::Node& node, 
        const std::string& name, 
        radahn::core::TorqueQuantity tx, 
        radahn::core::TorqueQuantity ty, 
        radahn::core::TorqueQuantity tz, 
        const std::vector<radahn::core::atomIndexes_t>& selection);
    static void registerRotateCommandToConduit(
        conduit::Node& node, 
        const std::string& name, 
        radahn::core::DistanceQuantity px, 
        radahn::core::DistanceQuantity py, 
        radahn::core::DistanceQuantity pz, 
        radahn::core::atomPositions_t ax,     
        radahn::core::atomPositions_t ay,
        radahn::core::atomPositions_t az,
        radahn::core::TimeQuantity period,     
        const std::vector<radahn::core::atomIndexes_t>& selection);
    static void registerWaitCommandToConduit(conduit::Node& node, const std::string& name);

protected:
    std::vector<std::shared_ptr<LammpsCommand>> m_cmds;
    std::string m_integrateGroupName = "integrateGRP";
    std::string m_nonIntegrateGroupName = "nonintegrateGRP";
    bool m_hasPermanentAnchor = false;
    std::string m_permanentAnchorName;
};

} // core

} // radahn