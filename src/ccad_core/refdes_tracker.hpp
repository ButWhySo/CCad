#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <mutex>
#include <set>
#include <sstream>
#include <map>
#include <vector>
#include <functional>
#include <algorithm>

namespace ccad {

/**
 * Class to efficiently track reference designators and provide next available designators.
 *
 * Maintains internal data structures for O(1) lookup of existing designators and efficient
 * retrieval of next available numerical suffixes for any given prefix.
 */
class RefdesTracker
{
public:
    /**
     * Constructor.
     *
     * @param aThreadSafe if true, enables mutex locking for thread-safe operation
     */
    explicit RefdesTracker( bool aThreadSafe = false );

    /**
     * Insert a reference designator into the tracker.
     *
     * @param aRefDes the reference designator to insert
     * @return true if inserted successfully, false if already exists
     */
    bool insert( const std::string& aRefDes );

    /**
     * Check if a reference designator exists in the tracker.
     *
     * @param aRefDes the reference designator to check
     * @return true if the reference designator exists
     */
    bool contains( const std::string& aRefDes ) const;

    /**
     * Get the next available reference designator for a given prefix and reserve it.
     *
     * The returned designator is automatically inserted into the tracker.
     *
     * @param aPrefix the alphabetic prefix (e.g., "R", "C", "IC")
     * @param aMinValue the minimum numerical value to use (default 1)
     * @return the next available number for the given prefix (now reserved)
     */
    int get_next_refdes( const std::string& aPrefix, int aMinValue = 1 );

    /**
     * Serialize the tracker data to a compact string representation.
     *
     * Uses range notation for consecutive numbers (e.g., "R1-3,R5-7,R10").
     *
     * @return serialized string representation
     */
    std::string serialize() const;

    /**
     * Deserialize tracker data from string representation.
     *
     * @param aData the serialized data string
     * @return true if deserialization was successful
     */
    bool deserialize( const std::string& aData );

    /**
     * Clear all stored reference designators.
     */
    void clear();

    /**
     * Get the total count of stored reference designators.
     *
     * @return number of reference designators stored
     */
    size_t size() const;

    bool get_reuse_refdes() const { return m_reuseRefDes; }
    void set_reuse_refdes( bool aReuse ) { m_reuseRefDes = aReuse; }

private:
    /**
     * Data structure for tracking used numbers and caching next available values.
     */
    struct PrefixData
    {
        std::set<int>              m_usedNumbers; ///< Sorted set of used numbers for this prefix
        mutable std::map<int, int> m_nextCache;   ///< Cache of next available number for given min values
        mutable int                m_baseNext;    ///< Next available from 1 (cached)
        mutable bool               m_cacheValid;  ///< True if m_baseNext cache is valid

        PrefixData() : m_baseNext( 1 ), m_cacheValid( false ) {}
    };

    mutable std::mutex      m_mutex;            ///< Mutex for thread safety
    bool                    m_threadSafe;       ///< True if thread safety is enabled

    /// Map from prefix to its tracking data
    std::unordered_map<std::string, PrefixData> m_prefixData;
    std::unordered_set<std::string>             m_allRefDes;

    bool m_reuseRefDes; ///< If true, allows reusing existing reference designators

    /**
     * Internal implementation of insert without locking.
     *
     * @param aRefDes reference designator to insert
     * @return true if inserted, false if already exists
     */
    bool insert_impl( const std::string& aRefDes );

    /**
     * Clear all internal data structures without locking.
     *
     * This is used internally to reset the tracker state.
     */
    void clear_impl();

    /**
     * Check if a reference designator exists in the tracker without locking.
     *
     * @param aRefDes reference designator to check
     * @return true if the reference designator exists
     */
    bool contains_impl( const std::string& aRefDes ) const;

    /**
     * Parse a reference designator into prefix and numerical suffix.
     *
     * @param aRefDes the reference designator to parse
     * @return pair of (prefix, number) where number is 0 if no numerical suffix
     */
    std::pair<std::string, int> parse_refdes( const std::string& aRefDes ) const;

    /**
     * Update cached next available values when a number is inserted.
     *
     * @param aData the prefix data to update
     * @param aInsertedNumber the number that was just inserted
     */
    void update_cache_on_insert( PrefixData& aData, int aInsertedNumber ) const;

    /**
     * Find next available number for a prefix starting from a minimum value.
     *
     * @param aData the prefix data
     * @param aMinValue minimum value to start search from
     * @return next available number >= aMinValue
     */
    int find_next_available( const PrefixData& aData, int aMinValue ) const;

    /**
     * Insert a number for a specific prefix, updating internal structures.
     *
     * @param aPrefix the prefix
     * @param aNumber the number to insert (0 for prefix-only)
     * @return true if inserted, false if already exists
     */
    bool insert_number( const std::string& aPrefix, int aNumber );

    /**
     * Escape special characters for serialization.
     *
     * @param aStr string to escape
     * @return escaped string
     */
    std::string escape_for_serialization( const std::string& aStr ) const;

    /**
     * Unescape special characters from serialization.
     *
     * @param aStr escaped string
     * @return unescaped string
     */
    std::string unescape_from_serialization( const std::string& aStr ) const;

    /**
     * Split string by delimiter, handling escaped characters.
     *
     * @param aStr string to split
     * @param aDelimiter delimiter character
     * @return vector of split parts
     */
    std::vector<std::string> split_string( const std::string& aStr, char aDelimiter ) const;

    void update_base_next( PrefixData& aData ) const;
};

} // namespace ccad
