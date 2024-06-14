#pragma once

#include <string>
#include <cstdint>
#include <stdexcept>

namespace radahn {

namespace core {

enum class SimUnits : uint8_t
{
    LAMMPS_REAL = 0,
    LAMMPS_METAL = 1,
    GROMACS = 2
};


constexpr const char* to_string(SimUnits u)// throw()
{
    switch (u)
    {
    case SimUnits::LAMMPS_METAL:
        return "LAMMPS_METAL";
    case SimUnits::LAMMPS_REAL:
        return "LAMMPS_REAL";
    case SimUnits::GROMACS:
        return "GROMACS";
    default:
        throw std::invalid_argument("Unknown SimUnits given to to_string(SimUnits).");
    }
}

constexpr SimUnits from_string(const std::string_view& str)// throw()
{
    if (str == "LAMMPS_METAL")
        return SimUnits::LAMMPS_METAL;
    if (str == "LAMMPS_REAL")
        return SimUnits::LAMMPS_REAL;
    if (str == "GROMACS")
        return SimUnits::GROMACS;

    throw std::invalid_argument("Unknown string given to from_string(const std::string&).");
}

// In this case, we are limiting ourselves to only positions, velocities, and forces, and only with 3 unit sets with few calls. 
// If the use case becomes more complicated, it will be worth looking at proper unit libraries.

class DistanceQuantity
{
public:
    DistanceQuantity(){}
    DistanceQuantity(double value, SimUnits unit) : m_value(value), m_unit(unit){}

    double convertTo(SimUnits destUnit) const;

    double m_value;
    SimUnits m_unit;
};

class VelocityQuantity
{
public:
    VelocityQuantity(){}
    VelocityQuantity(double value, SimUnits unit) : m_value(value), m_unit(unit){}

    double convertTo(SimUnits destUnit) const;

    double m_value;
    SimUnits m_unit;
};

class ForceQuantity
{
public:
    ForceQuantity(){}
    ForceQuantity(double value, SimUnits unit) : m_value(value), m_unit(unit){}

    double convertTo(SimUnits destUnit) const;

    double m_value;
    SimUnits m_unit;
};

class TorqueQuantity
{
public:
    TorqueQuantity(){}
    TorqueQuantity(double value, SimUnits unit) : m_value(value), m_unit(unit){}

    double convertTo(SimUnits destUnit) const;

    double m_value;
    SimUnits m_unit;
};

class TimeQuantity
{
public:
    TimeQuantity(){}
    TimeQuantity(double value, SimUnits unit) : m_value(value), m_unit(unit){}

    double convertTo(SimUnits destUnit) const;

    double m_value;
    SimUnits m_unit;
};


} // core

} // radahn