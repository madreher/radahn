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
    virtual std::string writeDoCommands() const = 0;
    virtual std::string getGroupName() const { return std::string(""); }
};

class MoveLammpsCommand : public LammpsCommand
{
public:
    VelocityQuantity m_vx;
    VelocityQuantity m_vy;
    VelocityQuantity m_vz;
    std::string m_origin;
    std::span<atomIndexes_t> m_selection;

    MoveLammpsCommand() : LammpsCommand(){}
    virtual bool loadFromConduit(conduit::Node& node) override;
    virtual std::string writeDoCommands() const override;
    virtual std::string getGroupName() const override;

};

class WaitLammpsCommand : public LammpsCommand
{
public:
    std::string m_origin;

    WaitLammpsCommand() : LammpsCommand(){}
    virtual bool loadFromConduit(conduit::Node& node) override;
    virtual std::string writeDoCommands() const override;
};

class LammpsCommandsUtils 
{
public:
    LammpsCommandsUtils(){}

    bool loadCommandsFromConduit(conduit::Node& cmds);

    static void registerMoveCommandToConduit(conduit::Node& node, const std::string& name, VelocityQuantity vx, VelocityQuantity vy, VelocityQuantity vz, const std::vector<atomIndexes_t>& selection);
    static void registerWaitCommandToConduit(conduit::Node& node, const std::string& name);

protected:
    std::vector<std::shared_ptr<LammpsCommand>> m_cmds;
};

} // core

} // radahn