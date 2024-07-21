#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <sstream>

#include <radahn/core/units.h>

#include <spdlog/spdlog.h>
#include <conduit/conduit.hpp>

#include <filesystem>

namespace radahn 
{

namespace core
{

// Not using inheritance with CSVWriter here just because almost all the internal would change, it's not worth the cost
class DynamicCSVWriter
{

public:
    DynamicCSVWriter() : m_name("defaultDynamicWriter"){}
    DynamicCSVWriter(const std::string& name, char sep) : m_name(name), m_sep(sep){}

    void setName(const std::string& name) { m_name = name; }
    void setSeparator(char sep) { m_sep = sep; } 

    void appendFrame(radahn::core::simIt_t it, conduit::Node& node)
    {
        //m_content<<it;

        dynamicCSVFrame frame;

        auto & listFields = node.child_names();

        for(auto & field :listFields)
        {
            m_fields.insert(field);

            // Code adjusted from conduit_utils.cpp
            conduit::Node curr = node[field];

            switch(curr.dtype().id())
            {
                /* ints */
                case conduit::DataType::INT8_ID:
                {
                    int8_t val = curr.as_int8();
                    //m_content<<m_sep<<val;
                    frame.insert({field, std::to_string(val)});
                    break;
                }
                case conduit::DataType::INT16_ID:
                {
                    int16_t val = curr.as_int16();
                    //m_content<<m_sep<<val;
                    frame.insert({field, std::to_string(val)});
                    break;
                }
                case conduit::DataType::INT32_ID:
                {
                    int32_t val = curr.as_int32();
                    //m_content<<m_sep<<val;
                    frame.insert({field, std::to_string(val)});
                    break;
                }
                case conduit::DataType::INT64_ID:
                {
                    int64_t val = curr.as_int64();
                    //m_content<<m_sep<<val;
                    frame.insert({field, std::to_string(val)});
                    break;
                }
                /* uints */
                case conduit::DataType::UINT8_ID:
                {
                    uint8_t val = curr.as_uint8();
                    //m_content<<m_sep<<val;
                    frame.insert({field, std::to_string(val)});
                    break;
                }
                case conduit::DataType::UINT16_ID:
                {
                    uint16_t val = curr.as_uint16();
                    //m_content<<m_sep<<val;
                    frame.insert({field, std::to_string(val)});
                    break;
                }
                case conduit::DataType::UINT32_ID:
                {
                    uint32_t val = curr.as_uint32();
                    //m_content<<m_sep<<val;
                    frame.insert({field, std::to_string(val)});
                    break;
                }
                case conduit::DataType::UINT64_ID:
                {
                    uint64_t val = curr.as_uint64();
                    //m_content<<m_sep<<val;
                    frame.insert({field, std::to_string(val)});
                    break;
                }
                /* floats */
                case conduit::DataType::FLOAT32_ID:
                {
                    float val = curr.as_float32();
                    //m_content<<m_sep<<val;
                    frame.insert({field, std::to_string(val)});
                    break;
                }
                case conduit::DataType::FLOAT64_ID:
                {
                    double val = curr.as_float64();
                    //m_content<<m_sep<<val;
                    frame.insert({field, std::to_string(val)});
                    break;
                }
                // string case
                case conduit::DataType::CHAR8_STR_ID:
                {
                    std::string val = curr.as_string();
                    //m_content<<m_sep<<val;
                    frame.insert({field, val});

                    break;
                }
                default:
                {
                    spdlog::error("Unable to cast the field \"{}\" to a string when writting for the CSV {}.", field, m_name);
                    //m_content<<m_sep<<"PARSE_ERROR";
                    frame.insert({field, "PARSE_ERROR"});
                }
            }
        }

        // All the frame has been converted to string, now we can insert the frame
        m_frames.insert({it, frame});
    }

    void writeFile(const std::string& folder) const
    {
        std::string fileName = m_name + ".csv";
        std::filesystem::path fullPath = std::filesystem::path(folder) / std::filesystem::path(fileName); 
        std::ofstream csvFile(fullPath.string());
        if(csvFile.is_open())
        {
            //csvFile<<m_content.str();
            //csvFile.close();

            auto cleanFields(m_fields);
            cleanFields.erase("simIt"); // Might want to remove other fields which are duplicated.

            // Writting the header
            csvFile<<"simIt";
            for(auto & key : cleanFields)
            {
                csvFile<<m_sep<<key;
            }
            csvFile<<"\n";

            // Going through the different frames
            for(auto & [k, v] : m_frames)
            {
                csvFile<<k;

                for(auto & field : cleanFields)
                {
                    auto entry = v.find(field);
                    if(entry != v.end())
                    {
                        csvFile<<m_sep<<entry->second;
                    }
                    else 
                        csvFile<<m_sep;
                }
                csvFile<<"\n";
            }

            csvFile.close();
        }
        else
        {
            spdlog::error("Failed to open the file {} when saving to csv.", fullPath.string());
        }
    }

protected:
    typedef std::unordered_map<std::string, std::string> dynamicCSVFrame;

    std::string m_name;
    std::set<std::string> m_fields;
    std::map<radahn::core::simIt_t, dynamicCSVFrame> m_frames;
    //std::stringstream m_content;
    char m_sep;
};

}

}