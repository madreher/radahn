#include <radahn/core/atomSet.h>

/*radahn::core::AtomSet::AtomSet(const AtomSet& ref)
{
    m_selection = ref.m_selection;
    m_currentIt = ref.m_currentIt;
    m_indices = ref.m_indices;
    m_positions = ref.m_positions;
}*/

bool radahn::core::AtomSet::selectAtoms(radahn::core::simIt_t currentIt, const std::vector<atomIndexes_t>& indices, const std::vector<atomPositions_t>& positions)
{
    m_indices.clear();
    m_positions.clear();

    // TODO: speed this up assuming tha the input array is already sorted
    for(uint64_t i = 0; i < indices.size(); ++i)
    {
        if(m_selection.count(indices[i]) > 0)
        {
            m_indices.push_back(indices[i]);
            m_positions.push_back(positions[3*i]);
            m_positions.push_back(positions[3*i+1]);
            m_positions.push_back(positions[3*i+2]);
            m_currentIt = currentIt;
        }
    }

    return m_indices.size() == m_selection.size();
}