/* Copyright (c) 2017, Human Brain Project
 *                          Stefan.Eilemann@epfl.ch
 */

#pragma once

#include "common.h"
#include "constants.h"
#include "socket.h"

#include "../log.h"

#include <servus/servus.h>
#include <zmq.h>

namespace zeroeq
{
namespace detail
{
/** Manages and updates a set of connections with a zeroconf browser. */
class Browser
{
public:
    Browser(const std::string& service, const std::string session)
        : _servus(service)
        , _session(session)
    {
        if (_session == zeroeq::NULL_SESSION || session.empty())
            return;

        if (!servus::Servus::isAvailable())
            ZEROEQTHROW(
                std::runtime_error(std::string("Empty servus implementation")));

        _servus.beginBrowsing(servus::Servus::IF_ALL);
        update();
    }

    virtual ~Browser()
    {
        if (_servus.isBrowsing())
            _servus.endBrowsing();
    }

    const std::string& getSession() const { return _session; }

    void update()
    {
        if (!_servus.isBrowsing())
            return;

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
                if (!addConnection(zmqURI, identifier))
                {
                    ZEROEQINFO << "Cannot connect to " << zmqURI << ": "
                               << zmq_strerror(zmq_errno()) << std::endl;
                }
            }
        }
    }

    virtual bool addConnection(const std::string& zmqURI,
                               const uint128_t& instance) = 0;

    void addSockets(std::vector<detail::Socket>& entries)
    {
        entries.insert(entries.end(), _entries.begin(), _entries.end());
    }

protected:
    using SocketMap = std::map<std::string, zmq::SocketPtr>;

    const SocketMap& getSockets() { return _sockets; }
    bool addConnection(const std::string& zmqURI, zmq::SocketPtr socket)
    {
        if (zmq_connect(socket.get(), zmqURI.c_str()) == -1)
            return false;

        _sockets[zmqURI] = socket;

        detail::Socket entry;
        entry.socket = socket.get();
        entry.events = ZMQ_POLLIN;
        _entries.push_back(entry);
        return true;
    }

private:
    servus::Servus _servus;
    const std::string _session;

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
