
/* Copyright (c) 2014-2015, Human Brain Project
 *                          Stefan.Eilemann@epfl.ch
 */

#include "service.h"
#include <zeq/publisher.h>
#include <lunchbox/log.h>
#include <lunchbox/sleep.h>
#include <zmq.h>

namespace zeq
{
namespace connection
{
bool Service::subscribe( const std::string& broker, const Publisher& publisher )
{
    void* context = zmq_ctx_new();
    void* socket = zmq_socket( context, ZMQ_REQ );
    const std::string zmqAddress = std::string("tcp://" ) + broker;
    if( zmq_connect( socket, zmqAddress.c_str( )) == -1 )
    {
        LBINFO << "Can't reach connection broker at " << broker << std::endl;
        zmq_close( socket );
        zmq_ctx_destroy( context );
        return false;
    }

    const std::string& address = publisher.getAddress();
    zmq_msg_t request;
    zmq_msg_init_size( &request, address.size( ));
    memcpy( zmq_msg_data( &request ), address.c_str(), address.size( ));

    if( zmq_msg_send( &request, socket, 0 ) == -1 )
    {
        LBINFO << "Can't send connection request " << address << " to "
               << broker << ": " << zmq_strerror( zmq_errno( )) << std::endl;
        return false;
    }
    zmq_msg_close( &request );

    zmq_msg_t reply;
    zmq_msg_init( &reply );
    if( zmq_msg_recv( &reply, socket, 0 )  == -1 )
    {
        LBINFO << "Can't receive connection reply from " << broker << std::endl;
        return false;
    }

    const std::string result( (const char*)zmq_msg_data( &reply ),
                              zmq_msg_size( &reply ));
    zmq_msg_close( &reply );

    zmq_close( socket );
    zmq_ctx_destroy( context );

    return address == std::string( result );
}

}
}
