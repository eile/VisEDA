
/* Copyright (c) 2014-2015, Human Brain Project
 *                          Daniel Nachbaur <daniel.nachbaur@epfl.ch>
 *                          Stefan.Eilemann@epfl.ch
 */

#define BOOST_TEST_MODULE zeq_pub_sub

#include "broker.h"

#include <servus/servus.h>

#include <thread>
#include <chrono>

using namespace zeq::vocabulary;

BOOST_AUTO_TEST_CASE(subscribe_to_same_schema)
{
    zeq::Publisher publisher( test::buildPublisherURI( "foo" ));
    BOOST_CHECK_NO_THROW( zeq::Subscriber subscriber(
                              test::buildURI( "foo", "localhost",
                                              zeq::detail::getRandomPort( ))));
}

BOOST_AUTO_TEST_CASE(subscribe_to_different_schema)
{
    zeq::Publisher publisher( test::buildPublisherURI( "foo" ));
    BOOST_CHECK_NO_THROW( zeq::Subscriber subscriber(
                              test::buildURI( "bar", "localhost",
                                              zeq::detail::getRandomPort( ))));
}

BOOST_AUTO_TEST_CASE(publish_receive)
{
    const unsigned short port = zeq::detail::getRandomPort();
    const servus::URI uri = test::buildURI( "foo", "localhost", port );

    zeq::Subscriber subscriber( uri );
    test::EchoIn echo;
    BOOST_CHECK( subscriber.subscribe( echo ));

    zeq::Publisher publisher( test::buildURI( "foo", "*", port ));

    bool received = false;
    for( size_t i = 0; i < 10; ++i )
    {
        BOOST_CHECK( publisher.publish( test::EchoOut( )));

        if( subscriber.receive( 100 ))
        {
            received = true;
            break;
        }
    }
    BOOST_CHECK( received );
}

BOOST_AUTO_TEST_CASE(no_receive)
{
    zeq::Subscriber subscriber( test::buildURI( "foo", "127.0.0.1",
                                                zeq::detail::getRandomPort( )));
    BOOST_CHECK( !subscriber.receive( 100 ));
}

BOOST_AUTO_TEST_CASE(subscribe_to_same_schema_zeroconf )
{
    if( !servus::Servus::isAvailable() || getenv("TRAVIS"))
        return;

    zeq::Publisher publisher( servus::URI( "foo://" ));
    BOOST_CHECK_NO_THROW(
        zeq::Subscriber subscriber( servus::URI( "foo://" )));
}

BOOST_AUTO_TEST_CASE(subscribe_to_different_schema_zeroconf)
{
    if( !servus::Servus::isAvailable() || getenv("TRAVIS"))
        return;

    zeq::Publisher publisher( servus::URI( "foo://" ));
    BOOST_CHECK_NO_THROW(
        zeq::Subscriber subscriber( servus::URI( "bar://" )));
}

BOOST_AUTO_TEST_CASE(no_receive_zeroconf)
{
    if( !servus::Servus::isAvailable() || getenv("TRAVIS"))
        return;

    zeq::Subscriber subscriber( servus::URI( "foo://" ));
    BOOST_CHECK( !subscriber.receive( 100 ));
}

BOOST_AUTO_TEST_CASE(publish_receive_zeroconf)
{
    if( !servus::Servus::isAvailable() || getenv("TRAVIS"))
        return;

    zeq::Publisher publisher( servus::URI( "foo://" ));
    zeq::Subscriber subscriber( servus::URI( "foo://" ));

    test::EchoIn echo;
    BOOST_CHECK( subscriber.subscribe( echo ));

    bool received = false;
    for( size_t i = 0; i < 20; ++i )
    {
        BOOST_CHECK( publisher.publish( test::EchoOut( )));

        if( subscriber.receive( 100 ))
        {
            received = true;
            break;
        }
    }
    BOOST_CHECK( received );
}

BOOST_AUTO_TEST_CASE(publish_receive_zeroconf_disabled)
{
    if( getenv("TRAVIS"))
        return;

    zeq::Publisher publisher( servus::URI( "foo://" ), zeq::ANNOUNCE_NONE );
    zeq::Subscriber subscriber( servus::URI( "foo://" ));

    test::EchoIn echo;
    BOOST_CHECK( subscriber.subscribe( echo ));

    bool received = false;
    for( size_t i = 0; i < 20; ++i )
    {
        BOOST_CHECK( publisher.publish( test::EchoOut( )));

        if( subscriber.receive( 100 ))
        {
            received = true;
            break;
        }
    }
    BOOST_CHECK( !received );
}

