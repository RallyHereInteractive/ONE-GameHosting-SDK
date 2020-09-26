#include <one/arcus/client.h>

#include <one/arcus/internal/connection.h>
#include <one/arcus/internal/message.h>
#include <one/arcus/opcode.h>
#include <one/arcus/internal/socket.h>
#include <one/arcus/message.h>

namespace i3d {
namespace one {

constexpr size_t connection_retry_delay_seconds = 5;

Client::Client()
    : _socket(nullptr)
    , _connection(nullptr)
    , _is_connected(false)
    , _last_connection_attempt_time(steady_clock::duration::zero())
    , _callbacks({0}) {}

Client::~Client() {
    shutdown();
}

Error Client::init(const char *address, unsigned int port) {
    const std::lock_guard<std::mutex> lock(_client);

    _server_address = address;
    _server_port = port;

    if (_socket != nullptr) {
        return ONE_ERROR_VALIDATION_SOCKET_IS_NULLPTR;
    }

    if (_connection != nullptr) {
        return ONE_ERROR_VALIDATION_CONNECTION_IS_NULLPTR;
    }

    auto err = init_socket_system();
    if (is_error(err)) {
        return err;
    }

    _socket = new Socket();
    if (_socket == nullptr) {
        shutdown();
        return ONE_ERROR_VALIDATION_SOCKET_IS_NULLPTR;
    }

    err = _socket->init();
    if (is_error(err)) {
        shutdown();
        return err;
    }

    _connection =
        new Connection(Connection::max_message_default, Connection::max_message_default);
    _connection->init(*_socket);
    if (_connection == nullptr) {
        shutdown();
        return ONE_ERROR_VALIDATION_CONNECTION_IS_NULLPTR;
    }

    return ONE_ERROR_NONE;
}

void Client::shutdown() {
    const std::lock_guard<std::mutex> lock(_client);

    _is_connected = false;

    if (_socket != nullptr) {
        delete _socket;
        _socket = nullptr;
    }

    if (_connection != nullptr) {
        delete _connection;
        _connection = nullptr;
    }

    shutdown_socket_system();
    _callbacks = {0};
}

Error Client::update() {
    const std::lock_guard<std::mutex> lock(_client);

    if (!is_initialized()) {
        return ONE_ERROR_CLIENT_NOT_INITIALIZED;
    }
    assert(_connection != nullptr);

    // If not connected, attempt to connect at an interval.
    if (!_is_connected) {
        const auto now = steady_clock::now();
        const size_t delta =
            duration_cast<seconds>(now - _last_connection_attempt_time).count();
        if (delta > connection_retry_delay_seconds) {
            _last_connection_attempt_time = now;
            auto error = connect();
            // If connection fails, then nothing else to update. Return the error.
            if (is_error(error)) {
                return error;
            }
        }
    }

    auto close_client = [this](const Error passthrough_err) -> Error {
        _connection->shutdown();
        _socket->close();
        _is_connected = false;
        _socket->init();
        _connection->init(*_socket);
        return passthrough_err;
    };

    auto err = _connection->update();
    // In the case of any error, reset the socket for reconnection attempt.
    if (is_error(err)) {
        return close_client(err);
    }

    // Read pending incoming messages.
    while (true) {
        unsigned int count = 0;
        err = _connection->incoming_count(count);
        if (is_error(err)) return close_client(err);
        if (count == 0) break;

        err = _connection->remove_incoming([this](const Message &message) {
            auto err = process_incoming_message(message);
            if (is_error(err)) return err;

            return ONE_ERROR_NONE;
        });
        if (is_error(err)) {
            return close_client(err);
        }
    }

    return ONE_ERROR_NONE;
}

std::string Client::status_to_string(Status status) {
    switch (status) {
        case Status::uninitialized:
            return "uninitialized";
        case Status::connecting:
            return "connecting";
        case Status::handshake:
            return "handshake";
        case Status::ready:
            return "ready";
        case Status::error:
            return "error";
    }
    return "unknown";
}

Client::Status Client::status() const {
    const std::lock_guard<std::mutex> lock(_client);

    if (!is_initialized()) {
        return Status::uninitialized;
    }
    if (!_is_connected) {
        return Status::connecting;
    }

    const auto status = _connection->status();
    switch (status) {
        case Connection::Status::handshake_not_started:
        case Connection::Status::handshake_hello_scheduled:
        case Connection::Status::handshake_hello_sent:
            return Status::handshake;
        case Connection::Status::ready:
            return Status::ready;
        default:
            return Status::error;
    }
}

Error Client::send_soft_stop_request(int timeout) {
    const std::lock_guard<std::mutex> lock(_client);

    Message message;
    messages::prepare_soft_stop_request(timeout, message);
    auto err = process_outgoing_message(message);
    if (is_error(err)) {
        return err;
    }

    return ONE_ERROR_NONE;
}

Error Client::send_allocated_request(Array *data) {
    const std::lock_guard<std::mutex> lock(_client);

    if (data == nullptr) {
        return ONE_ERROR_VALIDATION_DATA_IS_NULLPTR;
    }

    Message message;
    messages::prepare_allocated_request(*data, message);
    auto err = process_outgoing_message(message);
    if (is_error(err)) {
        return err;
    }

    return ONE_ERROR_NONE;
}

Error Client::send_meta_data_request(Array *data) {
    const std::lock_guard<std::mutex> lock(_client);

    if (data == nullptr) {
        return ONE_ERROR_VALIDATION_DATA_IS_NULLPTR;
    }

    Message message;
    messages::prepare_meta_data_request(*data, message);
    auto err = process_outgoing_message(message);
    if (is_error(err)) {
        return err;
    }

    return ONE_ERROR_NONE;
}

Error Client::send_live_state_request() {
    const std::lock_guard<std::mutex> lock(_client);

    Message message;
    messages::prepare_live_state_request(message);
    auto err = process_outgoing_message(message);
    if (is_error(err)) {
        return err;
    }

    return ONE_ERROR_NONE;
}

Error Client::set_live_state_response_callback(
    std::function<void(void *, int, int, const std::string &, const std::string &,
                       const std::string &, const std::string &)>
        callback,
    void *data) {
    const std::lock_guard<std::mutex> lock(_client);

    if (callback == nullptr) {
        return ONE_ERROR_VALIDATION_CALLBACK_IS_NULLPTR;
    }

    _callbacks._live_state_response = callback;
    _callbacks._live_state_response_data = data;
    return ONE_ERROR_NONE;
}

Error Client::set_player_joined_event_response_callback(
    std::function<void(void *, int)> callback, void *data) {
    const std::lock_guard<std::mutex> lock(_client);

    if (callback == nullptr) {
        return ONE_ERROR_VALIDATION_CALLBACK_IS_NULLPTR;
    }

    _callbacks._player_joined_event_response = callback;
    _callbacks._player_joined_event_response_data = data;
    return ONE_ERROR_NONE;
}

Error Client::set_player_left_response_callback(std::function<void(void *, int)> callback,
                                                void *data) {
    const std::lock_guard<std::mutex> lock(_client);

    if (callback == nullptr) {
        return ONE_ERROR_VALIDATION_CALLBACK_IS_NULLPTR;
    }

    _callbacks._player_left_response = callback;
    _callbacks._player_left_response_data = data;
    return ONE_ERROR_NONE;
}

Error Client::set_host_information_request_callback(std::function<void(void *)> callback,
                                                    void *data) {
    const std::lock_guard<std::mutex> lock(_client);

    if (callback == nullptr) {
        return ONE_ERROR_VALIDATION_CALLBACK_IS_NULLPTR;
    }

    _callbacks._host_information_request = callback;
    _callbacks._host_information_request_data = data;
    return ONE_ERROR_NONE;
}

Error Client::set_application_instance_information_request_callback(
    std::function<void(void *)> callback, void *data) {
    const std::lock_guard<std::mutex> lock(_client);

    if (callback == nullptr) {
        return ONE_ERROR_VALIDATION_CALLBACK_IS_NULLPTR;
    }

    _callbacks._application_instance_information_request = callback;
    _callbacks._application_instance_information_request_data = data;
    return ONE_ERROR_NONE;
}

Error Client::set_application_instance_get_status_request_callback(
    std::function<void(void *)> callback, void *data) {
    const std::lock_guard<std::mutex> lock(_client);

    if (callback == nullptr) {
        return ONE_ERROR_VALIDATION_CALLBACK_IS_NULLPTR;
    }

    _callbacks._application_instance_get_status_request = callback;
    _callbacks._application_instance_get_status_request_data = data;
    return ONE_ERROR_NONE;
}

Error Client::set_application_instance_set_status_request_callback(
    std::function<void(void *, int)> callback, void *data) {
    const std::lock_guard<std::mutex> lock(_client);

    if (callback == nullptr) {
        return ONE_ERROR_VALIDATION_CALLBACK_IS_NULLPTR;
    }

    _callbacks._application_instance_set_status_request = callback;
    _callbacks._application_instance_set_status_request_data = data;
    return ONE_ERROR_NONE;
}

Error Client::process_incoming_message(const Message &message) {
    switch (message.code()) {
        case Opcode::live_state_response:
            if (_callbacks._live_state_response == nullptr) {
                return ONE_ERROR_NONE;
            }

            return invocation::live_state_response(message,
                                                   _callbacks._live_state_response,
                                                   _callbacks._live_state_response_data);
        case Opcode::player_joined_event_response:
            if (_callbacks._player_joined_event_response == nullptr) {
                return ONE_ERROR_NONE;
            }

            return invocation::player_joined_event_response(
                message, _callbacks._player_joined_event_response,
                _callbacks._player_joined_event_response_data);
        case Opcode::player_left_response:
            if (_callbacks._player_left_response == nullptr) {
                return ONE_ERROR_NONE;
            }

            return invocation::player_left_response(
                message, _callbacks._player_left_response,
                _callbacks._player_left_response_data);
        case Opcode::host_information_request:
            if (_callbacks._host_information_request == nullptr) {
                return ONE_ERROR_NONE;
            }

            return invocation::host_information_request(
                message, _callbacks._host_information_request,
                _callbacks._host_information_request_data);
        case Opcode::application_instance_information_request:
            if (_callbacks._application_instance_information_request == nullptr) {
                return ONE_ERROR_NONE;
            }

            return invocation::application_instance_information_request(
                message, _callbacks._application_instance_information_request,
                _callbacks._application_instance_information_request_data);
        case Opcode::application_instance_get_status_request:
            if (_callbacks._application_instance_get_status_request == nullptr) {
                return ONE_ERROR_NONE;
            }

            return invocation::application_instance_get_status_request(
                message, _callbacks._application_instance_get_status_request,
                _callbacks._application_instance_get_status_request_data);
        case Opcode::application_instance_set_status_request:
            if (_callbacks._application_instance_set_status_request == nullptr) {
                return ONE_ERROR_NONE;
            }

            return invocation::application_instance_set_status_request(
                message, _callbacks._application_instance_set_status_request,
                _callbacks._application_instance_set_status_request_data);
        default:
            return ONE_ERROR_NONE;
    }
}

Error Client::process_outgoing_message(const Message &message) {
    Error err = ONE_ERROR_NONE;
    switch (message.code()) {
        case Opcode::soft_stop_request: {
            params::SoftStopRequest params;
            err = validation::soft_stop_request(message, params);
            if (is_error(err)) {
                return err;
            }

            break;
        }
        case Opcode::allocated_request: {
            params::AllocatedRequest params;
            err = validation::allocated_request(message, params);
            if (is_error(err)) {
                return err;
            }

            break;
        }
        case Opcode::meta_data_request: {
            params::MetaDataRequest params;
            err = validation::meta_data_request(message, params);
            if (is_error(err)) {
                return err;
            }

            break;
        }
        case Opcode::live_state_request: {
            params::LiveStateRequest params;
            err = validation::live_state_request(message, params);
            if (is_error(err)) {
                return err;
            }

            break;
        }
        default:
            return ONE_ERROR_NONE;
    }

    if (_connection == nullptr) {
        return ONE_ERROR_VALIDATION_CONNECTION_IS_NULLPTR;
    }
    // Do not accumulate messages if the connection is not active and past
    // handshaking.
    if (_connection->status() != Connection::Status::ready) {
        return ONE_ERROR_SERVER_CONNECTION_NOT_READY;
    }

    err = _connection->add_outgoing([&](Message &m) {
        m = message;
        return ONE_ERROR_NONE;
    });
    if (is_error(err)) {
        return err;
    }

    return ONE_ERROR_NONE;
}

Error Client::connect() {
    if (!is_initialized()) {
        return ONE_ERROR_CLIENT_NOT_INITIALIZED;
    }
    assert(_socket != nullptr);
    assert(_connection != nullptr);

    if (!_socket->is_initialized()) {
        return ONE_ERROR_CLIENT_NOT_INITIALIZED;
    }

    auto err = _socket->connect(_server_address.c_str(), _server_port);
    if (is_error(err)) {
        return err;
    }

    _is_connected = true;
    return ONE_ERROR_NONE;
}

}  // namespace one
}  // namespace i3d
