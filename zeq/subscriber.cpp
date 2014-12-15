
/* Copyright (c) 2014, Human Brain Project
 *                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>
 *                     Stefan.Eilemann@epfl.ch
 */

#include "subscriber.h"
#include "detail/subscriber.h"

namespace zeq
{
Subscriber::Subscriber( const lunchbox::URI& uri )
    : Receiver()
    , _impl( new detail::Subscriber( uri, getZMQContext( )))
{
}

Subscriber::Subscriber( const lunchbox::URI& uri, Receiver& shared )
    : Receiver( shared )
    , _impl( new detail::Subscriber( uri, getZMQContext( )))
{
}

Subscriber::~Subscriber()
{
    delete _impl;
}

bool Subscriber::registerHandler( const uint128_t& event, const EventFunc& func)
{
    return _impl->registerHandler( event, func );
}

bool Subscriber::deregisterHandler( const uint128_t& event )
{
    return _impl->deregisterHandler( event );
}

void Subscriber::addSockets( std::vector< detail::Socket >& entries )
{
    _impl->addSockets( entries );
}

void Subscriber::process( detail::Socket& socket )
{
    _impl->process( socket );
}

void Subscriber::update()
{
    _impl->update( getZMQContext( ));
}

}
