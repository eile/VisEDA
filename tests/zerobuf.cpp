
/* Copyright (c) 2015, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#define BOOST_TEST_MODULE zerobuf

#include "broker.h"
#include <zeq/dictionary.h>
#include <zeq/echo.h>
#include <zeq/event.h>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(echo)
{
    zeq::vocabulary::Echo echo;

    const std::string message( "The quick brown fox" );
    echo.setMessage( message );
    BOOST_CHECK_EQUAL( echo.getMessage().empty(), false );
    BOOST_CHECK_EQUAL( echo.getMessage().size(), 19 );
    BOOST_CHECK_EQUAL( message, echo.getMessageString( ));
    BOOST_CHECK_EQUAL( message.length(), echo.getMessageString().length( ));

    zeq::vocabulary::Echo::Message echoMessage = echo.getMessage();
    BOOST_CHECK_EQUAL( echoMessage[2], 'e' );
    BOOST_CHECK_EQUAL( echoMessage.empty(), false );
    BOOST_CHECK_EQUAL( echoMessage.size(), 19 );

    echoMessage.push_back( '!' );
    BOOST_CHECK_EQUAL( echoMessage.size(), 20 );
    BOOST_CHECK_EQUAL( std::string( echoMessage.data( )), message + "!" );
    BOOST_CHECK_EQUAL( echo.getMessageString(), message + "!" );
    BOOST_CHECK_MESSAGE( echo.getZerobufSize() >= 40,
                         echo.getZerobufSize( ));

    const std::string longMessage( "So long, and thanks for all the fish!" );
    echo.setMessage( longMessage );
    BOOST_CHECK_EQUAL( echo.getMessage().empty(), false );
    BOOST_CHECK_EQUAL( echo.getMessage().size(), 37 );
    BOOST_CHECK_EQUAL( longMessage, echo.getMessageString( ));

    const std::string shortMessage( "The fox" );
    echo.setMessage( shortMessage );
    BOOST_CHECK_EQUAL( echo.getMessage().empty(), false );
    BOOST_CHECK_EQUAL( echo.getMessage().size(), 7 );
    BOOST_CHECK_EQUAL( shortMessage, echo.getMessageString( ));

    echo.setMessage( message );
    BOOST_CHECK_EQUAL( echo.getMessage().empty(), false );
    BOOST_CHECK_EQUAL( echo.getMessage().size(), 19 );
    BOOST_CHECK_EQUAL( message, echo.getMessageString( ));

    zeq::vocabulary::Echo echo2( echo );
    BOOST_CHECK_EQUAL( message, echo2.getMessageString( ));
    BOOST_CHECK_EQUAL( echo.getMessage(), echo2.getMessage( ));
    BOOST_CHECK_EQUAL( echo.getZerobufType(),
                       servus::make_uint128( "zeq::vocabulary::EchocharVector" ));
}

BOOST_AUTO_TEST_CASE(test_publish_receive)
{
    zeq::vocabulary::Echo echoOut;
    zeq::vocabulary::Echo echoIn;

    echoOut.setMessage( "The quick brown fox" );

    const unsigned short port = zeq::detail::getRandomPort();
    const servus::URI uri = test::buildURI( "foo", "localhost", port );

    zeq::Subscriber subscriber( uri );
    BOOST_CHECK( subscriber.subscribe( echoIn ));

    zeq::Publisher publisher( test::buildURI( "foo", "*", port ));

    for( size_t i = 0; i < 10; ++i )
    {
        BOOST_CHECK( publisher.publish( echoOut ));

        if( subscriber.receive( 100 ))
        {
            BOOST_CHECK_EQUAL( echoIn.getMessageString(),
                               echoOut.getMessageString( ));
            return;
        }
    }
    BOOST_CHECK( !"reachable" );
}

BOOST_AUTO_TEST_CASE(dictionary_serialization)
{
    zeq::vocabulary::Dictionary dictionary;

    zeq::EventDescriptor onlySubscribedEvent( "testName01", zeq::uint128_t( 1 ),
                                           "testSchema01", zeq::SUBSCRIBER );
    zeq::EventDescriptor onlyPublishedEvent( "testName02", zeq::uint128_t( 2 ),
                                           "testSchema02", zeq::PUBLISHER );
    zeq::EventDescriptor bidirectionalEvent( "testName03",  zeq::uint128_t( 3 ),
                                           "testSchema03", zeq::BIDIRECTIONAL );
    zeq::EventDescriptors vocabulary;
    vocabulary.push_back( std::move(onlySubscribedEvent) );
    vocabulary.push_back( std::move(onlyPublishedEvent) );
    vocabulary.push_back( std::move(bidirectionalEvent) );

    const zeq::Event& event = zeq::vocabulary::serializeVocabulary( vocabulary);
    const std::string& json = zeq::vocabulary::deserializeJSON( event );
    std::cout <<  json << std::endl;

    dictionary.readJSON( json );
}
