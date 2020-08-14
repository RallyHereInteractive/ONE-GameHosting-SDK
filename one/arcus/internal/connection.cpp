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

// Stream buffers sizes used to pump pending data from/to the connection's
// socket.
constexpr size_t stream_send_buffer_size = 1024 * 128;
constexpr size_t stream_receive_buffer_size = 1024 * 128;

Connection::Connection(Socket &socket, size_t max_messages_in, size_t max_messages_out)
    : _status(Status::handshake_not_started)
    , _socket(socket)
    , _send_stream(stream_send_buffer_size)
    , _receive_stream(stream_receive_buffer_size) {
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
    bool is_ready = false;
    auto err = _socket.ready_for_send(0.f, is_ready);
    if (is_error(err)) return err;
    if (!is_ready) return ONE_ERROR_NONE;

    if (_status != Status::ready) return process_handshake();

    return process_messages();
}

void Connection::reset() {
    _status = Status::handshake_not_started;
}

Error Connection::ensure_nothing_received() {
    char byte;
    size_t received;
    auto err = _socket.receive(&byte, 1, received);
    if (is_error(err)) {
        return err;
    }
    if (received > 0) {
        return ONE_ERROR_CONNECTION_RECEIVE_BEFORE_SEND;
    }
    return ONE_ERROR_NONE;
}

Error Connection::try_send_hello() {
    auto &stream = _send_stream;

    // Buffer outgoing message if not yet done so. Nearly all the time the
    // send will succeed since it is tiny and partial sends
    // are rare edge cases in general.
    if (stream.size() == 0) {
        stream.put(&codec::valid_hello(), codec::hello_size());
    }

    // Get remaining buffer.
    const auto size = stream.size();
    assert(size > 0);
    void *data = nullptr;
    stream.peek(size, &data);
    assert(data != nullptr);

    // Send as much as possible.
    size_t sent;
    auto err = _socket.send(data, size, sent);
    if (is_error(err)) {  // Error.
        return ONE_ERROR_CONNECTION_HELLO_SEND_FAILED;
    }
    if (sent == 0) return ONE_ERROR_TRY_AGAIN;

    // Remove from send stream, check if finished.
    stream.trim(sent);
    if (stream.size() > 0) return ONE_ERROR_TRY_AGAIN;

    return ONE_ERROR_NONE;
}

Error Connection::try_receive_hello() {
    // Read a hello packet from socket.
    codec::Hello hello = {0};
    size_t received;
    auto err = _socket.receive(&hello, codec::hello_size(), received);
    if (is_error(err)) {
        return ONE_ERROR_CONNECTION_HELLO_RECEIVE_FAILED;
    }
    if (received == 0) {             // No error but nothing received.
        return ONE_ERROR_TRY_AGAIN;  // Retry next attempt.
    }
    if (received > codec::hello_size()) {
        return ONE_ERROR_CONNECTION_HELLO_TOO_BIG;
    }

    // Buffer bytes read. This will normally be the entire hello, but
    // there are edge cases that might result in partial reads. Return
    // if the full hello has not been read yet.
    _receive_stream.put(&hello, received);
    if (_receive_stream.size() < codec::hello_size()) {
        return ONE_ERROR_TRY_AGAIN;
    }

    // Read and validate the full hello from the receive buffer.
    codec::Hello *data;
    _receive_stream.get(codec::hello_size(), reinterpret_cast<void **>(&data));
    assert(data != nullptr);

    if (!codec::validate_hello(*data)) {
        return ONE_ERROR_CONNECTION_HELLO_INVALID;
    }
    return ONE_ERROR_NONE;
}

// There are two hello packets. The initial codec::hello sent from the
// handshake initiater, and the response codec::Header message with a
// hello opcode sent in response. This is the response header.
const codec::Header &hello_message() {
    static codec::Header message = {0};
    message.opcode = static_cast<char>(Opcodes::hello);
    return message;
}

Error Connection::try_send_hello_message() {
    auto &stream = _send_stream;

    // Buffer outgoing message if not yet done so. Nearly all the time the send
    // will succeed since it is tiny and partial sends are rare edge cases in
    // general.
    if (stream.size() == 0) {
        stream.put(&hello_message(), codec::header_size());
    }

    // Get remaining buffer.
    const auto size = stream.size();
    assert(size > 0);
    void *data = nullptr;
    stream.peek(size, &data);
    assert(data != nullptr);

    // Send.
    size_t sent;
    auto err = _socket.send(data, size, sent);
    if (is_error(err)) {
        return ONE_ERROR_CONNECTION_HELLO_MESSAGE_SEND_FAILED;
    }
    if (sent == 0) return ONE_ERROR_TRY_AGAIN;

    // Remove from send stream, check if finished.
    stream.trim(sent);
    if (stream.size() > 0) return ONE_ERROR_TRY_AGAIN;

    return ONE_ERROR_NONE;
}

