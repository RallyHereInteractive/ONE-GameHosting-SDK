#include <one/game/game.h>

#include <assert.h>
#include <string>

#include <one/game/log.h>

namespace game {

Game::Game(unsigned int port)
    : _server(port)
    , _soft_stop_call_count(0)
    , _allocated_call_count(0)
    , _meta_data_call_count(0)
    , _host_information_call_count(0)
    , _application_instance_information_call_count(0)
    , _application_instance_get_status_call_count(0)
    , _application_instance_set_status_call_count(0)
    , _quiet(false)
    , _players(0)
    , _max_players(0)
    , _starting(false)
    , _online(false)
    , _allocated(false) {}

Game::~Game() {
    _server.shutdown();
}

bool Game::init(int max_players, const std::string &name, const std::string &map,
                const std::string &mode, const std::string &version) {
    const std::lock_guard<std::mutex> lock(_game);
    if (!_server.init()) {
        L_ERROR("failed to init server");
        return false;
    }

    _players = 0;  // Game starts with 0 active players.
    _max_players = max_players;
    _name = name;
    _map = map;
    _mode = mode;
    _version = version;

    OneServerWrapper::GameState state;
    state.players = 0;  // Game starts with 0 active players.
    state.max_players = _max_players;
    state.name = _name;
    state.map = _map;
    state.mode = _mode;
    state.version = _version;
    _server.set_game_state(state);

    _server.set_soft_stop_callback(soft_stop_callback, this);
    _server.set_allocated_callback(allocated_callback, this);
    _server.set_meta_data_callback(meta_data_callback, this);
    _server.set_host_information_callback(host_information_callback, this);
    _server.set_application_instance_information_callback(
        application_instance_information_callback, this);
    _server.set_application_instance_get_status_callback(
        application_instance_get_status_callback, this);
    _server.set_application_instance_set_status_callback(
        application_instance_set_status_callback, this);

    return (_server.status() == OneServerWrapper::Status::waiting_for_client);
}

void Game::shutdown() {
    const std::lock_guard<std::mutex> lock(_game);
    _server.shutdown();
}

void Game::alter_game_state() {
    const std::lock_guard<std::mutex> lock(_game);

    // This is mainly to emulate a very simple game change (i.e.: changing both the number
    // of player & status.
    // In a real life senario the game would use its own mecahnisms to get the number of
    // players, current maps, etc...

    // The number of player is arbitrarily changed to trigger player joined & left
    // messages.
    if (_max_players < _players + 1) {
        _players = 0;
    } else {
        _players += 1;
    }

    OneServerWrapper::GameState new_state;
    new_state.players = _players;
    new_state.max_players = _max_players;
    new_state.name = _name;
    new_state.map = _map;
    new_state.mode = _mode;
    new_state.version = _version;

    _server.set_game_state(new_state);

    // Updating the server status incrementally.
    // First time it is set as starting.
    // Second time it is set as online.
    // Third time it is set as allocated.
    // The progession order is the good one, but the timing is arbitrarily and might
    // change depending on the game startup sequence.
    if (!_allocated && _online && _starting) {
        if (!_server.send_application_instance_set_status(
                OneServerWrapper::StatusCode::allocated)) {
            if (!_quiet) L_ERROR("failed to send set status code allocated");
        } else {
            _allocated = true;
        }
    }

    if (!_online && _starting) {
        if (!_server.send_application_instance_set_status(
                OneServerWrapper::StatusCode::online)) {
            if (!_quiet) L_ERROR("failed to send set status code starting");
        } else {
            _online = true;
        }
    }

    if (!_starting) {
        if (!_server.send_application_instance_set_status(
                OneServerWrapper::StatusCode::starting)) {
            if (!_quiet) L_ERROR("failed to send set status code starting");
        } else {
            _starting = true;
        }
    }

    // This get status is only sent to show how the game can get the current status.
    // The frequency of it being sent should be a lot less in a real life senario.
    if (!_server.send_application_instance_get_status()) {
        if (!_quiet) L_ERROR("failed to send get status code");
    }
}

void Game::update() {
    const std::lock_guard<std::mutex> lock(_game);
    _server.update();
}

int Game::soft_stop_call_count() const {
    const std::lock_guard<std::mutex> lock(_game);
    return _soft_stop_call_count;
}

int Game::allocated_call_count() const {
    const std::lock_guard<std::mutex> lock(_game);
    return _allocated_call_count;
}

int Game::meta_data_call_count() const {
    const std::lock_guard<std::mutex> lock(_game);
    return _meta_data_call_count;
}

int Game::host_information_call_count() const {
    const std::lock_guard<std::mutex> lock(_game);
    return _host_information_call_count;
}

int Game::application_instance_information_call_count() const {
    const std::lock_guard<std::mutex> lock(_game);
    return _application_instance_information_call_count;
}

int Game::application_instance_get_status_call_count() const {
    const std::lock_guard<std::mutex> lock(_game);
    return _application_instance_get_status_call_count;
}

int Game::application_instance_set_status_call_count() const {
    const std::lock_guard<std::mutex> lock(_game);
    return _application_instance_set_status_call_count;
}

void Game::soft_stop_callback(int timeout, void *userdata) {
    L_INFO("soft stop called:");
    L_INFO("\ttimeout:" + std::to_string(timeout));
    auto game = reinterpret_cast<Game *>(userdata);
    if (game == nullptr) {
        L_ERROR("null userdata");
        return;
    }
    game->_soft_stop_call_count++;
}

void Game::allocated_callback(const OneServerWrapper::AllocatedData &data,
                              void *userdata) {
    L_INFO("allocated called:");
    L_INFO("\tmax_players:" + data.max_players);
    L_INFO("\tmap:" + data.map);
    auto game = reinterpret_cast<Game *>(userdata);
    if (game == nullptr) {
        L_ERROR("null userdata");
        return;
    }
    game->_allocated_call_count++;
}

void Game::meta_data_callback(const OneServerWrapper::MetaDataData &data,
                              void *userdata) {
    L_INFO("meta data called:");
    L_INFO("\tmap:" + data.map);
    L_INFO("\tmode:" + data.mode);
    L_INFO("\ttype:" + data.type);
    auto game = reinterpret_cast<Game *>(userdata);
    if (game == nullptr) {
        L_ERROR("null userdata");
        return;
    }
    game->_meta_data_call_count++;
}

void Game::host_information_callback(const OneServerWrapper::HostInformationData &data,
                                     void *) {
    L_INFO("host information called:");
    L_INFO("\tid:" + std::to_string(data.id));
    L_INFO("\tserver id:" + std::to_string(data.server_id));
    L_INFO("\tserver name:" + data.server_name);
}

void Game::application_instance_information_callback(
    const OneServerWrapper::ApplicationInstanceInformationData &data, void *) {
    L_INFO("application instance information called:");
    L_INFO("\tfleet id:" + data.fleet_id);
    L_INFO("\thost id:" + std::to_string(data.host_id));
    L_INFO("\tis virtual:" + std::to_string(data.is_virtual));
}

void Game::application_instance_get_status_callback(
    const OneServerWrapper::ApplicationInstanceGetStatusData &data, void *) {
    L_INFO("application instance get status called:");
    L_INFO("\tstatus:" + std::to_string(data.status));
}

void Game::application_instance_set_status_callback(
    const OneServerWrapper::ApplicationInstanceSetStatusData &data, void *) {
    L_INFO("application instance set status called:");
    L_INFO("\tcode:" + std::to_string(static_cast<int>(data.code)));
}

}  // namespace game
