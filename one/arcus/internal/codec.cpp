#include <one/arcus/internal/codec.h>

#include <one/arcus/internal/message.h>
#include <one/arcus/message.h>

#include <cstring>
#include <algorithm>

namespace one {
namespace codec {

const Hello hello = Hello{{'a', 'r', 'c', 0}, (char)0x1, 0};  // namespace codec

bool validate_hello(const Hello &other) {
    const auto cmp = std::memcmp(&hello, &other, hello_size());
    return cmp == 0;
}

const Hello &valid_hello() {
    return hello;
}

bool validate_header(const Header &header) {
    // Minimal validation in the codec at the moment. Opcode will be handled
    // by message layer. Length will be handled by document reader.
    // Flags are not used at the moment and should be zero.
    bool is_valid = true;
    is_valid &= header.flags == (char)0x0;
    is_valid &= is_opcode_supported(static_cast<Opcode>(header.opcode));
    return is_valid;
}

Error data_to_message(const void *data, size_t length, Header &header, Message &message) {
    // handle byte order to a specific order for wire

    if (length < header_size()) {
        return ONE_ERROR_CODEC_DATA_LENGTH_TOO_SMALL;
    }

    if (header_size() + payload_max_size() < length) {
        return ONE_ERROR_CODEC_DATA_LENGTH_TOO_BIG;
    }

    header = {0};
    auto err = data_to_header(data, header_size(), header);
    if (is_error(err)) return err;

    const size_t expected_length = header_size() + header.length;

    if (header_size() + payload_max_size() < header.length) {
        return ONE_ERROR_CODEC_EXPECTED_DATA_LENGTH_TOO_BIG;
    }

    if (length != expected_length) {
        return ONE_ERROR_CODEC_DATA_LENGTH_NOT_MATCHING_EXPECTED_LENGTH;
    }

    Payload payload;

    if (0 < header.length) {
        const size_t payload_length = header.length;
        const char *payload_data = static_cast<const char *>(data) + codec::header_size();
        err = payload.from_json({payload_data, payload_length});
        if (is_error(err)) return err;
    }

    const Opcode code = static_cast<Opcode>(header.opcode);
    err = message.init(code, payload);
    if (is_error(err)) {
        message.reset();
        return err;
    }

    return ONE_ERROR_NONE;
}

Error message_to_data(const Message &message, size_t &data_length,
                      std::array<char, header_size() + payload_max_size()> &data) {
    // Todo: handle byte order to a specific order for wire

    Header header{0};
    header.opcode = static_cast<char>(message.code());

    std::array<char, payload_max_size()> payload_data;
    size_t payload_length = 0;
    auto err = payload_to_data(message.payload(), payload_length, payload_data);
    if (is_error(err)) return err;

    header.length = payload_length;

    std::array<char, header_size()> header_data;
    err = header_to_data(header, header_data);
    if (is_error(err)) return err;

    data_length = header_size() + payload_length;
    std::copy(header_data.cbegin(), header_data.cend(), data.begin());

    if (0 < payload_length) {
        std::copy_n(payload_data.cbegin(), payload_length, data.begin() + header_size());
    }

    return ONE_ERROR_NONE;
}

Error data_to_header(const void *data, size_t length, Header &header) {
    // handle byte order to a specific order for wire

    if (length < header_size()) {
        return ONE_ERROR_CODEC_HEADER_LENGTH_TOO_SMALL;
    }

    if (header_size() < length) {
        return ONE_ERROR_CODEC_HEADER_LENGTH_TOO_BIG;
    }

    std::memcpy(&header, data, length);

    if (!validate_header(header)) {
        return ONE_ERROR_CODEC_INVALID_HEADER;
    }

    return ONE_ERROR_NONE;
}

Error header_to_data(const Header &header, std::array<char, header_size()> &data) {
    // Todo: handle byte order to a specific order for wire

    if (!validate_header(header)) {
        return ONE_ERROR_CODEC_INVALID_HEADER;
    }

    std::memcpy(data.data(), &header, header_size());
    return ONE_ERROR_NONE;
}

Error data_to_payload(const void *data, size_t length, Payload &payload) {
    // Todo: handle byte order to a specific order for wire

    if (payload_max_size() < length) {
        return ONE_ERROR_CODEC_INVALID_MESSAGE_PAYLOAD_SIZE_TOO_BIG;
    }

    if (length == 0) {
        payload.clear();
        return ONE_ERROR_NONE;
    }

    auto err = payload.from_json({static_cast<const char *>(data), length});
    if (is_error(err)) return err;

    return ONE_ERROR_NONE;
}

Error payload_to_data(const Payload &payload, size_t &payload_length,
                      std::array<char, payload_max_size()> &data) {
    // Todo: handle byte order to a specific order for wire

    if (payload.is_empty()) {
        payload_length = 0;
        return ONE_ERROR_NONE;
    }

    auto str = payload.to_json();
    payload_length = str.size();

    if (payload_max_size() < payload_length) {
        return ONE_ERROR_CODEC_INVALID_MESSAGE_PAYLOAD_SIZE_TOO_BIG;
    }

    std::memcpy(data.data(), str.c_str(), payload_length);
    return ONE_ERROR_NONE;
}

}  // namespace codec
}  // namespace one
