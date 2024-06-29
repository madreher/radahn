#include <radahn/core/units.h>
#include <spdlog/spdlog.h>

double radahn::core::DistanceQuantity::convertTo(SimUnits destUnit) const
{
    return g_distanceConversion[m_unit][destUnit](m_value);
}

double radahn::core::VelocityQuantity::convertTo(SimUnits destUnit) const
{
    return g_velocityConversion[m_unit][destUnit](m_value);
}

double radahn::core::ForceQuantity::convertTo(SimUnits destUnit) const
{
    return g_forceConversion[m_unit][destUnit](m_value);
}

double radahn::core::TorqueQuantity::convertTo(SimUnits destUnit) const
{
    return g_torqueConversion[m_unit][destUnit](m_value);
}
    

double radahn::core::TimeQuantity::convertTo(SimUnits destUnit) const
{
    return g_timeConversion[m_unit][destUnit](m_value);
}