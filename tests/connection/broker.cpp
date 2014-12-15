
/* Copyright (c) 2014, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#include "../broker.h"
#include <zeq/connection/broker.h>

#include <lunchbox/sleep.h>
#include <lunchbox/thread.h>
#include <lunchbox/servus.h>
#include <boost/bind.hpp>

BOOST_AUTO_TEST_CASE(test_broker)
{
    lunchbox::RNG rng;
    const unsigned short port = (rng.get<uint16_t>() % 60000) + 1024;
    const std::string& portStr = boost::lexical_cast< std::string >( port );
    zeq::Subscriber subscriber( lunchbox::URI( "foo://localhost:" + portStr ));
    BOOST_CHECK( subscriber.registerHandler( zeq::vocabulary::EVENT_ECHO,
                                      boost::bind( &test::onEchoEvent, _1 )));

    // Using a different scheme so zeroconf resolution does not work
    zeq::Publisher publisher( lunchbox::URI( "bar://*:" + portStr ));

    // Using the connection broker in place of zeroconf
    const std::string& brokerPort = boost::lexical_cast< std::string >( port+1 );
    zeq::connection::Broker broker( std::string( "*:" ) + brokerPort,
                                    subscriber );
    zeq::connection::Service service;
    BOOST_CHECK( service.subscribe( std::string( "localhost:" ) + brokerPort,
                                    publisher ));

    bool received = false;
    for( size_t i = 0; i < 10; ++i )
    {
        BOOST_CHECK( publisher.publish(
                         zeq::vocabulary::serializeEcho( test::echoMessage )));

        if( subscriber.receive( 100 ))
        {
            received = true;
            break;
        }
    }
    BOOST_CHECK( received );
}
