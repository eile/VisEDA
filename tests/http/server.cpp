
/* Copyright (c) 2016, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#define BOOST_TEST_MODULE http_server

#include <zeroeq/http/server.h>
#include <zeroeq/subscriber.h>
#include <zeroeq/uri.h>
#include <zmq.h>
#include <servus/serializable.h>
#include <servus/uri.h>
#include <boost/test/unit_test.hpp>
#include <thread>

namespace
{
const std::string jsonGet( "Not JSON, just want to see that the is data a-ok" );
const std::string jsonPut( "See what my stepbrother jsonGet says" );

const std::string cors_headers(
    "Access-Control-Allow-Headers: Content-Type\r\n"
    "Access-Control-Allow-Methods: GET,PUT,OPTIONS\r\n"
    "Access-Control-Allow-Origin: *\r\n");
const std::string error400(
    "HTTP/1.0 400 Bad Request\r\n" + cors_headers +
    "Content-Length: 28\r\n\r\nHTTP/1.0 400 Bad Request\r\n\r\n" );
const std::string error404(
    "HTTP/1.0 404 Not Found\r\n" + cors_headers +
    "Content-Length: 26\r\n\r\nHTTP/1.0 404 Not Found\r\n\r\n" );
const std::string error405(
    "HTTP/1.0 405 Method Not Allowed\r\n" + cors_headers +
    "Content-Length: 35\r\n\r\nHTTP/1.0 405 Method Not Allowed\r\n\r\n" );
const std::string error411(
    "HTTP/1.0 411 Length Required\r\n" + cors_headers +
    "Content-Length: 32\r\n\r\nHTTP/1.0 411 Length Required\r\n\r\n" );

class Foo : public servus::Serializable
{
public:
    Foo() { _notified = false; }

    void setNotified() { _notified = true; }
    bool getNotified() const { return _notified; }

private:
    std::string getTypeName() const final { return "test::Foo"; }
    virtual zeroeq::uint128_t getTypeIdentifier() const final
        { return servus::make_uint128( getTypeName( )); }

    bool _fromJSON( const std::string& json ) final
    {
        return jsonPut == json;
    }

    std::string _toJSON() const final
    {
        return jsonGet;
    }

    bool _notified;
};

class Client
{
public:
    Client( const zeroeq::URI& uri )
        : _ctx( ::zmq_ctx_new ( ))
        , _socket( ::zmq_socket( _ctx, ZMQ_STREAM ))
    {
        if( ::zmq_connect( _socket, std::to_string( uri ).c_str( )) == -1 )
            throw std::runtime_error( "Connect failed" );

        // Get server identity
        BOOST_CHECK_EQUAL( zmq_getsockopt( _socket, ZMQ_IDENTITY, _id,
                                           &_idSize ), 0 );
    }

    ~Client()
    {
        if( _socket )
            ::zmq_close( _socket );
        if( _ctx )
            ::zmq_ctx_destroy( _ctx );
    }

    void sendRequest( const std::string& request )
    {
        if( ::zmq_send( _socket, _id, _idSize, ZMQ_SNDMORE ) != int(_idSize) ||
            ::zmq_send( _socket, request.c_str(), request.length(), 0 ) !=
            int( request.length( )))
        {
            throw std::runtime_error( "Send failed" );
        }
    }

    void test( std::string request, const std::string& expected, const int line,
               const unsigned nMessages = 1 )
    {
        const size_t requestSize = request.size() / nMessages;
        for( unsigned i = 1; i <= nMessages; ++i )
        {
            if( i == nMessages )
                sendRequest( request );
            else
            {
                sendRequest( request.substr( 0, requestSize ));
                request = request.substr( requestSize );
                std::this_thread::sleep_for( std::chrono::milliseconds( 100 ));
            }
        }

        std::string response;
        while( response.size() < expected.size( ))
        {
            if( ::zmq_recv( _socket, _id, _idSize, 0 ) != int( _idSize ))
                throw std::runtime_error( "Recv failed" );

            char msg[256];
            const int read = ::zmq_recv( _socket, msg, sizeof( msg ), 0 );
            BOOST_REQUIRE_GE( read, 0 );
            response.append( msg, read );
        }

        BOOST_CHECK_MESSAGE( response == expected,
                             "At l." + std::to_string( line ) + ": " +
                             response + " != " + expected);
    }

private:
    void* _ctx;
    void* _socket;

    uint8_t _id[256];
    size_t _idSize = sizeof( _id );
};

}

BOOST_AUTO_TEST_CASE(construction)
{
    zeroeq::http::Server server1;
    BOOST_CHECK_NE( server1.getURI().getHost(), "" );
    BOOST_CHECK_NE( server1.getURI().getHost(), "*" );
    BOOST_CHECK_NE( server1.getURI().getPort(), 0 );
    BOOST_CHECK_NO_THROW( server1.getSocketDescriptor( ));
    BOOST_CHECK_GT( server1.getSocketDescriptor(), 0 );

    const zeroeq::URI uri( "tcp://" );
    zeroeq::http::Server server2( uri );
    zeroeq::http::Server server3( uri );
    BOOST_CHECK_NE( server2.getURI(), server3.getURI( ));
    BOOST_CHECK_NE( server2.getURI().getPort(), 0 );

    BOOST_CHECK_THROW( zeroeq::http::Server( server2.getURI( )),
                       std::runtime_error );
    BOOST_CHECK_NO_THROW( server2.getSocketDescriptor( ));
    BOOST_CHECK_GT( server1.getSocketDescriptor(), 0 );
}

BOOST_AUTO_TEST_CASE(construction_argv_host_port)
{
    const char* app = boost::unit_test::framework::master_test_suite().argv[0];
    const char* argv[] = { app, "--zeroeq-http-server", "127.0.0.1:0" };
    const int argc = sizeof(argv)/sizeof(char*);

    std::unique_ptr< zeroeq::http::Server > server1 =
            zeroeq::http::Server::parse( argc, argv );

    BOOST_REQUIRE( server1 );
    BOOST_CHECK_EQUAL( server1->getURI().getHost(), "127.0.0.1" );
    BOOST_CHECK_NE( server1->getURI().getPort(), 0 );

    zeroeq::Subscriber shared;
    std::unique_ptr< zeroeq::http::Server > server2 =
            zeroeq::http::Server::parse( argc, argv, shared );

    BOOST_REQUIRE( server2 );
    BOOST_CHECK_EQUAL( server2->getURI().getHost(), "127.0.0.1" );
    BOOST_CHECK_NE( server2->getURI().getPort(), 0 );
}

BOOST_AUTO_TEST_CASE(construction_argv)
{
    const char* app = boost::unit_test::framework::master_test_suite().argv[0];
    const char* argv[] = { app, "--zeroeq-http-server" };
    const int argc = sizeof(argv)/sizeof(char*);

    std::unique_ptr< zeroeq::http::Server > server1 =
            zeroeq::http::Server::parse( argc, argv );

    BOOST_REQUIRE( server1 );
    BOOST_CHECK( !server1->getURI().getHost().empty( ));
    BOOST_CHECK_NE( server1->getURI().getPort(), 0 );

    zeroeq::Subscriber shared;
    std::unique_ptr< zeroeq::http::Server > server2 =
            zeroeq::http::Server::parse( argc, argv, shared );

    BOOST_CHECK( !server2->getURI().getHost().empty( ));
    BOOST_CHECK_NE( server2->getURI().getPort(), 0 );
}

BOOST_AUTO_TEST_CASE(construction_empty_argv)
{
    const char* app = boost::unit_test::framework::master_test_suite().argv[0];
    const char* argv[] = { app };
    const int argc = sizeof(argv)/sizeof(char*);

    std::unique_ptr< zeroeq::http::Server > server1 =
            zeroeq::http::Server::parse( argc, argv );

    BOOST_CHECK( !server1 );

    zeroeq::Subscriber shared;
    std::unique_ptr< zeroeq::http::Server > server2 =
            zeroeq::http::Server::parse( argc, argv, shared );

    BOOST_CHECK( !server2 );
}

BOOST_AUTO_TEST_CASE(registration)
{
    zeroeq::http::Server server;
    Foo foo;
    BOOST_CHECK( server.register_( foo ));
    BOOST_CHECK( !server.register_( foo ));
    BOOST_CHECK( server.unregister( foo ));
    BOOST_CHECK( !server.unregister( foo ));

    BOOST_CHECK( server.register_( "foo", [](){ return "bla"; } ));
    BOOST_CHECK( !server.register_( "foo", [](){ return "bla"; } ));
    BOOST_CHECK( server.unregister( "foo" ));
    BOOST_CHECK( !server.unregister( "foo" ));

    BOOST_CHECK( server.subscribe( foo ));
    BOOST_CHECK( !server.subscribe( foo ));
    BOOST_CHECK( server.unsubscribe( foo ));
    BOOST_CHECK( !server.unsubscribe( foo ));

    BOOST_CHECK( server.subscribe( "foo", zeroeq::PUTPayloadFunc
                                  ([]( const std::string& ) { return true; })));
    BOOST_CHECK( !server.subscribe( "foo", zeroeq::PUTPayloadFunc
                                  ([]( const std::string& ) { return true; })));
    BOOST_CHECK( server.unsubscribe( "foo" ));
    BOOST_CHECK( !server.unsubscribe( "foo" ));

    BOOST_CHECK( server.add( foo ));
    BOOST_CHECK( !server.add( foo ));
    BOOST_CHECK( server.remove( foo ));
    BOOST_CHECK( !server.remove( foo ));
}

BOOST_AUTO_TEST_CASE(get_serializable)
{
    bool running = true;
    zeroeq::http::Server server;
    Foo foo;

    foo.registerSerializeCallback( [&]{ foo.setNotified(); });
    server.register_( foo );

    std::thread thread( [&]() { while( running ) server.receive( 100 ); });

    Client client( server.getURI( ));

    client.test( "GET /unknown HTTP/1.0\r\n\r\n", error404, __LINE__ );
    BOOST_CHECK( !foo.getNotified( ));

    client.test( "GET /test/Foo HTTP/1.0\r\n\r\n",
                 std::string( "HTTP/1.0 200 OK\r\n" + cors_headers +
                 "Content-Length: 48\r\n\r\n" )+
                 jsonGet, __LINE__ );
    BOOST_CHECK( foo.getNotified( ));

    running = false;
    thread.join();
}

BOOST_AUTO_TEST_CASE(get_event)
{
    bool running = true;
    zeroeq::http::Server server;

    bool requested = false;
    server.register_( "test::Foo", [&](){ requested = true; return jsonGet; } );

    std::thread thread( [&]() { while( running ) server.receive( 100 ); });

    Client client( server.getURI( ));

    client.test( "GET /unknown HTTP/1.0\r\n\r\n", error404, __LINE__ );
    BOOST_CHECK( !requested );

    client.test( "GET /test/Foo HTTP/1.0\r\n\r\n",
                 std::string( "HTTP/1.0 200 OK\r\n" + cors_headers +
                 "Content-Length: 48\r\n\r\n" )+
                 jsonGet, __LINE__ );
    BOOST_CHECK( requested );

    running = false;
    thread.join();
}

BOOST_AUTO_TEST_CASE(shared)
{
    bool running = true;
    zeroeq::Subscriber subscriber;
    zeroeq::http::Server server1( subscriber );
    zeroeq::http::Server server2( server1 );
    Foo foo;
    server2.register_( foo );

    std::thread thread( [&]() { while( running ) subscriber.receive( 100 );});

    Client client1( server1.getURI( ));
    Client client2( server2.getURI( ));

    client1.test( "GET /test/Foo HTTP/1.0\r\n\r\n", error404, __LINE__ );
    client2.test( "GET /test/Foo HTTP/1.0\r\n\r\n",
                  std::string( "HTTP/1.0 200 OK\r\n" + cors_headers +
                  "Content-Length: 48\r\n\r\n" ) +
                  jsonGet, __LINE__ );
    running = false;
    thread.join();
}

BOOST_AUTO_TEST_CASE(put_serializable)
{
    bool running = true;
    zeroeq::http::Server server;
    Foo foo;

    foo.registerDeserializedCallback( [&]{ foo.setNotified(); });
    server.subscribe( foo );

    std::thread thread( [&]() { while( running ) server.receive( 100 ); });

    Client client( server.getURI( ));
    client.test( std::string( "PUT /test/Foo HTTP/1.0\r\n\r\n" ) + jsonPut,
                 error411, __LINE__ );
    client.test(
        std::string( "PUT /test/Foo HTTP/1.0\r\n" + cors_headers +
                     "Content-Length: 3\r\n\r\nFoo" ),
                     error400, __LINE__ );
    client.test( std::string( "PUT /test/Bar HTTP/1.0\r\n" +
                              cors_headers +
                              "Content-Length: " ) +
                 std::to_string( jsonPut.length( )) + "\r\n\r\n" + jsonPut,
                 error404, __LINE__ );
    BOOST_CHECK( !foo.getNotified( ));

    client.test( std::string( "PUT /test/Foo HTTP/1.0\r\n" +
                              cors_headers +
                              "Content-Length: " ) +
                 std::to_string( jsonPut.length( )) + "\r\n\r\n" + jsonPut,
                 std::string( "HTTP/1.0 200 OK\r\n" +
                              cors_headers +
                              "Content-Length: 0\r\n\r\n" ),
                 __LINE__);
    BOOST_CHECK( foo.getNotified( ));

    running = false;
    thread.join();
}


BOOST_AUTO_TEST_CASE(issue175)
{
    Foo foo;
    foo.registerDeserializedCallback( [&]{ foo.setNotified(); });

    zeroeq::http::Server server;
    server.subscribe( foo );

    bool running = true;
    std::thread thread( [&]() { while( running ) server.receive( 100 ); });

    Client client( server.getURI( ));
    client.test( std::string( "PUT /test/Foo HTTP/1.0\r\n" +
                              cors_headers +
                              "Content-Length: " ) +
                 std::to_string( jsonPut.length( )) + "\r\n\r\n" + jsonPut,
                 std::string( "HTTP/1.0 200 OK\r\n" +
                              cors_headers +
                              "Content-Length: 0\r\n\r\n" ),
                 __LINE__, 5 );
    BOOST_CHECK( foo.getNotified( ));

    running = false;
    thread.join();
}

BOOST_AUTO_TEST_CASE(put_event)
{
    bool running = true;
    zeroeq::http::Server server;

    bool receivedEmpty = false;
    server.subscribe( "empty", zeroeq::PUTFunc
                               ([&]() { receivedEmpty = true; return true; } ));
    server.subscribe( "foo", zeroeq::PUTPayloadFunc
                             ([&]( const std::string& received )
                             { return jsonPut == received; } ));

    std::thread thread( [&]() { while( running ) server.receive( 100 ); });

    Client client( server.getURI( ));
    client.test( std::string( "PUT /foo HTTP/1.0\r\n\r\n" ) + jsonPut,
                 error411, __LINE__ );
    client.test(
        std::string( "PUT /foo HTTP/1.0\r\n" + cors_headers +
                     "Content-Length: 3\r\n\r\nFoo" ),
                     error400, __LINE__ );

    client.test( std::string( "PUT /test/Bar HTTP/1.0\r\n" +
                              cors_headers +
                              "Content-Length: " ) +
                 std::to_string( jsonPut.length( )) + "\r\n\r\n" + jsonPut,
                 error404, __LINE__ );

    client.test( std::string( "PUT /foo HTTP/1.0\r\n" +
                              cors_headers +
                              "Content-Length: " ) +
                 std::to_string( jsonPut.length( )) + "\r\n\r\n" + jsonPut,
                 std::string( "HTTP/1.0 200 OK\r\n" +
                              cors_headers +
                              "Content-Length: 0\r\n\r\n" ),
                 __LINE__);

    client.test( std::string( "PUT /empty HTTP/1.0\r\n" +
                              cors_headers +
                              "Content-Length: 0" + "\r\n\r\n"),
                 std::string( "HTTP/1.0 200 OK\r\n" +
                              cors_headers +
                              "Content-Length: 0\r\n\r\n" ),
                 __LINE__);
    BOOST_CHECK( receivedEmpty );

    running = false;
    thread.join();
}

BOOST_AUTO_TEST_CASE(post)
{
    bool running = true;
    zeroeq::http::Server server;
    Foo foo;
    server.register_( foo );

    std::thread thread( [&]() { while( running ) server.receive( 100 ); });

    Client client( server.getURI( ));
    client.test( std::string( "POST /test/Foo HTTP/1.0\r\n" +
                              cors_headers +
                              "Content-Length: " ) +
                 std::to_string( jsonPut.length( )) + "\r\n\r\n" + jsonPut,
                 error405, __LINE__ );

    running = false;
    thread.join();
}

BOOST_AUTO_TEST_CASE(largeGet)
{
    bool running = true;
    zeroeq::http::Server server;
    Foo foo;
    server.register_( foo );

    std::thread thread( [&]() { while( running ) server.receive( 100 ); });

    Client client( server.getURI( ));
    client.test( "GET" + std::string( 4096, ' ' ) +
                 "/test/Foo HTTP/1.0\r\n\r\n",
                 std::string( "HTTP/1.0 200 OK\r\n" +
                              cors_headers +
                              "Content-Length: 48\r\n\r\n" )+
                 jsonGet, __LINE__ );

    running = false;
    thread.join();
}

BOOST_AUTO_TEST_CASE(issue157)
{
    bool running = true;
    zeroeq::http::Server server;
    Foo foo;
    server.register_( foo );

    std::thread thread( [&]() { while( running ) server.receive( 100 ); });

    // Close client before receiving request to provoke #157
    {
        Client client( server.getURI( ));
        client.sendRequest( "GET" + std::string( 4096, ' ' ) +
                     "/test/Foo HTTP/1.0\r\n\r\n" );
    }

    running = false;
    thread.join();
}

BOOST_AUTO_TEST_CASE(garbage)
{
    bool running = true;
    zeroeq::http::Server server;
    std::thread thread( [&]() { while( running ) server.receive( 100 ); });

    Client client( server.getURI( ));
    client.test( "ramble mumble foo bar", "", __LINE__ );

    running = false;
    thread.join();
}

BOOST_AUTO_TEST_CASE(urlcasesensitivity)
{
    bool running = true;
    zeroeq::http::Server server;
    Foo foo;
    server.register_( foo );

    std::thread thread( [&]() { while( running ) server.receive( 100 ); });

    Client client( server.getURI( ));
    client.test( "GET" + std::string( 4096, ' ' ) +
                 "/TEST/FOO HTTP/1.0\r\n\r\n",
                 std::string( "HTTP/1.0 200 OK\r\n" +
                              cors_headers +
                              "Content-Length: 48\r\n\r\n" ) +
                 jsonGet, __LINE__ );

    client.test( "GET" + std::string( 4096, ' ' ) +
                 "/test/foo HTTP/1.0\r\n\r\n",
                 std::string( "HTTP/1.0 200 OK\r\n" +
                              cors_headers +
                              "Content-Length: 48\r\n\r\n" ) +
                 jsonGet, __LINE__ );

    running = false;
    thread.join();
}
