// Generated by zerobufCxx.py
#pragma once
#include <zerobuf/ConstVector.h>
#include <zerobuf/NonMovingAllocator.h>
#include <zerobuf/Vector.h>
#include <zerobuf/Zerobuf.h>

namespace zeq
{
namespace hbp
{
template< class Alloc = zerobuf::NonMovingAllocator >
class LookupTable1DBase : public zerobuf::Zerobuf
{
public:
    uint8_t* getLut();
    const uint8_t* getLut() const;
    std::vector< uint8_t> getLutVector() const;
    void setLut( uint8_t value[ 1024 ] );
    void setLut( const std::vector< uint8_t >& value );
    void setLut( const std::string& value );

    bool readJSON( const std::string& json );

    LookupTable1DBase();
    LookupTable1DBase( const LookupTable1DBase& from );
    virtual ~LookupTable1DBase() {}

    LookupTable1DBase< Alloc >& operator = ( const LookupTable1DBase& rhs );
    static bool isEmptyZerobuf() { return false; }
    static bool isStaticZerobuf() { return true; }

    virtual servus::uint128_t getZerobufType() const
    { return servus::uint128_t( 0xf0c8e1cc089e0eb5ull, 0xcb0f2f57eddb8019ull ); }

private:
};

typedef LookupTable1DBase< ::zerobuf::NonMovingAllocator > LookupTable1D;

}
}