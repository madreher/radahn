#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

#include <radahn/core/units.h>

#include <spdlog/spdlog.h>
#include <conduit/conduit.hpp>

#include <filesystem>

namespace radahn 
{

namespace core
{

class CSVWriter
{

public:
    CSVWriter() : m_name("defaultWriter"){}
    CSVWriter(const std::string& name, char sep) : m_name(name), m_sep(sep){}

    void setName(const std::string& name) { m_name = name; }
    void setSeparator(char sep) { m_sep = sep; } 

    void declareFieldNames(const std::vector<std::string> fields)
    {
        m_fields = fields;

        m_content.clear();
        m_content<<"it";    // We always keep the iteration first in every csv. 
        for(auto & field : m_fields)
        {
            m_content<<m_sep<<field;
        }
        m_content<<"\n";
    }

    void appendFrame(radahn::core::simIt_t it, conduit::Node& node)
    {
        m_content<<it;

        for(auto & field : m_fields)
        {
            if(node.has_child(field))
            {
                // Code adjusted from conduit_utils.cpp
                conduit::Node curr = node[field];
                switch(curr.dtype().id())
                {
                    /* ints */
                    case conduit::DataType::INT8_ID:
                    {
                        int8_t val = curr.as_int8();
                        m_content<<m_sep<<val;
                        break;
                    }
                    case conduit::DataType::INT16_ID:
                    {
                        int16_t val = curr.as_int16();
                        m_content<<m_sep<<val;
                        break;
                    }
                    case conduit::DataType::INT32_ID:
                    {
                        int32_t val = curr.as_int32();
                        m_content<<m_sep<<val;
                        break;
                    }
                    case conduit::DataType::INT64_ID:
                    {
                        int64_t val = curr.as_int64();
                        m_content<<m_sep<<val;
                        break;
                    }
                    /* uints */
                    case conduit::DataType::UINT8_ID:
                    {
                        uint8_t val = curr.as_uint8();
                        m_content<<m_sep<<val;
                        break;
                    }
                    case conduit::DataType::UINT16_ID:
                    {
                        uint16_t val = curr.as_uint16();
                        m_content<<m_sep<<val;
                        break;
                    }
                    case conduit::DataType::UINT32_ID:
                    {
                        uint32_t val = curr.as_uint32();
                        m_content<<m_sep<<val;
                        break;
                    }
                    case conduit::DataType::UINT64_ID:
                    {
                        uint64_t val = curr.as_uint64();
                        m_content<<m_sep<<val;
                        break;
                    }
                    /* floats */
                    case conduit::DataType::FLOAT32_ID:
                    {
                        float val = curr.as_float32();
                        m_content<<m_sep<<val;
                        break;
                    }
                    case conduit::DataType::FLOAT64_ID:
                    {
                        double val = curr.as_float64();
                        m_content<<m_sep<<val;
                        break;
                    }
                    // string case
                    case conduit::DataType::CHAR8_STR_ID:
                    {
                        std::string val = curr.as_string();
                        m_content<<m_sep<<val;

                        break;
                    }
                    default:
                    {
                        spdlog::error("Unable to cast the field \"{}\" to a string when writting for the CSV {}.", field, m_name);
                        m_content<<m_sep<<"PARSE_ERROR";
                    }
                }
            }
            else
            {
                spdlog::warn("Unable to find the field {} in the node.", field);
                node.print();
                spdlog::warn("End print node.");
                m_content<<m_sep;
            }
        }

        m_content<<"\n";
    }

    void writeFile(const std::string& folder) const
    {
        std::string fileName = m_name + ".csv";
        std::filesystem::path fullPath = std::filesystem::path(folder) / std::filesystem::path(fileName); 
        std::ofstream csvFile(fullPath.string());
        if(csvFile.is_open())
        {
            csvFile<<m_content.str();
            csvFile.close();
        }
        else
        {
            spdlog::error("Failed to open the file {} when saving to csv.", fullPath.string());
        }
    }

protected:
    std::string m_name;
    std::vector<std::string> m_fields;
    std::stringstream m_content;
    char m_sep;
};

}

}