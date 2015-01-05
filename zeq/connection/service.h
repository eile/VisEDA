
/* Copyright (c) 2014-2015, Human Brain Project
 *                          Stefan.Eilemann@epfl.ch
 */

#ifndef ZEQ_CONNECTION_SERVICE_H
#define ZEQ_CONNECTION_SERVICE_H

#include <zeq/api.h>
#include <zeq/types.h>
#include <boost/noncopyable.hpp>
#include <string>

namespace zeq
{
namespace connection
{

/**
 * Services subscription requests for a zeq::Subscriber.
 *
 * Example: @include tests/connection/broker.cpp
 */
class Service : public boost::noncopyable
{
public:
    /**
     * Request subscription of the given publisher to a remote broker.
     *
     * Upon success, the Broker will add the publisher's address to its managed
     * Subscriber.
     *
     * @param broker the broker address, without the protocol.
     * @param publisher the publisher to subscribe to.
     *
     * @return
     */
    ZEQ_API static bool subscribe( const std::string& broker,
                                   const Publisher& publisher );
};

}
}
#endif
