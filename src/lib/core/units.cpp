#include <radahn/core/units.h>
#include <spdlog/spdlog.h>

void radahn::core::DistanceQuantity::convertTo(SimUnits destUnit)
{
    m_value = g_distanceConversion[m_unit][destUnit](m_value);
    m_unit = destUnit;
}

void radahn::core::VelocityQuantity::convertTo(SimUnits destUnit)
{
    m_value = g_velocityConversion[m_unit][destUnit](m_value);
    m_unit = destUnit;
}

void radahn::core::ForceQuantity::convertTo(SimUnits destUnit)
{
    m_value = g_forceConversion[m_unit][destUnit](m_value);
    m_unit = destUnit;
}

void radahn::core::TorqueQuantity::convertTo(SimUnits destUnit)
{
    m_value = g_torqueConversion[m_unit][destUnit](m_value);
    m_unit = destUnit;
}
    

void radahn::core::TimeQuantity::convertTo(SimUnits destUnit)
{
    m_value = g_timeConversion[m_unit][destUnit](m_value);
    m_unit = destUnit;
}