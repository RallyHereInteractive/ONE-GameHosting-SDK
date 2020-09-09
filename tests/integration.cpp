#include <catch.hpp>
#include <util.h>

#include <one/agent/agent.h>
#include <one/arcus/array.h>
#include <one/arcus/error.h>
#include <one/arcus/internal/connection.h>
#include <one/arcus/internal/health.h>
#include <one/arcus/message.h>
#include <one/arcus/object.h>
#include <one/arcus/server.h>
#include <one/game/game.h>

#include <iostream>

using namespace game;
using namespace one;

bool error_callback_has_been_called = false;
bool live_state_callback_has_been_called = false;
bool allocated_callback_has_been_called = false;
bool meta_data_callback_has_been_called = false;
bool host_information_callback_has_been_called = false;
bool soft_stop_callback_has_been_called = false;

void error_callback(void *) {
    std::cout << "error_callback!" << std::endl;
    error_callback_has_been_called = true;
}

void live_state_callback(void *, int player, int max_player, const std::string &name,
                         const std::string &map, const std::string &mode,
                         const std::string &version) {
    std::cout << "send_live_callback:"
              << " player " << player << " max_player : " << max_player
              << " name: " << name << " map: " << map << " mode: " << mode
              << " version: " << version << std::endl;
    live_state_callback_has_been_called = true;
}

void allocated_callback(void *, void *) {
    std::cout << "allocated_callback!" << std::endl;
    allocated_callback_has_been_called = true;
}

void meta_data_callback(void *, void *) {
    std::cout << "meta_data_callback!" << std::endl;
    allocated_callback_has_been_called = true;
}

void host_information_callback(void *) {
    std::cout << "host_information_callback!" << std::endl;
    host_information_callback_has_been_called = true;
}

void soft_stop_callback(void *, int timeout) {
    std::cout << "soft_stop_callback:"
              << " timeout " << timeout << std::endl;
    soft_stop_callback_has_been_called = true;
}

void pump_updates(Agent &agent, Game &game) {
    for_sleep(10, 1, [&]() {
        REQUIRE(game.update());
        REQUIRE(agent.update() == 0);
        if (agent.client().status() == Client::Status::ready &&
            game.one_server_wrapper().status() == OneServerWrapper::Status::ready)
            return true;
        return false;
    });
    REQUIRE(game.one_server_wrapper().status() == OneServerWrapper::Status::ready);
    REQUIRE(agent.client().status() == Client::Status::ready);
}

// Todo: disabled. This test doesn't appear to test connection failure or add value.
TEST_CASE("Agent connection failure", "[.][integration]") {
    const auto address = "127.0.0.1";
    const unsigned int port = 19001;

    Game game(port);
    REQUIRE(game.init(1, 16, "name", "map", "mode", "version"));
    REQUIRE(game.one_server_wrapper().status() ==
            OneServerWrapper::Status::waiting_for_client);

    Agent agent;
    REQUIRE(agent.init(address, port) == 0);

    REQUIRE(game.update());
    REQUIRE(agent.update() == ONE_ERROR_NONE);
    REQUIRE(agent.client().status() == Client::Status::handshake);
}