BOOST_AUTO_TEST_CASE(publish_receive_filters)
{
    zeq::Publisher publisher( servus::URI( "foo://" ), zeq::ANNOUNCE_NONE );
    zeq::Subscriber subscriber( publisher.getURI( ));
    const std::string message( 60000, 'a' );

    // Make sure we're connected
    zeq::vocabulary::Echo echoIn;
    zeq::vocabulary::Echo echoOut;
    BOOST_CHECK( subscriber.subscribe( echoIn ));
    for( size_t i = 0; i < 20; ++i )
    {
        BOOST_CHECK( publisher.publish( echoOut ));
        if( subscriber.receive( 100 ))
            break;
    }
    BOOST_CHECK( subscriber.subscribe( echoIn ));

    // benchmark with no data to be transmitted
    echoOut.setMessage( message );
    auto startTime = std::chrono::high_resolution_clock::now();
    for( size_t i = 0; i < 1000; ++i )
    {
        BOOST_CHECK( publisher.publish( echoOut ));
        while( subscriber.receive( 0 )) /* NOP to drain */;
    }
    const auto& noEchoTime = std::chrono::high_resolution_clock::now() -
                             startTime;

    // Benchmark with echo handler, now should send data
    BOOST_CHECK( subscriber.subscribe( echoIn ));

    startTime = std::chrono::high_resolution_clock::now();
    for( size_t i = 0; i < 1000; ++i )
    {
        BOOST_CHECK( publisher.publish( echoOut ));
        while( subscriber.receive( 0 )) /* NOP to drain */;
    }
    const auto& echoTime = std::chrono::high_resolution_clock::now() -
                           startTime;

    BOOST_CHECK_MESSAGE( noEchoTime < echoTime,
                         std::chrono::nanoseconds( noEchoTime ).count() << ", "
                         << std::chrono::nanoseconds( echoTime ).count( ));
}

BOOST_AUTO_TEST_CASE(publish_receive_late_zeroconf)
{
    if( !servus::Servus::isAvailable() || getenv("TRAVIS"))
        return;

    zeq::Subscriber subscriber( servus::URI( "foo://" ));
    zeq::Publisher publisher( servus::URI( "foo://" ));
    test::EchoIn echo;
    BOOST_CHECK( subscriber.subscribe( echo ));

    bool received = false;
    for( size_t i = 0; i < 20; ++i )
    {
        BOOST_CHECK( publisher.publish( test::EchoOut( )));

        if( subscriber.receive( 100 ))
        {
            received = true;
            break;
        }
    }
    BOOST_CHECK( received );
}

BOOST_AUTO_TEST_CASE(publish_receive_empty_event_zeroconf)
{
    if( !servus::Servus::isAvailable() || getenv("TRAVIS"))
        return;

    zeq::Publisher publisher( servus::URI( "foo://" ));
    zeq::Subscriber subscriber( servus::URI( "foo://" ));

    BOOST_CHECK( subscriber.registerHandler( EVENT_EXIT,
                       std::bind( &test::onExitEvent, std::placeholders::_1 )));
    bool received = false;
    const zeq::Event event( EVENT_EXIT );
    for( size_t i = 0; i < 20; ++i )
    {
        BOOST_CHECK( publisher.publish( event ));

        if( subscriber.receive( 100 ))
        {
            received = true;
            break;
        }
    }
    BOOST_CHECK( received );
}

namespace
{
class Publisher
{
public:
    Publisher()
        : running( true )
        , _publisher( servus::URI( "foo://" ))
    {}

    void run()
    {
        running = true;
        size_t i = 0;
        while( running )
        {
            BOOST_CHECK( _publisher.publish( test::EchoOut( )));
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ));
            ++i;

            if( i > 200 )
                ZEQTHROW( std::runtime_error(
                              "Publisher giving up after 20s" ));
        }
    }

    bool running;

private:
    zeq::Publisher _publisher;
};
}


BOOST_AUTO_TEST_CASE(publish_blocking_receive_zeroconf)
{
    if( !servus::Servus::isAvailable() || getenv("TRAVIS"))
        return;

    zeq::Subscriber subscriber( servus::URI( "foo://" ));
    test::EchoIn echo;
    BOOST_CHECK( subscriber.subscribe( echo ));

    Publisher publisher;
    std::thread thread( std::bind( &Publisher::run, &publisher ));
    BOOST_CHECK( subscriber.receive( ));

    publisher.running = false;
    thread.join( );
}
