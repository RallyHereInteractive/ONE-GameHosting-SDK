#pragma once

#include <one/game/one_server_wrapper.h>

#include <chrono>
#include <functional>
#include <string>
#include <mutex>

using namespace std::chrono;

namespace one_integration {

/// A fake Game server. It owns a OneServerWrapper member that encapsulates
/// the c_api.h of the arcus server.
///
/// The Fake Game's purpose is to:
/// - Facilitate automated and manual integration testing of the SDK
/// - Illustrate an integration
///
/// Please note that this particular class is a completely fake game, and its
/// interface is not meant to represent those found in real games. Its public
/// surface's main goal is to assist integration-style testing. See the use of
/// its _one_server member variable for integration functionality.
class Game final {
public:
    Game();
    Game(const Game &) = delete;
    Game &operator=(const Game &) = delete;
    ~Game();

    //------------
    // Life cycle.

    bool init(unsigned int port, int max_players, const std::string &name,
              const std::string &map, const std::string &mode,
              const std::string &version);
    void shutdown();

    void alter_game_state();

    void update();

    //--------------------------------------------------------------------------
    // Query how many times message game information has been requested from the
    // game by the One platform.

    int soft_stop_receive_count() const;
    int allocated_receive_count() const;
    int metadata_receive_count() const;
    int host_information_receive_count() const;
    int application_instance_information_receive_count() const;

    // Exposed for testing purposes, however use of this should be kept to a
    // minimum or removed. The game, as much as reasonable, should be tested as
    // a black box.
    OneServerWrapper &one_server_wrapper() {
        return _one_server;
    }

    // Exposed for testing purposes to avoid spamming std::error when testing for expected
    // failures.
    void set_quiet(bool quiet) {
        _quiet = quiet;
    }

    // Exposed for fake game executable to enable exiting the process when a
    // soft stop message is received from the One platform. If enabled, the
    // process will end at a random time from the time of message receipt up
    // to 2x the given timeout.
    void set_process_exit_enabled(bool enabled) {
        _is_exit_time_enabled = enabled;
    }

    // Exposed for testing purposes to avoid spamming std::error when testing for expected
    // failures.
    bool is_quiet() const {
        return _quiet;
    }

    int player_count() const {
        return _players;
    }
    void set_player_count(int count);

private:
    static void soft_stop_callback(int timeout, void *userdata);
    static void allocated_callback(const OneServerWrapper::AllocatedData &data,
                                   void *userdata);
    static void metadata_callback(const OneServerWrapper::MetaDataData &data,
                                  void *userdata);
    static void host_information_callback(
        const OneServerWrapper::HostInformationData &data, void *userdata);
    static void application_instance_information_callback(
        const OneServerWrapper::ApplicationInstanceInformationData &data, void *userdata);

    void update_match();
    void update_arcus_server_game_state();

    OneServerWrapper _one_server;

    mutable std::mutex _game;

    int _soft_stop_receive_count;
    int _allocated_receive_count;
    int _metadata_receive_count;
    int _host_information_receive_count;
    int _application_instance_information_receive_count;

    // Log level.
    bool _quiet;

    //-----------------------------------
    // Simulated game behavior and state.

    // Example match configuration extracted from the allocated message received
    // from the one platform.
    int _max_players;
    std::chrono::seconds _match_duration;
    bool _player_joining_match;
    bool _player_playing_match;
    bool _player_leaving_match;
    steady_clock::time_point _match_start_time;

    // Current game state.
    int _players;
    std::string _name;
    std::string _map;
    std::string _mode;
    std::string _version;

    // A planned time to exit the process, configured as a response to a
    // soft_stop message from the one platform.
    steady_clock::time_point _exit_time;
    bool _is_exit_time_enabled;

    // These states match the expectation of the one platform. Allocated is
    // optional but this example uses it to respond to the allocated message
    // as a matchmaking request.
    enum class MatchmakingStatus { none, starting, online, allocated };
    MatchmakingStatus _matchmaking_status;
    MatchmakingStatus _previous_matchmaking_status;
};

namespace allocation {
// Return number of calls to alloc made on the custom allocator.
size_t alloc_count();

// Returns number of calls to free made on the custom allocator.
size_t free_count();
}  // namespace allocation

}  // namespace one_integration
