#pragma once

#include <one/arcus/error.h>

#include <functional>
#include <mutex>
#include <string>

namespace i3d {
namespace one {

class Array;
class Connection;
class Message;
class Object;
class Socket;

namespace callback {

struct ServerCallbacks {
    std::function<void(void *, int)> _soft_stop;
    void *_soft_stop_userdata;
    std::function<void(void *, Array *)> _allocated;
    void *_allocated_userdata;
    std::function<void(void *, Array *)> _metadata;
    void *_metadata_userdata;
    std::function<void(void *, Object *)> _host_information;
    void *_host_information_data;
    std::function<void(void *, Object *)> _application_instance_information;
    void *_application_instance_information_data;
};

}  // namespace callback

// An Arcus Server is designed for use by a Game. It allows an Arcus One Agent
// to connect and communicate with the game.
class Server final {
public:
    Server();
    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;
    ~Server();

    Error init();

    Error shutdown();

    // Note these MUST be kept in sync with the values in c_api.cpp, or
    // otherwise ensured to translate properly to the c-api values.
    enum class Status {
        uninitialized = 0,
        initialized,
        waiting_for_client,
        handshake,
        ready,
        error
    };
    Status status() const;
    static std::string status_to_string(Status status);

    Error listen(unsigned int port);

    // Process pending received and outgoing messages. Any incoming messages are
    // validated according to the Arcus API version standard, and callbacks, if
    // set, are called. Messages without callbacks set are dropped and ignored.
    Error update();

    //------------------------------------------------------------------------------
    // Property setters.

    Error set_live_state(int players, int max_players, const char *name, const char *map,
                         const char *mode, const char *version, Object *additional_data);

    // Must match api standards.
    enum class ApplicationInstanceStatus { starting = 3, online = 4, allocated = 5 };
    Error set_application_instance_status(ApplicationInstanceStatus status);

    //------------------------------------------------------------------------------
    // Callbacks to be notified of all possible incoming Arcus messages.

    // Todo: update functions to match complete list from One API v2.

    // set the callback for when a soft_stop message in received.
    // The `void *data` is the user provided and will be passed as the first argument
    // of the callback when invoked.
    // The `data` can be nullptr, the callback is responsible to use the data properly.
    Error set_soft_stop_callback(std::function<void(void *, int)> callback, void *data);

    // set the callback for when a allocated message in received.
    // The `void *data` is the user provided and will be passed as the first argument
    // of the callback when invoked.
    // The `data` can be nullptr, the callback is responsible to use the data properly.
    Error set_allocated_callback(std::function<void(void *, Array *)> callback,
                                 void *data);

    // set the callback for when a metadata message in received.
    // The `void *data` is the user provided and will be passed as the first argument
    // of the callback when invoked.
    // The `data` can be nullptr, the callback is responsible to use the data properly.
    Error set_metadata_callback(std::function<void(void *, Array *)> callback,
                                void *data);

    // set the callback for when a host_information message in received.
    // The `void *data` is the user provided and will be passed as the first argument
    // of the callback when invoked.
    // The `data` can be nullptr, the callback is responsible to use the data properly.
    Error set_host_information_callback(std::function<void(void *, Object *)> callback,
                                        void *data);

    // set the callback for when a application_instance_information message in
    // received. The `void *data` is the user provided and will be passed as the first
    // argument of the callback when invoked. The `data` can be nullptr, the callback is
    // responsible to use the data properly.
    Error set_application_instance_information_callback(
        std::function<void(void *, Object *)> callback, void *data);

private:
    struct GameState {
        GameState() : players(0), max_players(0), name(), map(), mode(), version() {}

        int players;          // Game number of players.
        int max_players;      // Game max number of players.
        std::string name;     // Server name.
        std::string map;      // Game map.
        std::string mode;     // Game mode.
        std::string version;  // Game version.

        Object *additional_data;  // Optional extra fields.
    };
    static bool game_states_changed(Server::GameState &new_state,
                                    Server::GameState &old_state);

    bool is_initialized() const;
    Error update_client_connection();
    Error update_listen_socket();

    Error process_incoming_message(const Message &message);
    // The server must have an active and ready listen connection in order to
    // send outgoing messages. If not, either ONE_ERROR_SERVER_CONNECTION_IS_NULLPTR or
    // ONE_ERROR_SERVER_CONNECTION_NOT_READY is returned and the message is
    // not sent.
    Error process_outgoing_message(const Message &message);

    Error send_live_state();
    Error send_application_instance_status();

    Socket *_listen_socket;
    Socket *_client_socket;
    Connection *_client_connection;

    bool _is_waiting_for_client;

    GameState _game_state;
    GameState _last_sent_game_state;
    bool _game_state_was_set;

    ApplicationInstanceStatus _status;
    bool _should_send_status;

    callback::ServerCallbacks _callbacks;

    mutable std::mutex _server;
};

}  // namespace one
}  // namespace i3d
