
/* Copyright (c) 2014-2015, Human Brain Project
 *                          Daniel Nachbaur <daniel.nachbaur@epfl.ch>
 *                          Stefan.Eilemann@epfl.ch
 */

#ifndef ZEQ_PUBLISHER_H
#define ZEQ_PUBLISHER_H

#include <zeq/api.h>
#include <zeq/types.h>
#include <zerobuf/types.h>

namespace zeq
{
namespace detail { class Publisher; }

/**
 * Serves and publishes events, consumed by Subscriber.
 *
 * Example: @include tests/publisher.cpp
 */
class Publisher
{
public:
    /**
     * Create a publisher on the given URI.
     *
     * @param uri publishing URI in the format scheme://[*|host|IP|IF][:port]
     * @param announceMode bitwise combination of AnnounceMode network protocols
     */
    ZEQ_API Publisher( const servus::URI& uri,
                       uint32_t announceMode = ANNOUNCE_ALL );

    ZEQ_API ~Publisher();

    /**
     * Publish the given event to any subscriber.
     *
     * If there is no subscriber for that event, no event will be send.
     *
     * @param event the serialized event to publish
     * @return true if publish was successful
     */
    ZEQ_API bool publish( const Event& event );

    /**
     * Publish the given event to any subscriber.
     *
     * If there is no subscriber for that event, no event will be send.
     *
     * @param event the serialized event to publish
     * @return true if publish was successful
     */
    ZEQ_API bool publish( const zerobuf::Zerobuf& zerobuf );

    /**
     * Get the publisher URI.
     *
     * Contains the used hostname and port, if none where given in the
     * constructor uri.
     *
     * @return the publisher URI.
     */
    ZEQ_API const servus::URI& getURI() const;

    std::string getAddress() const; //!< @internal

private:
    detail::Publisher* const _impl;

    Publisher( const Publisher& ) = delete;
    Publisher& operator=( const Publisher& ) = delete;
};

}

#endif
