#include <one/arcus/internal/connection.h>

#include <assert.h>
#include <cstring>

#include <one/arcus/internal/codec.h>
#include <one/arcus/internal/opcodes.h>
#include <one/arcus/internal/socket.h>

#ifdef WINDOWS
#else
    #include <errno.h>
#endif

namespace one {

Connection::Connection(Socket &socket, size_t max_messages_in, size_t max_messages_out)
    : _status(Status::handshake_not_started), _socket(socket) {
    assert(socket.is_initialized());
}

Connection::Status Connection::status() const {
    return _status;
}

Error Connection::push_outgoing(const Message &message) {
    return ONE_ERROR_NONE;
}

Error Connection::incoming_count() const {
    return ONE_ERROR_NONE;
}

Error Connection::pop_incoming(Message **message) {
    return ONE_ERROR_NONE;
}

void Connection::initiate_handshake() {
    assert(_status == Status::handshake_not_started);
    _status = Status::handshake_hello_scheduled;
}

Error Connection::update() {
    if (_status == Status::error) return ONE_ERROR_CONNECTION_UPDATE_AFTER_ERROR;

    // Return if socket has no activity or has an error.
    const auto select = _socket.select(0.f);
    if (select < 0) return ONE_ERROR_CONNECTION_UPDATE_READY_FAIL;

    if (_status != Status::ready) return process_handshake();

    return process_messages();
}

void Connection::reset() {
    _status = Status::handshake_not_started;
}

Error Connection::process_handshake() {
    // Check if handshake timed out.
    // return ONE_ERROR_NONE;

    // There are two hello packets. The initial codec::hello sent from the
    // handshake initiater, and the response codec::Header message with a
    // hello opcode sent in response. This is the response header.
    static codec::Header hello_header = {0};
    hello_header.opcode = static_cast<char>(Opcodes::hello);

    if (_status == Status::handshake_not_started) {
        // Check if hello received.
        codec::Hello hello = {0};
        size_t received;
        auto err = _socket.receive(&hello, codec::hello_size(), received);
        if (is_error(err)) {
            _status = Status::error;
            return ONE_ERROR_CONNECTION_HELLO_RECEIVE_FAILED;
        }

        if (received == 0) {        // No error but nothing received.
            return ONE_ERROR_NONE;  // Retry next attempt.
        }

        // todo - hello buffering
        if (received != codec::hello_size()) {
            _status = Status::error;
            return ONE_ERROR_CONNECTION_HELLO_TOO_BIG;
        }
        if (!codec::validate_hello(hello)) {
            _status = Status::error;
            return ONE_ERROR_CONNECTION_HELLO_INVALID;
        }

        // Send back a hello Message.
        size_t sent;
        err = _socket.send(&hello_header, codec::header_size(), sent);
        if (is_error(err)) {
            _status = Status::error;
            return ONE_ERROR_CONNECTION_HELLO_MESSAGE_SEND_FAILED;
        }
        // Todo - buffer outgoing send, send remaining if sent < codec::header_size().

        // Assume handshaking is complete now. This side is free to send other
        // Messages now. If handshaking fails on the server, then the connection
        // will be closed and the Messages will be ignored.
        _status = Status::ready;
    } else if (_status == Status::handshake_hello_scheduled) {
        // Ensure nothing is received. Arcus client should not send
        // until it receives a Hello.
        char byte;
        size_t received;
        auto err = _socket.receive(&byte, 1, received);
        if (is_error(err)) {
            _status = Status::error;
            return err;
        }
        if (received > 0) {
            _status = Status::error;
            return ONE_ERROR_CONNECTION_RECEIVE_BEFORE_SEND;
        }
        // Send the hello.
        size_t sent;
        err = _socket.send(&codec::valid_hello(), codec::hello_size(), sent);
        if (is_error(err)) {  // Error.
            _status = Status::error;
            return ONE_ERROR_CONNECTION_HELLO_SEND_FAILED;
        } else if (sent == 0) {     // No error but nothing sent.
            return ONE_ERROR_NONE;  // Retry next attempt.
        }
        // Todo - above send buffering, ensure all is sent if partial send.

        _status = Status::handshake_hello_sent;
    } else if (_status == Status::handshake_hello_sent) {
        // Check if hello message Header received back.
        codec::Header header = {0};
        size_t received;
        auto err = _socket.receive(&header, codec::header_size(), received);
        // Todo - buffer.
        if (is_error(err)) {
            _status = Status::error;
            return ONE_ERROR_CONNECTION_HELLO_MESSAGE_RECEIVE_FAILED;
        }
        if (received > codec::header_size()) {
            _status = Status::error;
            return ONE_ERROR_CONNECTION_HELLO_MESSAGE_HEADER_TOO_BIG;
        }
        if (received < codec::header_size()) {
            _status = Status::error;
            return ONE_ERROR_CONNECTION_HELLO_MESSAGE_HEADER_TOO_SMALL;
        }
        if (std::memcmp(&header, &hello_header, codec::header_size()) != 0) {
            _status = Status::error;
            return ONE_ERROR_CONNECTION_HELLO_MESSAGE_REPLY_INVALID;
        }
        _status = Status::ready;
    }

    return ONE_ERROR_NONE;
}

Error Connection::process_messages() {
    return ONE_ERROR_NONE;
}

}  // namespace one
