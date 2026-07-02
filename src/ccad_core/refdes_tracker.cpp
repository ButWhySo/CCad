#include "refdes_tracker.hpp"

#include <regex>
#include <algorithm>
#include <cctype>
#include <charconv>

namespace ccad {

RefdesTracker::RefdesTracker( bool aThreadSafe ) :
    m_threadSafe( aThreadSafe ), m_reuseRefDes( true )
{
}

bool RefdesTracker::insert( const std::string& aRefDes )
{
    std::unique_lock<std::mutex> lock;
    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    return insert_impl( aRefDes );
}

bool RefdesTracker::insert_impl( const std::string& aRefDes )
{
    if( m_allRefDes.find( aRefDes ) != m_allRefDes.end() )
        return false;

    auto [prefix, number] = parse_refdes( aRefDes );

    m_allRefDes.insert( aRefDes );

    // Insert the number and update caches
    return insert_number( prefix, number );
}

bool RefdesTracker::insert_number( const std::string& aPrefix, int aNumber )
{
    PrefixData& data = m_prefixData[aPrefix];

    if( data.m_usedNumbers.find( aNumber ) != data.m_usedNumbers.end() )
        return false;

    data.m_usedNumbers.insert( aNumber );

    if( aNumber > 0 )
        update_cache_on_insert( data, aNumber );

    return true;
}

bool RefdesTracker::contains_impl( const std::string& aRefDes ) const
{
    return m_allRefDes.find( aRefDes ) != m_allRefDes.end();
}

bool RefdesTracker::contains( const std::string& aRefDes ) const
{
    std::unique_lock<std::mutex> lock;
    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    return contains_impl( aRefDes );
}

int RefdesTracker::get_next_refdes( const std::string& aPrefix, int aMinValue )
{
    std::unique_lock<std::mutex> lock;
    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    PrefixData& data = m_prefixData[aPrefix];
    int nextNum = find_next_available( data, aMinValue );

    std::string newRefDes = aPrefix + std::to_string( nextNum );
    insert_impl( newRefDes );

    return nextNum;
}

std::pair<std::string, int> RefdesTracker::parse_refdes( const std::string& aRefDes ) const
{
    if( aRefDes.empty() )
        return { "", 0 };

    size_t pos = aRefDes.size();
    while( pos > 0 && std::isdigit( static_cast<unsigned char>( aRefDes[pos - 1] ) ) )
        pos--;

    if( pos == 0 )
        return { aRefDes, 0 };

    if( pos == aRefDes.size() )
        return { aRefDes, 0 };

    int number = 0;
    const char* first = aRefDes.data() + pos;
    const char* last = aRefDes.data() + aRefDes.size();
    auto [ptr, ec] = std::from_chars( first, last, number );

    if( ec != std::errc() || ptr != last )
        return { aRefDes, 0 };

    return { aRefDes.substr( 0, pos ), number };
}

void RefdesTracker::update_base_next( PrefixData& aData ) const
{
    if( aData.m_cacheValid )
        return;

    int candidate = 1;
    for( int used : aData.m_usedNumbers )
    {
        if( used <= 0 )
            continue;
        if( used == candidate )
        {
            candidate++;
        }
        else if( used > candidate )
        {
            break;
        }
    }

    aData.m_baseNext = candidate;
    aData.m_cacheValid = true;
}

void RefdesTracker::update_cache_on_insert( PrefixData& aData, int aInsertedNumber ) const
{
    if( aData.m_cacheValid )
    {
        if( aInsertedNumber == aData.m_baseNext )
        {
            int candidate = aData.m_baseNext + 1;
            while( aData.m_usedNumbers.find( candidate ) != aData.m_usedNumbers.end() )
            {
                candidate++;
            }
            aData.m_baseNext = candidate;
        }
    }

    for( auto cacheIt = aData.m_nextCache.begin(); cacheIt != aData.m_nextCache.end(); ++cacheIt )
    {
        int cachedNext = cacheIt->second;

        if( aInsertedNumber == cachedNext )
        {
            int candidate = cachedNext + 1;
            while( aData.m_usedNumbers.find( candidate ) != aData.m_usedNumbers.end() )
                candidate++;

            cacheIt->second = candidate;
        }
    }
}

int RefdesTracker::find_next_available( const PrefixData& aData, int aMinValue ) const
{
    if( auto cacheIt = aData.m_nextCache.find( aMinValue ); cacheIt != aData.m_nextCache.end() )
        return cacheIt->second;

    update_base_next( const_cast<PrefixData&>( aData ) );

    int candidate;

    if( aMinValue <= 1 )
    {
        candidate = aData.m_baseNext;
    }
    else
    {
        candidate = aMinValue;
        while( aData.m_usedNumbers.find( candidate ) != aData.m_usedNumbers.end() )
            candidate++;
    }

    aData.m_nextCache[aMinValue] = candidate;
    return candidate;
}

std::string RefdesTracker::serialize() const
{
    std::unique_lock<std::mutex> lock;
    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    std::ostringstream result;
    bool first = true;

    for( const auto& [prefix, data] : m_prefixData )
    {
        if( !first )
            result << ",";
        first = false;

        std::string escapedPrefix = escape_for_serialization( prefix );

        std::vector<int> numbers;
        bool hasPrefix = false;

        for( int num : data.m_usedNumbers )
        {
            if( num > 0 )
                numbers.push_back( num );
            else if( num == 0 )
                hasPrefix = true;
        }

        if( numbers.empty() && !hasPrefix )
            continue;

        std::vector<std::pair<int, int>> ranges;

        if( !numbers.empty() )
        {
            int start = numbers[0];
            int end = numbers[0];

            for( size_t i = 1; i < numbers.size(); ++i )
            {
                if( numbers[i] == end + 1 )
                {
                    end = numbers[i];
                }
                else
                {
                    ranges.push_back( { start, end } );
                    start = end = numbers[i];
                }
            }
            ranges.push_back( { start, end } );
        }

        bool firstRange = true;
        for( const auto& [start, end] : ranges )
        {
            if( !firstRange )
                result << ",";
            firstRange = false;

            result << escapedPrefix;
            if( start == end )
            {
                result << start;
            }
            else
            {
                result << start << "-" << end;
            }
        }

        if( hasPrefix )
        {
            if( !firstRange )
                result << ",";
            result << escapedPrefix;
        }
    }

    return result.str();
}

bool RefdesTracker::deserialize( const std::string& aData )
{
    std::unique_lock<std::mutex> lock;
    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    clear_impl();

    if( aData.empty() )
        return true;

    auto parts = split_string( aData, ',' );

    auto parsePositiveInt = []( const std::ssub_match& aMatch, int& aOut ) -> bool
    {
        const char* first = std::to_address( aMatch.first );
        const char* last = std::to_address( aMatch.second );
        int value = 0;
        auto [ptr, ec] = std::from_chars( first, last, value );

        if( ec != std::errc() || ptr != last || value <= 0 )
            return false;

        aOut = value;
        return true;
    };

    const std::regex rangePattern( R"(^(.*\D)(\d+)-(\d+)$)" );
    const std::regex numberedPattern( R"(^(.*\D)(\d+)$)" );
    const std::regex prefixOnlyPattern( R"(^(.+)$)" );

    for( const std::string& part : parts )
    {
        std::string unescaped = unescape_from_serialization( part );
        std::smatch match;

        if( std::regex_match( unescaped, match, rangePattern ) )
        {
            std::string prefix = match[1].str();
            int start = 0;
            int end = 0;

            if( !parsePositiveInt( match[2], start ) || !parsePositiveInt( match[3], end ) )
            {
                clear_impl();
                return false;
            }

            for( int i = start; i <= end; ++i )
                insert_impl( prefix + std::to_string( i ) );
        }
        else if( std::regex_match( unescaped, match, numberedPattern ) )
        {
            std::string prefix = match[1].str();
            int number = 0;

            if( !parsePositiveInt( match[2], number ) )
            {
                clear_impl();
                return false;
            }

            insert_impl( prefix + std::to_string( number ) );
        }
        else if( std::regex_match( unescaped, match, prefixOnlyPattern ) )
        {
            std::string prefix = match[1].str();
            insert_impl( prefix );
        }
        else
        {
            clear_impl();
            return false;
        }
    }

    return true;
}

void RefdesTracker::clear()
{
    std::unique_lock<std::mutex> lock;
    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    clear_impl();
}

void RefdesTracker::clear_impl()
{
    m_prefixData.clear();
    m_allRefDes.clear();
}

size_t RefdesTracker::size() const
{
    std::unique_lock<std::mutex> lock;
    if( m_threadSafe )
        lock = std::unique_lock<std::mutex>( m_mutex );

    return m_allRefDes.size();
}

std::string RefdesTracker::escape_for_serialization( const std::string& aStr ) const
{
    std::string result;
    result.reserve( aStr.length() * 2 );

    for( char c : aStr )
    {
        if( c == '\\' || c == ',' || c == '-' )
            result += '\\';
        result += c;
    }
    return result;
}

std::string RefdesTracker::unescape_from_serialization( const std::string& aStr ) const
{
    std::string result;
    result.reserve( aStr.length() );

    bool escaped = false;
    for( char c : aStr )
    {
        if( escaped )
        {
            result += c;
            escaped = false;
        }
        else if( c == '\\' )
        {
            escaped = true;
        }
        else
        {
            result += c;
        }
    }
    return result;
}

std::vector<std::string> RefdesTracker::split_string( const std::string& aStr, char aDelimiter ) const
{
    std::vector<std::string> result;
    std::string current;
    bool escaped = false;

    for( char c : aStr )
    {
        if( escaped )
        {
            current += c;
            escaped = false;
        }
        else if( c == '\\' )
        {
            escaped = true;
            current += c;
        }
        else if( c == aDelimiter )
        {
            result.push_back( current );
            current.clear();
        }
        else
        {
            current += c;
        }
    }

    if( !current.empty() )
        result.push_back( current );

    return result;
}

} // namespace ccad
