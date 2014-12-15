
/* Copyright (c) 2014, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#ifndef ZEQ_BROKER_H
#define ZEQ_BROKER_H

#include <zeq/receiver.h> // base class

namespace zeq
{
namespace connection
{
namespace detail { class Broker; }

/** Brokers subscription requests for a zeq::Subscriber. */
class Broker : public Receiver
{
public:
    /**
     * Create a new subscription broker.
     *
     * The given subscriber has to have at least the same lifetime as this
     * broker. The subscriber and receiver are automatically shared.
     *
     * For simplicity, only a single Subscriber is handled by a Broker. The
     * implementation should be extended if multiple subscribers shall be
     * handled.
     *
     * @param address the zmq reply socket address to be used.
     * @param subscriber the Subscriber to manage.
     * @throw std::runtime_error when zmq setup failed.
     */
    ZEQ_API Broker( const std::string& address, Subscriber& subscriber );

    /** Destroy this broker. */
    ZEQ_API ~Broker();

private:
    detail::Broker* const _impl;

    // Receiver API
    void addSockets( std::vector< zeq::detail::Socket >& entries ) final;
    void process( zeq::detail::Socket& socket ) final;
};

}
}
#endif
