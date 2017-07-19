
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
static const float TIMEOUT = 1000.f; // milliseconds

template <class R>
bool runOnce(zeroeq::Server& server, const test::Echo& request, const R& reply)
{
    bool handled = false;
    const auto func = [&](const void* data, const size_t size) {
        BOOST_CHECK((data && size) || (!data && !size));
        BOOST_CHECK(!handled);

        if (data)
        {
            test::Echo got;
            got.fromBinary(data, size);
            BOOST_CHECK_EQUAL(got, request);
        }

        handled = true;
        return zeroeq::ReplyData{R::IDENTIFIER(), reply.toBinary()};
    };

    server.handle(test::Echo::IDENTIFIER(), func);
    server.handle(test::Empty::IDENTIFIER(), func);

    BOOST_CHECK(!handled);
    BOOST_CHECK(server.receive(TIMEOUT));
    return handled;
}
}

BOOST_AUTO_TEST_CASE(serializable)
{
    test::Echo echo("The quick brown fox");
    const test::Echo reply("Jumped over the lazy dog");

    zeroeq::Server server(zeroeq::NULL_SESSION);
    zeroeq::Client client({zeroeq::URI(server.getURI())});

    std::thread thread([&] { BOOST_CHECK(runOnce(server, echo, reply)); });

    bool handled = false;
    client.request(echo, [&](const zeroeq::uint128_t& type, const void* data,
                             const size_t size) {
        BOOST_CHECK_EQUAL(type, test::Echo::IDENTIFIER());
        BOOST_CHECK(data);
        BOOST_CHECK(!handled);

        test::Echo got;
        got.fromBinary(data, size);
        BOOST_CHECK_EQUAL(got, reply);
        handled = true;
    });

    BOOST_CHECK(!handled);
    BOOST_CHECK(client.receive(TIMEOUT));
    BOOST_CHECK(handled);

    thread.join();
}

BOOST_AUTO_TEST_CASE(empty_request_raw)
{
    zeroeq::Server server(zeroeq::NULL_SESSION);
    zeroeq::Client client({zeroeq::URI(server.getURI())});
    const test::Echo reply("Jumped over the lazy dog");

    std::thread thread([&] { BOOST_CHECK(runOnce(server, {}, reply)); });

    bool handled = false;
    client.request(test::Echo::IDENTIFIER(), nullptr, 0,
                   [&](const zeroeq::uint128_t& type, const void* data,
                       const size_t size) {
                       BOOST_CHECK_EQUAL(type, test::Echo::IDENTIFIER());
                       BOOST_CHECK(data);
                       BOOST_CHECK(!handled);

                       test::Echo got;
                       got.fromBinary(data, size);
                       BOOST_CHECK_EQUAL(got, reply);
                       handled = true;
                   });

    BOOST_CHECK(!handled);
    BOOST_CHECK(client.receive(TIMEOUT));
    BOOST_CHECK(handled);

    thread.join();
}

BOOST_AUTO_TEST_CASE(empty_request_object)
{
    zeroeq::Server server(zeroeq::URI("inproc://zeroeq.test.empty_request_raw"),
                          zeroeq::NULL_SESSION);
    zeroeq::Client client({server.getURI()});
    const test::Echo reply("Jumped over the lazy dog");

    std::thread thread([&] { BOOST_CHECK(runOnce(server, {}, reply)); });

    bool handled = false;
    client.request(test::Empty(), [&](const zeroeq::uint128_t& type,
                                      const void* data, const size_t size) {
        BOOST_CHECK_EQUAL(type, test::Echo::IDENTIFIER());
        BOOST_CHECK(data);
        BOOST_CHECK(!handled);

        test::Echo got;
        got.fromBinary(data, size);
        BOOST_CHECK_EQUAL(got, reply);
        handled = true;
    });

    BOOST_CHECK(!handled);
    BOOST_CHECK(client.receive(TIMEOUT));
    BOOST_CHECK(handled);

    thread.join();
}

BOOST_AUTO_TEST_CASE(empty_reqrep)
{
    zeroeq::Server server(zeroeq::NULL_SESSION);
    zeroeq::Client client({zeroeq::URI(server.getURI())});
    const test::Empty reply;

    std::thread thread([&] { BOOST_CHECK(runOnce(server, {}, reply)); });

    bool handled = false;
    client.request(test::Echo::IDENTIFIER(), nullptr, 0,
                   [&](const zeroeq::uint128_t& type, const void* data,
                       const size_t size) {
                       BOOST_CHECK_EQUAL(type, test::Empty::IDENTIFIER());
                       BOOST_CHECK(!data);
                       BOOST_CHECK_EQUAL(size, 0);
                       BOOST_CHECK(!handled);
                       handled = true;
                   });

    BOOST_CHECK(!handled);
    BOOST_CHECK(client.receive(TIMEOUT));
    BOOST_CHECK(handled);

    thread.join();
}

BOOST_AUTO_TEST_CASE(unhandled_request)
{
    zeroeq::Server server(zeroeq::NULL_SESSION);
    zeroeq::Client client({zeroeq::URI(server.getURI())});
    const test::Empty reply;

    std::thread thread([&] { BOOST_CHECK(!runOnce(server, {}, reply)); });

    bool handled = false;
    client.request(servus::make_UUID(), nullptr, 0,
                   [&](const zeroeq::uint128_t& type, const void* data,
                       const size_t size) {
                       BOOST_CHECK_EQUAL(type, servus::uint128_t());
                       BOOST_CHECK(!data);
                       BOOST_CHECK_EQUAL(size, 0);
                       BOOST_CHECK(!handled);
                       handled = true;
                   });

    BOOST_CHECK(!handled);
    BOOST_CHECK(client.receive(TIMEOUT));
    BOOST_CHECK(handled);

    thread.join();
}
