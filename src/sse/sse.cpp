#include "sse.h"
#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpTypes.h>
#include <json/version.h>
#include <print>

namespace Sse {

size_t SSE::sse_curl_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t bytes = size * nmemb;
    
    // Retrieve persistent string buffer passed via userdata
    auto* streamBuffer = static_cast<std::string*>(userdata);
    streamBuffer->append(ptr, bytes);

    // Parse the buffer for the double-newline SSE delimiter
    size_t pos = 0;
    while ((pos = streamBuffer->find("\n\n")) != std::string::npos) {
        
        std::string eventPayload = streamBuffer->substr(0, pos);
        streamBuffer->erase(0, pos + 2); // Remove parsed event and \n\n

        // Extract the "data: " portion
        std::string dataPrefix = "data: ";
        size_t dataPos = eventPayload.find(dataPrefix);
        
        if (dataPos != std::string::npos) {
            std::string actualData = eventPayload.substr(dataPos + dataPrefix.length());
            std::println("received sse data: {}", actualData);
            responses.emplace(std::move(actualData));
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
    : remote_url(remote_url_), endpoint(endpoint_), port(port_) {

}
}  // namespace Sse