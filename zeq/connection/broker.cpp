
/* Copyright (c) 2014, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#include "broker.h"
#include "../detail/socket.h"
#include "../detail/subscriber.h"
#include "../subscriber.h"

#include <boost/foreach.hpp>
#include <lunchbox/log.h>
#include <lunchbox/servus.h>
#include <map>

namespace zeq
{
namespace connection
{
namespace detail
{
class Broker
{
public:
    Broker( Subscriber& subscriber, const std::string& address, void* context )
        : _subscriber( subscriber )
        , _socket( zmq_socket( context, ZMQ_REP ))
    {
        const std::string zmqAddr( std::string( "tcp://" ) + address );
        if( zmq_bind( _socket, zmqAddr.c_str( )) != 0 )
        {
            zmq_close( _socket );
            LBTHROW( std::runtime_error(
                         "Cannot connect broker to " + address + ", got " +
                         zmq_strerror( zmq_errno( ))));
        }
    }

    ~Broker()
    {
        if( _socket )
            zmq_close( _socket );
    }

    void addSockets( std::vector< zeq::detail::Socket >& entries )
    {
        zeq::detail::Socket entry;
        entry.socket = _socket;
        entry.events = ZMQ_POLLIN;
        entries.push_back( entry );
    }

    void process( void* context, zeq::detail::Socket& socket )
    {
        zmq_msg_t msg;
        zmq_msg_init( &msg );
        zmq_msg_recv( &msg, socket.socket, 0 );
        zmq_msg_close( &msg );

        const std::string address( (const char*)zmq_msg_data( &msg ),
                                   zmq_msg_size( &msg ));
        _subscriber._impl->_addConnection( context, address );

        zmq_msg_send( &msg, socket.socket, 0 );
    }

private:
    zeq::Subscriber& _subscriber;
    void* _socket;
};
}

Broker::Broker( const std::string& address, Subscriber& subscriber )
    : Receiver( subscriber )
    , _impl( new detail::Broker( subscriber, address, getZMQContext( )))
{
}

Broker::~Broker()
{
    delete _impl;
}

void Broker::addSockets( std::vector< zeq::detail::Socket >& entries )
{
    _impl->addSockets( entries );
}

void Broker::process( zeq::detail::Socket& socket )
{
    _impl->process( getZMQContext(), socket );
}


}
}
