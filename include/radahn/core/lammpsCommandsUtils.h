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
    virtual bool needMotionInteration() const { return true; }
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
    virtual bool needMotionInteration() const override { return false; }

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
    

    static void registerMoveCommandToConduit(conduit::Node& node, const std::string& name, VelocityQuantity vx, VelocityQuantity vy, VelocityQuantity vz, const std::vector<atomIndexes_t>& selection);
    static void registerWaitCommandToConduit(conduit::Node& node, const std::string& name);

protected:
    std::vector<std::shared_ptr<LammpsCommand>> m_cmds;
    std::string m_integrateGroupName = "integrateGRP";
    std::string m_nonIntegrateGroupName = "nonintegrateGRP";
};

} // core

} // radahn