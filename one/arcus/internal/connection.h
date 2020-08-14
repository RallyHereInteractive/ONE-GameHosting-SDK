#pragma once

#include <vector>

#include <one/arcus/error.h>
#include <one/arcus/internal/accumulator.h>
#include <one/arcus/internal/platform.h>

namespace one {

namespace codec {
class Header;
}
class Socket;
class Message;
template <typename T>
class RingBuffer;

// Connection manages Arcus protocol communication between two TCP sockets.
class Connection final {
public:
    // Connection must be given an active socket. Socket errors encountered
    // during processing will be returned as errors, and it is the caller's
    // responsibilty to either destroy the Connection, or restore the Socket's
    // state for communication.
    Connection(Socket &socket, size_t max_messages_in, size_t max_messages_out);
    ~Connection() = default;

    // Marks this side of the connection as responsible for initiating the
    // handshaking process. Must be called from one side of the connection
    // only. Attempting to send a Message or any other data to other side of
    // the Connection before handshaking is complete results in an error.
    void initiate_handshake();

    // Update process incoming and outgoing messges. It attempts to read
    // all incoming messages that are available. It attempts to send all
    // queued outgoing messages.
    Error update();

    // Reset the status to match construction time and clear any accumulated
    // buffers.
    void reset();

    enum class Status {
        handshake_not_started,
        handshake_hello_received,
        handshake_hello_scheduled,
        handshake_hello_sent,
        ready,
        error
    };
    Status status() const;

    Error push_outgoing(const Message &message);

    Error incoming_count() const;

    Error pop_incoming(Message **message);

private:
    Connection() = delete;

    Error process_handshake();
    Error process_messages();

    Error accumulate_receive(const void *, size_t);
    Error accumulate_send(const void *, size_t);

    // Message helpers.
    Error Connection::try_receive_message_header(codec::Header &header);

    // Handshake helpers.
    Error ensure_nothing_received();
    Error try_send_hello();  // The initial hello type.
    Error try_receive_hello();
    Error try_send_hello_message();  // Hello as a Message with opcode.
    Error try_receive_hello_message();

    Status _status;
    Socket &_socket;

    Accumulator _receive_stream;
    Accumulator _send_stream;

    // RingBuffer<Message*> _incomingMessages;
    // RingBuffer<Message*> _outgoingMessages;
};

}  // namespace one
