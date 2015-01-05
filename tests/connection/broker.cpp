
/* Copyright (c) 2014-2015, Human Brain Project
 *                          Stefan.Eilemann@epfl.ch
 */

#define BOOST_TEST_MODULE zeq_connection_broker

#include "../broker.h"
#include <zeq/connection/broker.h>

#include <lunchbox/sleep.h>
#include <lunchbox/thread.h>
#include <lunchbox/servus.h>
#include <boost/bind.hpp>

using boost::lexical_cast;

const unsigned short port = (lunchbox::RNG().get<uint16_t>() % 60000) + 1024;
bool received = false;

class Service : public lunchbox::Thread
{
public:
    void run() final
    {
        zeq::Subscriber subscriber( lunchbox::URI( "foo://127.0.0.1:" +
                                          lexical_cast< std::string >( port )));
        BOOST_CHECK( subscriber.registerHandler( zeq::vocabulary::EVENT_ECHO,
                                        boost::bind( &test::onEchoEvent, _1 )));

        // Using the connection broker in place of zeroconf
        std::string address = std::string( "127.0.0.1:" ) +
                              lexical_cast<std::string>( port + 1 );
        zeq::connection::Broker broker( address, subscriber );

        for( size_t i = 0; i < 10; ++i )
        {
            if( subscriber.receive( 100 ))
            {
                received = true;
                return;
            }
        }
    }
};

BOOST_AUTO_TEST_CASE(test_broker)
{
    Service service;
    service.start();

    // Using a different scheme so zeroconf resolution does not work
    zeq::Publisher publisher( lunchbox::URI( "bar://*:" +
                                          lexical_cast< std::string >( port )));
    const std::string address =
        std::string( "127.0.0.1:" ) + lexical_cast< std::string >( port+1 );
    BOOST_CHECK( zeq::connection::Service::subscribe( address, publisher ));

    for( size_t i = 0; i < 10 && !received; ++i )
    {
        BOOST_CHECK( publisher.publish(
                         zeq::vocabulary::serializeEcho( test::echoMessage )));
        lunchbox::sleep( 100 );
    }

    BOOST_CHECK( received );
    service.join();
}
