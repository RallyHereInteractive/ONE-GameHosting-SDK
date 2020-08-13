#include <iostream>
#include <one/arcus/internal/version.h>

#include <one/arcus/c_api.h>

int main() {
    std::cout << "Game -> project name: " << ONE_NAME << " version: " << ONE_VERSION << std::endl;
    OneGameHostingApiPtr api = one_game_hosting_api();
    OneServerPtr server{nullptr};

    int error = api->server_api->create(1024, 1024, &server);
    if (error != 0) {
        return error;
    }

    api->server_api->destroy(&server);
    return 0;
}
