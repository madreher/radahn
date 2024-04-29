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


constexpr const char* to_string(SimUnits u) throw()
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

// In this case, we are limiting ourselves to only positions, velocities, and forces, with few calls. 
// If the use case becomes more complicated, it will be worth looking at proper unit libraries.



} // core

} // radahn