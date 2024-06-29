#pragma once

#include <string>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <functional>
#include <spdlog/spdlog.h>

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

    throw std::invalid_argument("Unknown string given to from_string(const std::string&): " + std::string(str));
}

constexpr SimUnits from_lmp_string(const std::string_view& str)// throw()
{
    if (str == "metal")
        return SimUnits::LAMMPS_METAL;
    if (str == "real")
        return SimUnits::LAMMPS_REAL;

    throw std::invalid_argument("Unknown string given to from_lmp_string(const std::string&): " + std::string(str));
}

// In this case, we are limiting ourselves to only positions, velocities, and forces, and only with 3 unit sets with few calls. 
// If the use case becomes more complicated, it will be worth looking at proper unit libraries.

class DistanceQuantity
{
public:
    DistanceQuantity(){}
    DistanceQuantity(double value, SimUnits unit) : m_value(value), m_unit(unit){}

    void convertTo(SimUnits destUnit);

    double m_value;
    SimUnits m_unit;
};

class VelocityQuantity
{
public:
    VelocityQuantity(){}
    VelocityQuantity(double value, SimUnits unit) : m_value(value), m_unit(unit){}

    void convertTo(SimUnits destUnit);

    double m_value;
    SimUnits m_unit;
};

class ForceQuantity
{
public:
    ForceQuantity(){}
    ForceQuantity(double value, SimUnits unit) : m_value(value), m_unit(unit){}

    void convertTo(SimUnits destUnit);

    double m_value;
    SimUnits m_unit;
};

class TorqueQuantity
{
public:
    TorqueQuantity(){}
    TorqueQuantity(double value, SimUnits unit) : m_value(value), m_unit(unit){}

    void convertTo(SimUnits destUnit);

    double m_value;
    SimUnits m_unit;
};

class TimeQuantity
{
public:
    TimeQuantity(){}
    TimeQuantity(double value, SimUnits unit) : m_value(value), m_unit(unit){}

    void convertTo(SimUnits destUnit);

    double m_value;
    SimUnits m_unit;
};

static std::unordered_map<SimUnits, std::unordered_map<SimUnits, std::function<double(double)>>> g_distanceConversion = 
{
    // LAMMPS_REAL: A
    // LAMMPS_METAL: A 
    // GROMACS: nm
    {SimUnits::LAMMPS_REAL, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { return value; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { return value; }},
            {SimUnits::GROMACS, [](double value) -> double { return value * 0.1; }}
        }},

    {SimUnits::LAMMPS_METAL, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { return value; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { return value; }},
            {SimUnits::GROMACS, [](double value) -> double { return value * 0.1; }}
        }},

    {SimUnits::GROMACS, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { return value * 10.0; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { return value * 10.0; }},
            {SimUnits::GROMACS, [](double value) -> double { return value; }}
        }}
};

static std::unordered_map<SimUnits, std::unordered_map<SimUnits, std::function<double(double)>>> g_velocityConversion =
{
    // LAMMPS_REAL: A/ps (10^-12s)
    // LAMMPS_METAL: A/fs (10^-15s)
    // GROMACS: nm/ps
    {SimUnits::LAMMPS_REAL, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { return value * 1000.0; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { return value; }},
            {SimUnits::GROMACS, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs velocity is not implemented yet."); return 0.0; }}
        }},

    {SimUnits::LAMMPS_METAL, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { return value; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { return value / 1000.0; }},
            {SimUnits::GROMACS, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs velocity is not implemented yet."); return 0.0; }}
        }},

    {SimUnits::GROMACS, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs velocity is not implemented yet."); return 0.0; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs velocity is not implemented yet."); return 0.0; }},
            {SimUnits::GROMACS, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs velocity is not implemented yet."); return 0.0; }}
        }}
};

static std::unordered_map<SimUnits, std::unordered_map<SimUnits, std::function<double(double)>>> g_forceConversion = 
{
    // LAMMPS_REAL: (kcal/mol)/A
    // LAMMPS_METAL: eV/A
    // GROMACS: (kcal/mol)/nm
    {SimUnits::LAMMPS_REAL, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { return value * 0.0433641; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { return value; }},
            {SimUnits::GROMACS, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs force is not implemented yet."); return 0.0; }}
        }},

    {SimUnits::LAMMPS_METAL, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { return value; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { return value / 0.0433641; }},
            {SimUnits::GROMACS, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs force is not implemented yet."); return 0.0; }}
        }},

    {SimUnits::GROMACS, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs force is not implemented yet."); return 0.0; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs force is not implemented yet."); return 0.0; }},
            {SimUnits::GROMACS, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs force is not implemented yet."); return 0.0; }}
        }}    
};

static std::unordered_map<SimUnits, std::unordered_map<SimUnits, std::function<double(double)>>> g_torqueConversion = 
{
    // LAMMPS_REAL: (kcal/mol)
    // LAMMPS_METAL: eV
    // GROMACS: (kcal/mol)
    {SimUnits::LAMMPS_REAL, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { return value * 0.0433641; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { return value; }},
            {SimUnits::GROMACS, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs torque is not implemented yet."); return 0.0; }}
        }},

    {SimUnits::LAMMPS_METAL, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { return value; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { return value / 0.0433641; }},
            {SimUnits::GROMACS, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs torque is not implemented yet."); return 0.0; }}
        }},

    {SimUnits::GROMACS, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs torque is not implemented yet."); return 0.0; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs torque is not implemented yet."); return 0.0; }},
            {SimUnits::GROMACS, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs torque is not implemented yet."); return 0.0; }}
        }}    
};

static std::unordered_map<SimUnits, std::unordered_map<SimUnits, std::function<double(double)>>> g_timeConversion = 
{
    // LAMMPS_REAL: ps
    // LAMMPS_METAL: fs
    // GROMACS: ps
    {SimUnits::LAMMPS_REAL, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { return value / 1000.0; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { return value; }},
            {SimUnits::GROMACS, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs torque is not implemented yet."); return 0.0; }}
        }},

    {SimUnits::LAMMPS_METAL, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { return value; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { return value * 1000.0; }},
            {SimUnits::GROMACS, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs torque is not implemented yet."); return 0.0; }}
        }},

    {SimUnits::GROMACS, 
        {
            {SimUnits::LAMMPS_METAL, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs torque is not implemented yet."); return 0.0; }},
            {SimUnits::LAMMPS_REAL, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs torque is not implemented yet."); return 0.0; }},
            {SimUnits::GROMACS, [](double value) -> double { (void)value; spdlog::error("Conversion to Gromacs torque is not implemented yet."); return 0.0; }}
        }}   
};

} // core

} // radahn