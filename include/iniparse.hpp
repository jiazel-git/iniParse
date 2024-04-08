#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace jz {
template < typename T >
class ini_map;
namespace ini_utilty {
    const std::string white_space_delimiters = " \t\n\r\f\v";

    inline void trim( std::string& str ) {
        str.erase( str.find_last_not_of( white_space_delimiters ) + 1 );
        str.erase( 0, str.find_first_of( white_space_delimiters ) );
    }
    inline void tolower( std::string& str ) {
        std::transform( str.begin(), str.end(), str.begin(), []( const char ch ) { return static_cast< char >( std::tolower( ch ) ); } );
    }
    inline void replace( std::string& str, const std::string& src, const std::string& dest ) {
        if ( !src.empty() ) {
            std::size_t pos = 0;
            while ( ( pos = str.find( dest, pos ) ) != std::string::npos ) {
                str.replace( pos, dest.size(), dest );
                pos += dest.size();
            }
        }
    }
    enum class data_type : char { data_empty, data_comment, data_session, data_keyvalue, data_unknown };
    using data_item = std::pair< std::string, std::string >;
    inline data_type parse_line( const std::string& line, data_item& item ) {
        item.first.clear();
        item.second.clear();
        trim( const_cast< std::string& >( line ) );
        if ( line.empty() ) {
            return data_type::data_empty;
        }
        // parse comment
        if ( line[ 0 ] == ';' ) {
            return data_type::data_comment;
        }
        // parse session
        if ( line[ 0 ] == '[' ) {
            std::size_t pos = line.find( ']' );

            item.first = const_cast< std::string& >( line ).substr( 1, pos );
            trim( item.first );
            return data_type::data_session;
        }
        // parse key=value
        auto trim_line = const_cast< std::string& >( line );

        std::size_t pos = line.find( ';' );
        if ( pos != std::string::npos ) {
            trim_line = trim_line.substr( 0, pos );
        }
        pos = trim_line.find( '=' );
        if ( pos != std::string::npos ) {
            trim( trim_line );
            item.first  = trim_line.substr( 0, pos );
            item.second = trim_line.substr( pos + 1 );
            return data_type::data_keyvalue;
        }
        return data_type::data_unknown;
    }
}  // namespace ini_utilty
template < typename T >
class ini_map {
public:
    using data_index_map = std::unordered_map< std::string, std::size_t >;
    using data_item      = std::pair< std::string, T >;
    using data_continaer = std::vector< data_item >;
    using iterator       = typename data_continaer::iterator;
    using const_iterator = typename data_continaer::const_iterator;

public:
    ini_map()                            = default;
    ini_map( const ini_map& )            = delete;
    ini_map& operator=( const ini_map& ) = delete;

public:
    iterator begin() {
        return data_continaer_.begin();
    }
    iterator end() {
        return data_continaer_.end();
    }
    const_iterator cbegin() const {
        return data_continaer_.cbegin();
    }
    const_iterator cend() const {
        return data_continaer_.cend();
    }
    bool empty() const {
        return data_continaer_.empty();
    }
    bool size() const {
        return data_continaer_.size();
    }
    std::size_t set_empty( const std::string& key ) {
        std::size_t index      = data_continaer_.size();
        data_index_map_[ key ] = index;
        data_continaer_.emplace_back( key, T() );
        return index;
    }
    T& operator[]( const std::string& key ) {
        ini_utilty::trim( const_cast< std::string& >( key ) );
        ini_utilty::tolower( const_cast< std::string& >( key ) );
        auto it = data_index_map_.find( key );

        std::size_t index = it == data_index_map_.end() ? set_empty( key ) : it->second;
        return data_continaer_[ index ].second;
    }
    void clear() {
        data_index_map_.clear();
        data_continaer_.clear();
    }
    T get( const std::string& key ) const {
        ini_utilty::trim( const_cast< std::string& >( key ) );
        ini_utilty::tolower( const_cast< std::string& >( key ) );
        auto it = data_index_map_.find( key );
        if ( it == data_index_map_.end() ) {
            return T();
        }
        return data_continaer_[ it->second ];
    }
    void set( const std::string& key, const T& val ) {
        ini_utilty::trim( const_cast< std::string& >( key ) );
        ini_utilty::tolower( const_cast< std::string& >( key ) );
        auto it = data_index_map_.find( key );
        if ( it != data_index_map_.end() ) {
            data_continaer_[ it->second ] = val;
        }
        else {
            data_index_map_[ key ] = data_continaer_.size();
            data_continaer_.emplace_back( key, val );
        }
    }
    void set( std::initializer_list< data_item > il ) {
        std::for_each( il.begin(), il.end(), [ this ]( const data_item& item ) { this->set( item.first, item.second ); } );
    }

private:
    data_index_map data_index_map_;
    data_continaer data_continaer_;
};
class noncopyable {
public:
    noncopyable()                                = default;
    noncopyable( const noncopyable& )            = delete;
    noncopyable& operator=( const noncopyable& ) = delete;
};

using ini_structure = ini_map< ini_map< std::string > >;
class ini_file : public noncopyable {
public:
    using data_type   = ini_map< ini_map< std::string > >;
    using session_set = std::vector< std::string >;
    using line_data   = std::vector< std::string >;
    using line_item   = std::pair< std::string, std::string >;

public:
    explicit ini_file( const std::string& path ) : ifs_( path ) {
        if ( !ifs_.is_open() ) {
            throw std::logic_error( "open file failed" );
        }
        parse_line_data( read_file() );
    }

    ~ini_file() {}

private:
    line_data read_file() {
        ifs_.seekg( 0, std::ios::end );
        auto file_size = static_cast< std::size_t >( ifs_.tellg() );
        ifs_.seekg( 0, std::ios::beg );
        if ( file_size >= 3 ) {
            const char header[ 3 ] = { static_cast< char >( ifs_.get() ), static_cast< char >( ifs_.get() ), static_cast< char >( ifs_.get() ) };

            is_bom_ = ( header[ 0 ] == static_cast< char >( 0xEF ) && header[ 1 ] == static_cast< char >( 0xBB ) && header[ 2 ] == static_cast< char >( 0xBF ) );
        }
        std::string buffer( file_size, ' ' );
        ifs_.seekg( is_bom_ ? 3 : 0, std::ios::beg );
        ifs_.read( &buffer[ 0 ], file_size );
        ifs_.close();
        line_data ret;
        if ( buffer.empty() ) {
            return ret;
        }
        std::string line;
        for ( auto ch : buffer ) {
            if ( ch == '\n' ) {
                ret.emplace_back( line );
                line.clear();
                continue;
            }
            if ( ch != '\0' && ch != '\r' ) {
                line += ch;
            }
        }
        ret.emplace_back( line );
        return ret;
    }
    void parse_line_data( const line_data& data ) {
        if ( data.empty() ) {
            return;
        }
        line_item   item;
        std::string session;

        bool has_session = false;
        for ( auto& line : data ) {
            auto parse_ret = ini_utilty::parse_line( line, item );
            if ( parse_ret == ini_utilty::data_type::data_session ) {
                has_session = true;
                session     = item.first;

                data_[ session ];
                sesions_.emplace_back( session );
            }
            else if ( has_session && parse_ret == ini_utilty::data_type::data_keyvalue ) {
                auto key   = item.first;
                auto value = item.second;

                data_[ session ][ key ] = value;
            }
        }
    }

private:
    data_type     data_{};
    session_set   sesions_{};
    std::ifstream ifs_;

    bool is_bom_{ false };
};
}  // namespace jz