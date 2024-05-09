#pragma once 

#include <cstdint>
#include <vector>

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
    virtual std::string commandToLammpsScript() const = 0;
};

class MoveLammpsCommand : public LammpsCommand
{

    MoveLammpsCommand() : LammpsCommand(){}


};

class LammpsCommandsUtils 
{
public:
    LammpsCommandsUtils(){}

    static void registerMoveCommandToConduit(conduit::Node& node, const std::string& name, VelocityQuantity vx, VelocityQuantity vy, VelocityQuantity vz, const std::vector<atomIndexes_t>& selection);

};

} // core

} // radahn