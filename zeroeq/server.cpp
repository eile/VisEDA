
/* Copyright (c) 2017, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#include "server.h"

#include "detail/browser.h"
#include "detail/sender.h"

#include <cassert>
#include <unordered_map>

namespace zeroeq
{
class Server::Impl : public detail::Sender
{
public:
    Impl(const URI& uri_, const std::string& session)
        : detail::Sender(uri_, ZMQ_REP, SERVER_SERVICE,
                         session == DEFAULT_SESSION ? getDefaultAppSession()
                                                    : session)
    {
        if (session.empty())
            ZEROEQTHROW(
                std::runtime_error("Empty session is not allowed for server"));

        const std::string& zmqURI = buildZmqURI(uri);
        if (zmq_bind(socket.get(), zmqURI.c_str()) == -1)
            ZEROEQTHROW(
                std::runtime_error(std::string("Cannot bind server socket '") +
                                   zmqURI + "': " + zmq_strerror(zmq_errno())));

        initURI();
        if (session != NULL_SESSION)
            announce();
    }

    ~Impl() {}
    bool handle(const uint128_t& request, const HandleFunc& func)
    {
        if (_handlers.find(request) != _handlers.end())
            return false;

        _handlers[request] = func;
        return true;
    }

    bool process(detail::Socket&)
    {
        uint128_t requestID;
        const bool payload = _recv(&requestID, sizeof(requestID));

#ifdef ZEROEQ_BIGENDIAN
        detail::byteswap(requestID); // convert from little endian wire protocol
#endif

        zmq_msg_t msg;
        if (payload)
        {
            zmq_msg_init(&msg);
            zmq_msg_recv(&msg, socket.get(), 0);
        }

        auto i = _handlers.find(requestID);
        if (i == _handlers.cend()) // no handler, return "0"
        {
            const uint128_t zero;
            _send(&zero, sizeof(zero), 0); // request and reply, no playload
        }
        else
        {
            auto reply = payload
                             ? i->second(zmq_msg_data(&msg), zmq_msg_size(&msg))
                             : i->second(nullptr, 0);
            const bool hasReplyData = reply.second.ptr && reply.second.size;
#ifdef ZEROEQ_BIGENDIAN
            detail::byteswap(reply.first); // convert to little endian
#endif
            if (!_send(&reply.first, sizeof(reply.first),
                       hasReplyData ? ZMQ_SNDMORE : 0))
            {
                return true;
            }
            if (hasReplyData)
                _send(reply.second.ptr.get(), reply.second.size, 0);
        }

        if (payload)
            zmq_msg_close(&msg);
        return true;
    }

private:
    bool _send(const void* data, const size_t size, const int flags)
    {
        zmq_msg_t msg;
        zmq_msg_init_size(&msg, size);
        ::memcpy(zmq_msg_data(&msg), data, size);
        int ret = zmq_msg_send(&msg, socket.get(), flags);
        zmq_msg_close(&msg);

        if (ret != -1)
            return true;

        ZEROEQWARN << "Cannot send reply: " << zmq_strerror(zmq_errno())
                   << std::endl;
        return false;
    }

    /** @return true if more data available */
    bool _recv(void* data, const size_t size)
    {
        // std::cout << uri.getPort() << ": " << size << std::endl;
        zmq_msg_t msg;
        zmq_msg_init(&msg);
        zmq_msg_recv(&msg, socket.get(), 0);
        if (zmq_msg_size(&msg) != size)
            ZEROEQWARN << "Request size mismatch, expected " << size << " got "
                       << zmq_msg_size(&msg) << std::endl;
        else
            memcpy(data, zmq_msg_data(&msg), size);
        const bool more = zmq_msg_more(&msg);
        zmq_msg_close(&msg);
        return more;
    }

    std::unordered_map<uint128_t, HandleFunc> _handlers;
};

Server::Server()
    : Receiver()
    , _impl(new Impl({}, DEFAULT_SESSION))
{
}

Server::Server(const std::string& session)
    : Receiver()
    , _impl(new Impl({}, session))
{
}

Server::Server(const URI& uri)
    : Receiver()
    , _impl(new Impl(uri, DEFAULT_SESSION))
{
}

Server::Server(const URI& uri, const std::string& session)
    : Receiver()
    , _impl(new Impl(uri, session))
{
}

Server::Server(Receiver& shared)
    : Receiver(shared)
    , _impl(new Impl({}, DEFAULT_SESSION))
{
}

Server::Server(const std::string& session, Receiver& shared)
    : Receiver(shared)
    , _impl(new Impl({}, session))
{
}

Server::Server(const URI& uri, Receiver& shared)
    : Receiver(shared)
    , _impl(new Impl(uri, DEFAULT_SESSION))
{
}

Server::Server(const URI& uri, const std::string& session, Receiver& shared)
    : Receiver(shared)
    , _impl(new Impl(uri, session))
{
}

Server::~Server()
{
}

Server::Server(Server&& from)
    : Receiver(from)
    , _impl(std::move(from._impl))
{
}

Server& Server::operator=(Server&& from)
{
    if (this == &from)
        return *this;

    Receiver::operator=(std::move(from));
    _impl = std::move(from._impl);
    return *this;
}

const std::string& Server::getSession() const
{
    return _impl->getSession();
}

void Server::addSockets(std::vector<detail::Socket>& entries)
{
    _impl->addSockets(entries);
}

bool Server::process(detail::Socket& socket)
{
    return _impl->process(socket);
}

void Server::addConnection(const std::string&)
{
    ZEROEQTHROW(std::runtime_error("Server cannot add connections"));
}

const URI& Server::getURI() const
{
    return _impl->uri;
}

bool Server::handle(const uint128_t& request, const HandleFunc& func)
{
    return _impl->handle(request, func);
}

zmq::SocketPtr Server::getSocket()
{
    return _impl->socket;
}
}
