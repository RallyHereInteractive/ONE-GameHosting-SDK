#include <one/game/game.h>

#include <one/arcus/message.h>

#include <assert.h>

namespace game {

Game::Game(unsigned int port, int players, int max_players, const std::string &name,
           const std::string &map, const std::string &mode, const std::string &version)
    : _server(port)
    , _players(players)
    , _max_players(max_players)
    , _name(name)
    , _map(map)
    , _mode(mode)
    , _version(version) {}

Game::~Game() {
    _server.shutdown();
}

int Game::init() {
    _server.init();
    return (_server.status() == OneServerWrapper::Status::waiting_for_client) ? 0 : -1;
}

int Game::shutdown() {
    _server.shutdown();
    return 0;
}

int Game::update() {
    OneServerWrapper::GameState game_state = {0};
    game_state.players = _players;
    game_state.max_players = _max_players;
    game_state.name = _name;
    game_state.map = _map;
    game_state.mode = _mode;
    game_state.version = _version;
    _server.set_game_state(game_state);
    _server.update();
    return 0;
}

}  // namespace game
