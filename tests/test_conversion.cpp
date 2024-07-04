#include <radahn/core/units.h>
#include <spdlog/spdlog.h>

int main()
{
    radahn::core::DistanceQuantity distance = radahn::core::DistanceQuantity(1.0, radahn::core::SimUnits::LAMMPS_REAL);
    distance.convertTo(radahn::core::SimUnits::GROMACS);
    spdlog::info("Convert {} to {}.", 1.0, distance.m_value);
}