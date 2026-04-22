#include "sse.h"

#include <curl/curl.h>
#include <curl/easy.h>
#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpTypes.h>
#include <json/version.h>

#include <atomic>
#include <cstdint>
#include <format>
#include <mutex>
#include <print>
#include <thread>
#include <utility>

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

Json::Value extractPayload(std::string_view buffer) noexcept {
    size_t startPos = buffer.find("data:");
    size_t endPos = buffer.find("\n\n");
    return Json::Value(std::string(buffer.substr(startPos, endPos)));
}

SSE::SSE(std::string_view remote_url_, std::string_view endpoint_, int port_)
    : remote_url(remote_url_), endpoint(endpoint_), port(port_) {
    remote = std::format("{}:{}{}", remote_url, port, endpoint);
    client = drogon::HttpClient::newHttpClient(remote_url, static_cast<uint16_t>(port));
    curl = curl_easy_init();
    if(!curl){
        std::println("Error: Can't initialize curl");
    }
    curl_easy_setopt(curl, CURLOPT_URL, remote.c_str());
    // curl_easy_setopt(curl, CURLOPT_PORT, port);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sse_curl_callback);

    connection_thread = std::thread([](CURL* curl){
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }, curl);
}
Json::Value SSE::getJson(){
    std::lock_guard<std::mutex> lock_(data.response_mtx);
    auto temp = std::move(data.responses.front());
    data.responses.pop();
    if (data.responses.size() > 0) {
        data.is_new_data_available.store(true);
    }else{
        data.is_new_data_available.store(false);
    }
    return temp;
}

void SSE::pollImage(const std::string& _endpoint, std::string& img_buf,
                     std::mutex& img_buf_mtx, std::atomic<bool>& is_new_data_available) {
    drogon::HttpRequestPtr img_request = drogon::HttpRequest::newHttpRequest();
    img_request->setMethod(drogon::HttpMethod::Get);
    img_request->setPath(_endpoint);
    
    client->sendRequest(img_request,
                        [&is_new_data_available, &img_buf, &img_buf_mtx](drogon::ReqResult reqRes,
                                                           const drogon::HttpResponsePtr& resPtr) {
                            if (reqRes == drogon::ReqResult::Ok) {
                                std::lock_guard<std::mutex> _lock(img_buf_mtx);
                                img_buf = resPtr->getBody();
                                is_new_data_available.store(true);
                            } else {
                                std::println("Image request failed");
                            }
                        });
}
bool SSE::is_data_available() const noexcept{
    return data.is_new_data_available.load();
}
}  // namespace Sse