TEST_CASE("Agent connects to a game & send requests", "[integration]") {
    const auto address = "127.0.0.1";
    const unsigned int port = 19002;

    Game game(port);
    REQUIRE(game.init(1, 16, "name", "map", "mode", "version"));
    REQUIRE(game.one_server_wrapper().status() ==
            OneServerWrapper::Status::waiting_for_client);

    Agent agent;
    REQUIRE(agent.init(address, port) == 0);
    REQUIRE(agent.update() == ONE_ERROR_NONE);
    REQUIRE(agent.client().status() == Client::Status::handshake);
    REQUIRE(agent.set_error_response_callback(error_callback, nullptr) == 0);
    REQUIRE(agent.set_live_state_response_callback(live_state_callback, nullptr) == 0);
    REQUIRE(agent.set_host_information_request_callback(host_information_callback,
                                                        nullptr) == 0);

    pump_updates(agent, game);

    // FIXME: remove one the messages are sent over the socket.
    game.shutdown();
    return;

    {
        REQUIRE(agent.send_soft_stop_request(1000) == 0);
        for_sleep(5, 1, [&]() {
            REQUIRE(agent.update() == 0);
            REQUIRE(game.update());
            return soft_stop_callback_has_been_called;
        });
        REQUIRE(soft_stop_callback_has_been_called == true);
    }

    // error_response
    // Todo - game.fake_error(), and why is it called a response if it is active?
    // {
    //     REQUIRE(game.send_error_response() == 0);
    //     REQUIRE(agent.update() == 0);
    //     sleep(10);
    //     REQUIRE(game.update());
    //     REQUIRE(error_callback_has_been_called == true);
    // }

    // live_state_request
    {
        REQUIRE(agent.send_live_state_request() == 0);
        REQUIRE(agent.update() == 0);
        sleep(10);
        REQUIRE(game.update());
        REQUIRE(live_state_callback_has_been_called == true);
    }

    // allocated_request
    {
        Array array;
        REQUIRE(agent.send_allocated_request(&array) == 0);
        REQUIRE(agent.update() == 0);
        sleep(10);
        REQUIRE(game.update());
        REQUIRE(allocated_callback_has_been_called == true);
    }

    // meta_data_request
    {
        Array array;
        REQUIRE(agent.send_meta_data_request(&array) == 0);
        REQUIRE(agent.update() == 0);
        sleep(10);
        REQUIRE(game.update());
        REQUIRE(meta_data_callback_has_been_called == true);
    }

    // host_information_request
    // Todo - should this be from agent to game?
    // {
    //     REQUIRE(game.send_host_information_request() == 0);
    //     REQUIRE(game.update() == 0);
    //     sleep(10);
    //     REQUIRE(agent.update());
    //     REQUIRE(host_information_callback_has_been_called == true);
    // }

    // TODO: add more agent request & custom messages.

    game.shutdown();
}

TEST_CASE("long:Handshake timeout", "[integration]") {
    const auto address = "127.0.0.1";
    const unsigned int port = 19003;

    Game game(port);
    REQUIRE(game.init(1, 16, "test game", "test map", "test mode", "test version"));

    Agent agent;
    REQUIRE(agent.init(address, port) == 0);
    // Update agent and game one time to initiate the connection.
    REQUIRE(agent.update() == ONE_ERROR_NONE);
    REQUIRE(game.update() == ONE_ERROR_NONE);
    REQUIRE(game.one_server_wrapper().status() == OneServerWrapper::Status::handshake);

    // Update the game only, so that the agent doesn't progress the handshake,
    // until the handshake timeout expires.
    int ms = Connection::handshake_timeout_seconds * 1000;
    for_sleep(5, ms / 4, [&]() {
        REQUIRE(game.update() == 0);
        if (game.one_server_wrapper().status() ==
            OneServerWrapper::Status::waiting_for_client)
            return true;
        return false;
    });
    REQUIRE(game.one_server_wrapper().status() ==
            OneServerWrapper::Status::waiting_for_client);

    // Shut down the game and restart it, the client should reconnect automatically.
    game.shutdown();
}

TEST_CASE("long:Reconnection", "[integration]") {
    const auto address = "127.0.0.1";
    const unsigned int port = 19003;

    Game game(port);
    REQUIRE(game.init(1, 16, "test game", "test map", "test mode", "test version"));

    Agent agent;
    REQUIRE(agent.init(address, port) == 0);

    pump_updates(agent, game);

    // Shut down the game and restart it, the client should reconnect automatically.
    game.shutdown();

    // Sleep for long enough such that the next update should result in the agent
    // realizing that the game server is no longer there and the agent should
    // enter a state of reconnecting to the server.
    sleep(HealthChecker::health_check_receive_interval_seconds * 1000);
    REQUIRE(agent.update() == ONE_ERROR_CONNECTION_HEALTH_TIMEOUT);
    REQUIRE(agent.client().status() == Client::Status::connecting);

    // Restart the server and it should be waiting for client.
    REQUIRE(game.init(1, 16, "test game", "test map", "test mode", "test version"));
    REQUIRE(game.one_server_wrapper().status() ==
            OneServerWrapper::Status::waiting_for_client);

    // Update both and the agent should reconnect.
    pump_updates(agent, game);
    REQUIRE(game.one_server_wrapper().status() == OneServerWrapper::Status::ready);
    REQUIRE(agent.client().status() == Client::Status::ready);
}