Error Connection::try_receive_message_header(codec::Header &header) {
    size_t received;
    auto err = _socket.receive(&header, codec::header_size(), received);
    if (is_error(err)) {
        _status = Status::error;
        return ONE_ERROR_CONNECTION_HELLO_MESSAGE_RECEIVE_FAILED;
    }
    if (received > codec::header_size()) {
        _status = Status::error;
        return ONE_ERROR_CONNECTION_HELLO_MESSAGE_HEADER_TOO_BIG;
    }

    // Buffer bytes read.
    _receive_stream.put(&header, received);
    if (_receive_stream.size() < codec::header_size()) {
        return ONE_ERROR_TRY_AGAIN;
    }

    // Read the full hello from the receive buffer.
    codec::Header *data = nullptr;
    _receive_stream.get(codec::header_size(), reinterpret_cast<void **>(&data));
    assert(data != nullptr);
    memcpy(&header, data, codec::header_size());

    if (!codec::validate_header(*data)) return ONE_ERROR_CONNECTION_INVALID_MESSAGE_HEADER;

    return ONE_ERROR_NONE;
}

Error Connection::try_receive_hello_message() {
    codec::Header header = {0};
    auto err = try_receive_message_header(header);
    if (is_error(err)) {
        return err;
    }

    if (std::memcmp(&header, &hello_message(), codec::header_size()) != 0) {
        _status = Status::error;
        return ONE_ERROR_CONNECTION_HELLO_MESSAGE_REPLY_INVALID;
    }
    return ONE_ERROR_NONE;
}

Error Connection::process_handshake() {
    // Check if handshake timed out.
    // return ONE_ERROR_NONE;

    Error err;
    auto fail = [this](Error err) {
        _status = Status::error;
        return err;
    };

    switch (_status) {
        case Status::handshake_not_started:
            // Check if hello received.
            err = try_receive_hello();
            if (err == ONE_ERROR_TRY_AGAIN) break;
            if (is_error(err)) return fail(err);

            {
                bool is_ready = false;
                err = _socket.ready_for_send(0.f, is_ready);
                if (is_error(err)) return err;
                if (!is_ready) break;
            }
            // Fallthrough.
        case Status::handshake_hello_received:
            err = try_send_hello_message();
            if (err == ONE_ERROR_TRY_AGAIN) break;
            if (is_error(err)) return fail(err);
            // Assume handshaking is complete now. This side is free to send other
            // Messages now. If handshaking fails on the server, then the connection
            // will be closed and the Messages will be ignored.
            _status = Status::ready;
            break;
        case Status::handshake_hello_scheduled:
            // Ensure nothing is received. Arcus client should not send
            // until it receives a Hello.
            err = ensure_nothing_received();
            if (is_error(err)) return fail(err);

            // Send the hello.
            err = try_send_hello();
            if (err == ONE_ERROR_TRY_AGAIN) break;
            if (is_error(err)) return fail(err);

            _status = Status::handshake_hello_sent;
            break;
        case Status::handshake_hello_sent:
            err = try_receive_hello_message();
            if (err == ONE_ERROR_TRY_AGAIN) break;
            if (is_error(err)) return fail(err);
            _status = Status::ready;
            break;
        default:
            _status = Status::error;
            return fail(ONE_ERROR_CONNECTION_UNKNOWN_STATUS);
    }

    return ONE_ERROR_NONE;
}

Error Connection::process_messages() {
    codec::Header header = {0};
    auto err = ONE_ERROR_NONE;

    auto read_header_and_continue = [&]() -> bool {
        err = try_receive_message_header(header);
        if (err == ONE_ERROR_TRY_AGAIN) {
            err = ONE_ERROR_NONE;
            return false;
        }
        if (is_error(err)) {
            return false;
        }
        return true;
    };

    while (read_header_and_continue()) {
        // Convert to Message.

        // Store in incoming queue for consumption.

        // Check if socket is ready to check another.
        // Return if socket has no activity or has an error.
        bool is_ready = false;
        auto err = _socket.ready_for_send(0.f, is_ready);
        if (is_error(err)) return err;
        if (!is_ready) return ONE_ERROR_NONE;
    }

    if (is_error(err)) _status = Status::error;

    return ONE_ERROR_NONE;
}

}  // namespace one
