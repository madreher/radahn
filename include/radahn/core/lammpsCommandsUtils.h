#pragma once 

#include <cstdint>
#include <span>
#include <memory>

#include <radahn/core/units.h>
#include <radahn/core/types.h>

#include <conduit/conduit.hpp>

namespace radahn {

namespace core {

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
    VelocityQuantity m_vx;
    VelocityQuantity m_vy;
    VelocityQuantity m_vz;
    std::string m_origin;
    std::vector<atomIndexes_t> m_selection;

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
    DistanceQuantity m_px;      // 3D point of the axe
    DistanceQuantity m_py;
    DistanceQuantity m_pz;
    atomPositions_t m_ax;       // Axe vector
    atomPositions_t m_ay;
    atomPositions_t m_az;
    std::string m_origin;
    TimeQuantity m_period;      // Period of the rotation
    std::vector<atomIndexes_t> m_selection;
    
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
    ForceQuantity m_fx;
    ForceQuantity m_fy;
    ForceQuantity m_fz;
    std::string m_origin;
    std::vector<atomIndexes_t> m_selection;

    AddForceLammpsCommand() : LammpsCommand(){}
    virtual bool loadFromConduit(conduit::Node& node) override;
    virtual bool writeDoCommands(std::vector<std::string>& cmds) const override;
    virtual bool writeUndoCommands(std::vector<std::string>& cmds) const override;
    virtual std::string getGroupName() const override;
};

class AddTorqueLammpsCommand : public LammpsCommand
{
public:
    TorqueQuantity m_tx;
    TorqueQuantity m_ty;
    TorqueQuantity m_tz;
    std::string m_origin;
    std::vector<atomIndexes_t> m_selection;

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

    bool loadCommandsFromConduit(conduit::Node& cmds);
    bool writeDoCommands(std::vector<std::string>& cmds) const;
    bool writeUndoCommands(std::vector<std::string>& cmds) const;
    

    static void registerMoveCommandToConduit(
        conduit::Node& node, 
        const std::string& name, 
        VelocityQuantity vx, 
        VelocityQuantity vy, 
        VelocityQuantity vz, 
        const std::vector<atomIndexes_t>& selection);
    static void registerAddForceCommandToConduit(
        conduit::Node& node, 
        const std::string& name, 
        ForceQuantity fx, 
        ForceQuantity fy, 
        ForceQuantity fz, 
        const std::vector<atomIndexes_t>& selection);
    static void registerAddTorqueCommandToConduit(
        conduit::Node& node, 
        const std::string& name, 
        TorqueQuantity tx, 
        TorqueQuantity ty, 
        TorqueQuantity tz, 
        const std::vector<atomIndexes_t>& selection);
    static void registerRotateCommandToConduit(
        conduit::Node& node, 
        const std::string& name, 
        DistanceQuantity px, 
        DistanceQuantity py, 
        DistanceQuantity pz, 
        atomPositions_t ax,     
        atomPositions_t ay,
        atomPositions_t az,
        TimeQuantity period,     
        const std::vector<atomIndexes_t>& selection);
    static void registerWaitCommandToConduit(conduit::Node& node, const std::string& name);

protected:
    std::vector<std::shared_ptr<LammpsCommand>> m_cmds;
    std::string m_integrateGroupName = "integrateGRP";
    std::string m_nonIntegrateGroupName = "nonintegrateGRP";
};

} // core

} // radahn