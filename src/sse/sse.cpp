#include "sse.h"

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpTypes.h>
#include <json/version.h>

#include <atomic>
#include <mutex>
#include <print>

namespace Sse {

size_t sse_curl_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t bytes = size * nmemb;

    // Retrieve persistent string buffer passed via userdata
    UserData* dataStruct = static_cast<UserData*>(userdata);
    auto& streamBuffer = dataStruct->buffer;
    streamBuffer.append(ptr, bytes);

    // Parse the buffer for the double-newline SSE delimiter
    size_t pos = 0;
    while ((pos = streamBuffer.find("\n\n")) != std::string::npos) {
        std::string eventPayload = streamBuffer.substr(0, pos);
        streamBuffer.erase(0, pos + 2);  // Remove parsed event and \n\n

        // Extract the "data: " portion
        std::string dataPrefix = "data: ";
        size_t dataPos = eventPayload.find(dataPrefix);

        if (dataPos != std::string::npos) {
            std::string actualData = eventPayload.substr(dataPos + dataPrefix.length());
            std::println("received sse data: {}", actualData);
            std::lock_guard<std::mutex> lock_(dataStruct->response_mtx);
            dataStruct->responses.emplace(std::move(actualData));
            dataStruct->is_new_data_available.store(true, std::memory_order_relaxed);
        }
    }

    return bytes;
}

Json::Value extractPayload(std::string_view buffer) {
    size_t startPos = buffer.find("data:");
    size_t endPos = buffer.find("\n\n");
    return Json::Value(std::string(buffer.substr(startPos, endPos)));
}

SSE::SSE(std::string_view remote_url_, std::string_view endpoint_, int port_)
    : remote_url(remote_url_), endpoint(endpoint_), port(port_) {}
}  // namespace Sse