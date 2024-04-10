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
        str.erase( 0, str.find_first_not_of( white_space_delimiters ) );
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
    enum class data_type : char
    {
        data_empty,
        data_comment,
        data_session,
        data_keyvalue,
        data_unknown
    };
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

            item.first = const_cast< std::string& >( line ).substr( 1, pos - 1 );
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
    ini_map( const ini_map& )            = default;
    ini_map& operator=( const ini_map& ) = default;

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
        auto        it    = data_index_map_.find( key );
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
using ini_structure = ini_map< ini_map< std::string > >;
class ini_reader {
public:
    using ini_buffer = std::vector< std::string >;
    using line_item  = std::pair< std::string, std::string >;

public:
    explicit ini_reader( const std::string& file_name ) : ifs_( file_name, std::ios::in ), is_bom_( false ) {}
    ini_reader& operator>>( ini_structure& ini ) {
        ini_buffer  buffer = read_file_();
        line_item   item;
        bool        in_session;
        std::string session;
        for ( auto& line : buffer ) {
            if ( ini_utilty::parse_line( line, item ) == ini_utilty::data_type::data_session ) {
                in_session = true;
                ini[ session = item.first ];
                std::cout << item.first << " " << item.second << std::endl;
                continue;
            }
            if ( in_session && ini_utilty::parse_line( line, item ) == ini_utilty::data_type::data_keyvalue ) {
                auto key              = item.first;
                auto value            = item.second;
                ini[ session ][ key ] = value;
                std::cout << item.first << " " << item.second << std::endl;
                continue;
            }
        }
        return *this;
    }
    ~ini_reader() {
        if ( ifs_.is_open() ) {
            ifs_.close();
        }
    }

private:
    ini_buffer read_file_() {
        if ( !ifs_.is_open() ) {
            return ini_buffer{};
        }
        ifs_.seekg( 0, std::ios::end );
        auto file_size = static_cast< std::size_t >( ifs_.tellg() );
        ifs_.seekg( 0, std::ios::beg );
        if ( file_size >= 3 ) {
            const char header[ 3 ] = {
                static_cast< char >( ifs_.get() ),
                static_cast< char >( ifs_.get() ),
                static_cast< char >( ifs_.get() ),
            };
            is_bom_ = ( header[ 0 ] == static_cast< char >( 0xEF ) && header[ 1 ] == static_cast< char >( 0xBB ) && header[ 2 ] == static_cast< char >( 0xBF ) );
        }
        ifs_.seekg( is_bom_ ? 3 : 0, std::ios::beg );
        std::string file_contents( file_size, ' ' );
        ifs_.read( &file_contents[ 0 ], file_size );
        ini_buffer  buffer;
        std::string tmp;
        for ( auto ch : file_contents ) {
            if ( ch == '\n' ) {
                buffer.emplace_back( tmp );
                tmp.clear();
                continue;
            }
            if ( ch != '\0' && ch != '\r' ) {
                tmp += ch;
                continue;
            }
        }
        buffer.emplace_back( tmp );
        return buffer;
    }

private:
    std::ifstream ifs_;

    bool is_bom_;
};
class noncopyable {
public:
    noncopyable()                                = default;
    noncopyable( const noncopyable& )            = delete;
    noncopyable& operator=( const noncopyable& ) = delete;
};
class ini_file : public noncopyable {
public:
public:
    explicit ini_file( const std::string& file ) : file_( file ) {}
    void read( ini_structure& ini ) {
        ini_reader reader( file_ );
        reader >> ini;
    }

private:
    std::string file_;
};
}  // namespace jz