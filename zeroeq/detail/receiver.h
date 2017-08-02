/* Copyright (c) 2017, Human Brain Project
 *                          Stefan.Eilemann@epfl.ch
 */

#pragma once

#include "common.h"
#include "constants.h"
#include "context.h"
#include "socket.h"

#include "../log.h"

#include <servus/servus.h>
#include <zmq.h>

namespace zeroeq
{
namespace detail
{
/** Manages and updates a set of connections with a zeroconf browser. */
class Receiver
{
public:
    Receiver(const std::string& service, const std::string session)
        : _servus(service)
        , _session(session)
        , _context(detail::getContext())
    {
        if (session == zeroeq::NULL_SESSION || session.empty())
            ZEROEQTHROW(std::runtime_error(
                std::string("Invalid session name for browsing")));

        if (!servus::Servus::isAvailable())
            ZEROEQTHROW(
                std::runtime_error(std::string("Empty servus implementation")));

        _servus.beginBrowsing(servus::Servus::IF_ALL);
    }

    Receiver(const std::string& service)
        : _servus(service)
        , _session(zeroeq::NULL_SESSION)
        , _context(detail::getContext())
    {
    }

    virtual ~Receiver()
    {
        if (_servus.isBrowsing())
            _servus.endBrowsing();
    }

    const std::string& getSession() const { return _session; }

    bool update() //!< @return true if new connection made
    {
        if (!_servus.isBrowsing())
            return false;

        bool updated = false;
        _servus.browse(0);
        const servus::Strings& instances = _servus.getInstances();
        for (const std::string& instance : instances)
        {
            const std::string& zmqURI = _getZmqURI(instance);

            if (_sockets.count(zmqURI) == 0) // New provider
            {
                const std::string& session = _servus.get(instance, KEY_SESSION);
                if (_servus.containsKey(instance, KEY_SESSION) &&
                    !_session.empty() && session != _session)
                {
                    continue;
                }

                const uint128_t identifier(_servus.get(instance, KEY_INSTANCE));
                zmq::SocketPtr socket = createSocket(identifier);
                if (socket && _connect(zmqURI, socket))
                    updated = true;
            }
        }
        return updated;
    }

    bool addConnection(const std::string& zmqURI)
    {
        zmq::SocketPtr socket = createSocket(uint128_t());
        if (socket)
            return _connect(zmqURI, socket);
        return true;
    }

    void addSockets(std::vector<detail::Socket>& entries)
    {
        entries.insert(entries.end(), _entries.begin(), _entries.end());
    }

protected:
    using SocketMap = std::map<std::string, zmq::SocketPtr>;

    void* getContext() { return _context.get(); }

    /**
     * Create the socket for the given instance, return nullptr if connection is
     * to be ignored.
     */
    virtual zmq::SocketPtr createSocket(const uint128_t& instance) = 0;

    const SocketMap& getSockets() { return _sockets; }

    bool _connect(const std::string& zmqURI, zmq::SocketPtr socket)
    {
        if (zmq_connect(socket.get(), zmqURI.c_str()) == -1)
        {
            ZEROEQINFO << "Cannot connect to " << zmqURI << ": "
                       << zmq_strerror(zmq_errno()) << std::endl;
            return false;
        }

        _sockets[zmqURI] = socket; // ref socket since zmq struct is void*

        detail::Socket entry;
        entry.socket = socket.get();
        entry.events = ZMQ_POLLIN;
        _entries.push_back(entry);
        return true;
    }

private:
    servus::Servus _servus;
    const std::string _session;

    zmq::ContextPtr _context;
    SocketMap _sockets;
    std::vector<detail::Socket> _entries;

    std::string _getZmqURI(const std::string& instance)
    {
        const size_t pos = instance.find(":");
        const std::string& host = instance.substr(0, pos);
        const std::string& port = instance.substr(pos + 1);

        return buildZmqURI(DEFAULT_SCHEMA, host, std::stoi(port));
    }
};
}
}
