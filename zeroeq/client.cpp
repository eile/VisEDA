
/* Copyright (c) 2017, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#include "client.h"

#include "detail/browser.h"
#include "detail/common.h"
#include "detail/context.h"

#include <servus/servus.h>
#include <unordered_map>

namespace zeroeq
{
class Client::Impl : public detail::Browser
{
public:
    Impl(const std::string& session)
        : Browser(SERVER_SERVICE,
                  session == DEFAULT_SESSION ? getDefaultAppSession() : session)
        , _context(detail::getContext())
        , _servers(zmq_socket(_context.get(), ZMQ_DEALER),
                   [](void* s) { ::zmq_close(s); })
    {
        if (session == zeroeq::NULL_SESSION || session.empty())
            ZEROEQTHROW(std::runtime_error(
                std::string("Invalid session name for client")));
    }

    Impl(const URIs& uris)
        : Browser(SERVER_SERVICE, {})
        , _context(detail::getContext())
        , _servers(zmq_socket(_context.get(), ZMQ_DEALER),
                   [](void* s) { ::zmq_close(s); })
    {
        for (const auto& uri : uris)
        {
            if (uri.getScheme() == DEFAULT_SCHEMA &&
                (uri.getHost().empty() || uri.getPort() == 0))
            {
                ZEROEQTHROW(std::runtime_error(
                    std::string("Non-fully qualified URI used for server")));
            }

            const auto& zmqURI = buildZmqURI(uri);
            if (!addConnection(zmqURI, uint128_t()))
            {
                ZEROEQTHROW(std::runtime_error("Cannot connect client to " +
                                               zmqURI + ": " +
                                               zmq_strerror(zmq_errno())));
            }
        }
    }

    ~Impl() {}
    bool addConnection(const std::string& zmqURI, const uint128_t&) final
    {
        return detail::Browser::addConnection(zmqURI, _servers);
    }

    bool request(uint128_t requestID, const void* data, size_t size,
                 const ReplyFunc& func)
    {
        const bool hasPayload = data && size > 0;
        auto id = servus::make_UUID();
#ifdef ZEROEQ_BIGENDIAN
        detail::byteswap(requestID); // convert to little endian wire protocol
#endif

        if (!_send(&id, sizeof(id), ZMQ_SNDMORE) ||
            !_send(nullptr, 0, ZMQ_SNDMORE) || // frame delimiter
            !_send(&requestID, sizeof(requestID), hasPayload ? ZMQ_SNDMORE : 0))
        {
            return false;
        }

        if (hasPayload && !_send(data, size, 0))
            return false;

        _handlers[id] = func;
        return true;
    }

    bool process(detail::Socket& socket)
    {
        uint128_t id;
        uint128_t replyID;

        if (!_recv(&id, sizeof(id)) || !_recv(nullptr, 0))
            return false;
        const bool payload = _recv(&replyID, sizeof(replyID));

#ifdef ZEROEQ_BIGENDIAN
        detail::byteswap(replyID); // convert to little endian wire protocol
#endif

        zmq_msg_t msg;
        if (payload)
        {
            zmq_msg_init(&msg);
            zmq_msg_recv(&msg, socket.socket, 0);
        }

        auto i = _handlers.find(id);
        if (i == _handlers.cend())
        {
            if (payload)
                zmq_msg_close(&msg);

            ZEROEQTHROW(
                std::runtime_error("Got unrequested reply " + id.getString()));
        }

        if (payload)
        {
            i->second(replyID, zmq_msg_data(&msg), zmq_msg_size(&msg));
            zmq_msg_close(&msg);
        }
        else
            i->second(replyID, nullptr, 0);
        return true;
    }

private:
    bool _send(const void* data, const size_t size, const int flags)
    {
        zmq_msg_t msg;
        zmq_msg_init_size(&msg, size);
        ::memcpy(zmq_msg_data(&msg), data, size);
        int ret = zmq_msg_send(&msg, _servers.get(), flags);
        zmq_msg_close(&msg);

        if (ret != -1)
            return true;

        ZEROEQWARN << "Cannot send request, got " << zmq_strerror(zmq_errno())
                   << std::endl;
        return false;
    }

    /** @return true if more data available */
    bool _recv(void* data, const size_t size)
    {
        zmq_msg_t msg;
        zmq_msg_init(&msg);
        zmq_msg_recv(&msg, _servers.get(), 0);

        if (zmq_msg_size(&msg) != size)
            ZEROEQWARN << "Message size mismatch, expected " << size << " got "
                       << zmq_msg_size(&msg) << std::endl;
        else if (data)
            ::memcpy(data, zmq_msg_data(&msg), size);
        const bool more = zmq_msg_more(&msg);
        zmq_msg_close(&msg);
        return more;
    }

    zmq::ContextPtr _context;
    zmq::SocketPtr _servers;
    std::unordered_map<uint128_t, ReplyFunc> _handlers;
};

Client::Client()
    : Receiver()
    , _impl(new Impl(DEFAULT_SESSION))
{
}

Client::Client(const std::string& session)
    : Receiver()
    , _impl(new Impl(session))
{
}

Client::Client(const URIs& uris)
    : Receiver()
    , _impl(new Impl(uris))
{
}

Client::Client(Receiver& shared)
    : Receiver(shared)
    , _impl(new Impl(DEFAULT_SESSION))
{
}

Client::Client(const std::string& session, Receiver& shared)
    : Receiver(shared)
    , _impl(new Impl(session))
{
}

Client::Client(const URIs& uris, Receiver& shared)
    : Receiver(shared)
    , _impl(new Impl(uris))
{
}

Client::~Client()
{
}

bool Client::request(const servus::Serializable& req, const ReplyFunc& func)
{
    const auto& data = req.toBinary();
    return request(req.getTypeIdentifier(), data.ptr.get(), data.size, func);
}

bool Client::request(const uint128_t& requestID, const void* data, size_t size,
                     const ReplyFunc& func)
{
    return _impl->request(requestID, data, size, func);
}

const std::string& Client::getSession() const
{
    return _impl->getSession();
}

void Client::addSockets(std::vector<detail::Socket>& entries)
{
    _impl->addSockets(entries);
}

bool Client::process(detail::Socket& socket)
{
    return _impl->process(socket);
}

void Client::update()
{
    _impl->update();
}

void Client::addConnection(const std::string& uri)
{
    _impl->addConnection(uri, uint128_t());
}
}
