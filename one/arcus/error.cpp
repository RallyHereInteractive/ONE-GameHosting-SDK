#include <one/arcus/error.h>

#include <one/arcus/allocator.h>
#include <one/arcus/types.h>

namespace i3d {
namespace one {

#define ERROR_MAP_PAIR(e) e, #e
const char *error_text(OneError err) {
    static ONE_UNORDERED_MAP(OneError, String) lookup = {
        {ERROR_MAP_PAIR(ONE_ERROR_NONE)},
        {ERROR_MAP_PAIR(ONE_ERROR_ARRAY_ALLOCATION_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_ARRAY_POSITION_OUT_OF_BOUNDS)},
        {ERROR_MAP_PAIR(ONE_ERROR_ARRAY_WRONG_TYPE_IS_EXPECTING_ARRAY)},
        {ERROR_MAP_PAIR(ONE_ERROR_ARRAY_WRONG_TYPE_IS_EXPECTING_BOOL)},
        {ERROR_MAP_PAIR(ONE_ERROR_ARRAY_WRONG_TYPE_IS_EXPECTING_INT)},
        {ERROR_MAP_PAIR(ONE_ERROR_ARRAY_WRONG_TYPE_IS_EXPECTING_OBJECT)},
        {ERROR_MAP_PAIR(ONE_ERROR_ARRAY_WRONG_TYPE_IS_EXPECTING_STRING)},
        {ERROR_MAP_PAIR(ONE_ERROR_CLIENT_NOT_INITIALIZED)},
        {ERROR_MAP_PAIR(ONE_ERROR_CODEC_HEADER_LENGTH_TOO_SMALL)},
        {ERROR_MAP_PAIR(ONE_ERROR_CODEC_HEADER_LENGTH_TOO_BIG)},
        {ERROR_MAP_PAIR(ONE_ERROR_CODEC_DATA_LENGTH_TOO_SMALL_FOR_HEADER)},
        {ERROR_MAP_PAIR(ONE_ERROR_CODEC_DATA_LENGTH_TOO_SMALL_FOR_PAYLOAD)},
        {ERROR_MAP_PAIR(ONE_ERROR_CODEC_EXPECTED_DATA_LENGTH_TOO_BIG)},
        {ERROR_MAP_PAIR(ONE_ERROR_CODEC_INVALID_MESSAGE_PAYLOAD_SIZE_TOO_BIG)},
        {ERROR_MAP_PAIR(ONE_ERROR_CODEC_INVALID_HEADER)},
        {ERROR_MAP_PAIR(ONE_ERROR_CODEC_TRYING_TO_ENCODE_UNSUPPORTED_OPCODE)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_HANDSHAKE_TIMEOUT)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_HEALTH_TIMEOUT)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_HELLO_INVALID)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_HELLO_MESSAGE_HEADER_TOO_BIG)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_HELLO_MESSAGE_RECEIVE_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_HELLO_MESSAGE_REPLY_INVALID)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_HELLO_MESSAGE_SEND_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_HELLO_RECEIVE_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_HELLO_SEND_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_HELLO_TOO_BIG)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_HELLO_VERSION_MISMATCH)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_MESSAGE_RECEIVE_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_MESSAGE_RECIEVE_EXPECTED_SIZE_MISMATCH)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_READ_TOO_BIG_FOR_STREAM)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_OUT_MESSAGE_TOO_BIG_FOR_STREAM)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_QUEUE_EMPTY)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_QUEUE_INSUFFICIENT_SPACE)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_OUTGOING_QUEUE_INSUFFICIENT_SPACE)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_INCOMING_QUEUE_INSUFFICIENT_SPACE)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_RECEIVE_BEFORE_SEND)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_RECEIVE_FAIL)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_SEND_FAIL)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_TRY_AGAIN)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_UNKNOWN_STATUS)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_UPDATE_AFTER_ERROR)},
        {ERROR_MAP_PAIR(ONE_ERROR_CONNECTION_UPDATE_READY_FAIL)},
        {ERROR_MAP_PAIR(ONE_ERROR_MESSAGE_ALLOCATION_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_MESSAGE_CALLBACK_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_MESSAGE_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_MESSAGE_OPCODE_NOT_MATCHING_EXPECTING_ALLOCATED)},
        {ERROR_MAP_PAIR(ONE_ERROR_MESSAGE_OPCODE_NOT_MATCHING_EXPECTING_LIVE_STATE)},
        {ERROR_MAP_PAIR(ONE_ERROR_MESSAGE_OPCODE_NOT_MATCHING_EXPECTING_METADATA)},
        {ERROR_MAP_PAIR(ONE_ERROR_MESSAGE_OPCODE_NOT_MATCHING_EXPECTING_SOFT_STOP)},
        {ERROR_MAP_PAIR(
            ONE_ERROR_MESSAGE_OPCODE_NOT_MATCHING_EXPECTING_HOST_INFORMATION)},
        {ERROR_MAP_PAIR(
            ONE_ERROR_MESSAGE_OPCODE_NOT_MATCHING_EXPECTING_APPLICATION_INSTANCE_INFORMATION)},
        {ERROR_MAP_PAIR(
            ONE_ERROR_MESSAGE_OPCODE_NOT_MATCHING_EXPECTING_APPLICATION_INSTANCE_STATUS)},
        {ERROR_MAP_PAIR(ONE_ERROR_MESSAGE_OPCODE_NOT_SUPPORTED)},
        {ERROR_MAP_PAIR(ONE_ERROR_MESSAGE_OPCODE_PAYLOAD_NOT_EMPTY)},
        {ERROR_MAP_PAIR(ONE_ERROR_OBJECT_ALLOCATION_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_OBJECT_KEY_NOT_FOUND)},
        {ERROR_MAP_PAIR(ONE_ERROR_OBJECT_KEY_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_OBJECT_WRONG_TYPE_IS_EXPECTING_ARRAY)},
        {ERROR_MAP_PAIR(ONE_ERROR_OBJECT_WRONG_TYPE_IS_EXPECTING_BOOL)},
        {ERROR_MAP_PAIR(ONE_ERROR_OBJECT_WRONG_TYPE_IS_EXPECTING_INT)},
        {ERROR_MAP_PAIR(ONE_ERROR_OBJECT_WRONG_TYPE_IS_EXPECTING_OBJECT)},
        {ERROR_MAP_PAIR(ONE_ERROR_OBJECT_WRONG_TYPE_IS_EXPECTING_STRING)},
        {ERROR_MAP_PAIR(ONE_ERROR_PAYLOAD_KEY_NOT_FOUND)},
        {ERROR_MAP_PAIR(ONE_ERROR_PAYLOAD_KEY_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_PAYLOAD_PARSE_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_PAYLOAD_STRING_COPY_FAIED)},
        {ERROR_MAP_PAIR(ONE_ERROR_PAYLOAD_VAL_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_PAYLOAD_WRONG_TYPE_IS_EXPECTING_ARRAY)},
        {ERROR_MAP_PAIR(ONE_ERROR_PAYLOAD_WRONG_TYPE_IS_EXPECTING_BOOL)},
        {ERROR_MAP_PAIR(ONE_ERROR_PAYLOAD_WRONG_TYPE_IS_EXPECTING_INT)},
        {ERROR_MAP_PAIR(ONE_ERROR_PAYLOAD_WRONG_TYPE_IS_EXPECTING_OBJECT)},
        {ERROR_MAP_PAIR(ONE_ERROR_PAYLOAD_WRONG_TYPE_IS_EXPECTING_STRING)},
        {ERROR_MAP_PAIR(ONE_ERROR_SERVER_ALLOCATION_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SERVER_ALREADY_INITIALIZED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SERVER_CALLBACK_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_SERVER_CONNECTION_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_SERVER_CONNECTION_NOT_READY)},
        {ERROR_MAP_PAIR(ONE_ERROR_SERVER_MESSAGE_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_SERVER_SOCKET_ALLOCATION_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SERVER_SOCKET_NOT_INITIALIZED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SERVER_SOCKET_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_ACCEPT_NON_BLOCKING_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_ACCEPT_UNINITIALIZED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_ADDRESS_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_AVAILABLE_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_BIND_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_CLOSE_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_CONNECT_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_CONNECT_NON_BLOCKING_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_CONNECT_UNINITIALIZED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_CREATE_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_LISTEN_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_LISTEN_NON_BLOCKING_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_RECEIVE_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_SELECT_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_SELECT_UNINITIALIZED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_SEND_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_SOCKET_OPTIONS_FAILED)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_SYSTEM_CLEANUP_FAIL)},
        {ERROR_MAP_PAIR(ONE_ERROR_SOCKET_SYSTEM_INIT_FAIL)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_ARRAY_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_CALLBACK_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_CAPACITY_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_CODE_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_CONNECTION_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_DATA_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_DESTINATION_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_EMPTY_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_KEY_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_MAP_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_MESSAGE_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_MODE_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_NAME_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_OBJECT_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_RESULT_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_SERVER_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_SOCKET_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_SOURCE_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_STATUS_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_SIZE_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_VAL_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_VAL_SIZE_IS_TOO_SMALL)},
        {ERROR_MAP_PAIR(ONE_ERROR_VALIDATION_VERSION_IS_NULLPTR)},
        {ERROR_MAP_PAIR(ONE_ERROR_COUNT)}};
    auto it = lookup.find(err);
    if (it == lookup.end()) {
        return "";
    }
    return it->second.c_str();
}

}  // namespace one
}  // namespace i3d
