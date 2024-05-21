#pragma once

#include <vector>
#include <set>

#include <radahn/core/types.h>

namespace radahn {

namespace core {

class AtomSet 
{
public:
    AtomSet(){}
    AtomSet(const std::set<radahn::core::atomIndexes_t>& selection) : m_selection(selection), m_vecSelection(selection.begin(), selection.end()){};
    //AtomSet(const AtomSet& ref) = default;

    bool selectAtoms(radahn::core::simIt_t currentIt, const std::vector<atomIndexes_t>& indices, const std::vector<atomPositions_t>& positions);

    const std::vector<radahn::core::atomIndexes_t>& getSelectionVector() const { return m_vecSelection; }

protected:
    std::set<radahn::core::atomIndexes_t> m_selection;
    std::vector<radahn::core::atomIndexes_t> m_vecSelection;
    radahn::core::simIt_t m_currentIt;
    std::vector<radahn::core::atomIndexes_t> m_indices;
    std::vector<radahn::core::atomPositions_t> m_positions;
};

} // core

} // radahn