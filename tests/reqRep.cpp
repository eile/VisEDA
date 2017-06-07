
/* Copyright (c) 2017, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#define BOOST_TEST_MODULE zeroeq_req_rep

#include "common.h"

#include <servus/servus.h>
#include <servus/uri.h>

#include <chrono>
#include <thread>

namespace
{
static const float TIMEOUT = 100.f; // milliseconds

void runOnce(zeroeq::Server& server, const test::Echo& expected)
{
    bool handled = false;
    server.handle(test::Echo::IDENTIFIER(),
                  [&](const void* data, const size_t size) {
                      BOOST_CHECK(data);
                      BOOST_CHECK(!handled);

                      const auto got = test::Echo::create(data, size);
                      BOOST_CHECK_EQUAL(got, expected);

                      handled = true;
                      return test::Echo("Jumped over the lazy dog").toBinary();
                  });

    BOOST_CHECK(!handled);
    server.receive(TIMEOUT);
    BOOST_CHECK(handled);
}

bool running;
};
}

BOOST_AUTO_TEST_CASE(serializable)
{
    test::Echo echo("The quick brown fox");

    zeroeq::Server server(zeroeq::NULL_SESSION);
    zeroeq::Client client(zeroeq::URI(server.getURI()));

    std::thread thread([&] { runOnce(server, echo); });

    bool handled = false;
    client.get(echo.getTypeIdentifier(), [&](const void* data,
                                             const size_t size) {
        BOOST_CHECK(data);
        BOOST_CHECK(!handled);

        const auto got = test::Echo::create(data, size);
        BOOST_CHECK_EQUAL(got.getMessage(), "Jumped over the lazy dog");
        handled = true;
    });

    BOOST_CHECK(!handled);
    client.receive(TIMEOUT);
    BOOST_CHECK(handled);
}

#if 0
BOOST_AUTO_TEST_CASE(no_receive)
{
    zeroeq::Server server(zeroeq::URI("1.2.3.4:1234"));
    BOOST_CHECK(!server.receive(100));
}

BOOST_AUTO_TEST_CASE(subscribe_to_same_session_zeroconf)
{
    if (!servus::Servus::isAvailable() || getenv("TRAVIS"))
        return;

    zeroeq::Client client(test::buildUniqueSession());
    BOOST_CHECK_NO_THROW(zeroeq::Server server(client.getSession()));
}

BOOST_AUTO_TEST_CASE(subscribe_to_different_session_zeroconf)
{
    if (!servus::Servus::isAvailable() || getenv("TRAVIS"))
        return;

    zeroeq::Client client(test::buildUniqueSession());
    BOOST_CHECK_NO_THROW(zeroeq::Server server(client.getSession() + "bar"));
}

BOOST_AUTO_TEST_CASE(no_receive_zeroconf)
{
    if (!servus::Servus::isAvailable() || getenv("TRAVIS"))
        return;

    zeroeq::Server server(test::buildUniqueSession());
    BOOST_CHECK(!server.receive(100));
}

BOOST_AUTO_TEST_CASE(zeroconf)
{
    if (!servus::Servus::isAvailable() || getenv("TRAVIS"))
        return;

    zeroeq::Client client(test::buildUniqueSession());
    zeroeq::Server noServer(client.getSession());
    zeroeq::detail::Sender::getUUID() =
        servus::make_UUID(); // different machine
    zeroeq::Server server(client.getSession());

    BOOST_CHECK(server.subscribe(test::Echo::IDENTIFIER(),
                                 zeroeq::EventPayloadFunc(&test::onEchoEvent)));
    BOOST_CHECK(
        noServer.subscribe(test::Echo::IDENTIFIER(),
                           zeroeq::EventPayloadFunc(&test::onEchoEvent)));

    bool received = false;
    for (size_t i = 0; i < 20; ++i)
    {
        BOOST_CHECK(client.publish(test::Echo(test::echoMessage)));

        BOOST_CHECK(!noServer.receive(100));
        if (server.receive(100))
        {
            received = true;
            break;
        }
    }
    BOOST_CHECK(received);
}

BOOST_AUTO_TEST_CASE(late_zeroconf)
{
    if (!servus::Servus::isAvailable() || getenv("TRAVIS"))
        return;

    zeroeq::Server server(test::buildUniqueSession());
    zeroeq::detail::Sender::getUUID() =
        servus::make_UUID(); // different machine
    zeroeq::Client client(server.getSession());

    BOOST_CHECK(server.subscribe(test::Echo::IDENTIFIER(),
                                 zeroeq::EventPayloadFunc(&test::onEchoEvent)));
    bool received = false;
    for (size_t i = 0; i < 20; ++i)
    {
        BOOST_CHECK(client.publish(test::Echo(test::echoMessage)));

        if (server.receive(100))
        {
            received = true;
            break;
        }
    }
    BOOST_CHECK(received);
}

BOOST_AUTO_TEST_CASE(empty_event_zeroconf)
{
    if (!servus::Servus::isAvailable() || getenv("TRAVIS"))
        return;

    zeroeq::Client client(test::buildUniqueSession());
    zeroeq::detail::Sender::getUUID() =
        servus::make_UUID(); // different machine
    zeroeq::Server server(client.getSession());

    BOOST_CHECK(
        server.subscribe(test::Empty::IDENTIFIER(), zeroeq::EventFunc([] {})));

    bool received = false;
    for (size_t i = 0; i < 20; ++i)
    {
        BOOST_CHECK(client.publish(test::Empty()));

        if (server.receive(100))
        {
            received = true;
            break;
        }
    }
    BOOST_CHECK(received);
}

namespace
{
class Client
{
public:
    Client()
        : running(false)
    {
    }

    void run(const std::string& session)
    {
        zeroeq::Client client(session);
        running = true;
        size_t i = 0;
        while (running)
        {
            BOOST_CHECK(client.publish(test::Echo(test::echoMessage)));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ++i;

            if (i > 300)
                ZEROEQTHROW(std::runtime_error("Client giving up after 30s"));
        }
    }

    bool running;
};
}

BOOST_AUTO_TEST_CASE(publish_blocking_receive_zeroconf)
{
    if (!servus::Servus::isAvailable() || getenv("TRAVIS"))
        return;

    zeroeq::Server server(test::buildUniqueSession());
    zeroeq::detail::Sender::getUUID() =
        servus::make_UUID(); // different machine

    BOOST_CHECK(server.subscribe(test::Echo::IDENTIFIER(),
                                 zeroeq::EventPayloadFunc(&test::onEchoEvent)));

    Client client;
    std::thread thread(std::bind(&Client::run, &client, server.getSession()));

    BOOST_CHECK(server.receive());

    client.running = false;
    thread.join();
}
#endif
