
#include "poll.h"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpTypes.h>
#include <json/value.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <memory>
#include <mutex>
#include <print>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace HttpPoll {
// Keep the client alive to reuse TCP connections
static drogon::HttpClientPtr shared_client;

void init() {
    // 1. Initialize the client
    // shared_client = drogon::HttpClient::newHttpClient("https://build-it-together.click/");

    // 2. Schedule the recurring task
    /*
    drogon::app().getLoop()->runEvery(std::chrono::milliseconds(500), []() {
        pollData();
    });
    */

    // 3. START THE EVENT LOOP
    // We launch this in a separate thread so it doesn't block your ImGui/Main
    // loop.
    std::thread dr_thread([]() { drogon::app().run(); });

    // Detach so the thread lives as long as the process
    dr_thread.detach();
}

void pollData() {
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/");

    // Note: No need to pass 'url' if using the shared_client
    shared_client->sendRequest(
        req, [](drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
            if (result == drogon::ReqResult::Ok) {
                std::cout << "Exeecuting Callback";
                std::lock_guard<std::mutex> lock(data_mtx);
                data.emplace(response->getBody());
                is_data_available.store(true);
            }
        });
}

// std::pair<const std::variant<std::string, Json::Value>&, std::mutex&> Poll::getBody()
//     const noexcept {
//     return {response_body, res_body_mtx};
// }

bool Poll::is_data_available() const noexcept {
    return is_new_data_available.load();
}

Poll::Poll(std::string_view remote_url, std::string_view endpoint, uint16_t port)
    : res_body_mtx(std::make_shared<std::mutex>()),
      remote_url(remote_url),
      endpoint(endpoint),
      port(port) {
    client = HttpClient::newHttpClient(this->remote_url, port);
    request = HttpRequest::newHttpRequest();
    request->setPath(this->endpoint);
    timer_id = drogon::app().getLoop()->runEvery(poll_interval, [this]() {
        client->sendRequest(
            request,
            [this](drogon::ReqResult result, const HttpResponsePtr& response) {
                if (result != drogon::ReqResult::Ok) {
                    std::cerr << "request Failed\n";
                    return;
                }
                std::lock_guard<std::mutex> _lock(*res_body_mtx);
                // response_body.emplace<std::string>(response->body());
                json_ptr = response->getJsonObject();
                is_new_data_available.store(true);
            },
            timeout);
    });
}
Poll::~Poll() {
    drogon::app().getLoop()->invalidateTimer(timer_id);  // Stops the Polling task
}

void Poll::parsed_data() noexcept {
    is_new_data_available.store(false);
}

std::pair<std::shared_ptr<Json::Value>, std::shared_ptr<std::mutex>>
Poll::getJSONBodyPtr() noexcept {
    std::lock_guard<std::mutex> _lock(*res_body_mtx);
    return std::make_pair(json_ptr, res_body_mtx);
}
std::function<void(const std::string&, drogon::HttpMethod, const std::string&)>
Poll::getButtonCallback() {
    auto callback = [this](const std::string& _endpoint, drogon::HttpMethod _method,
                           const std::string& _body) {
        /*
        std::string path = remote_url + _endpoint;
        std::cerr << "Path before request =>";
        std::cerr << path << '\n';
        */
        HttpRequestPtr request = HttpRequest::newHttpRequest();
        request->setMethod(_method);
        request->setPath(_endpoint);
        if (_body.size())
            request->setBody(_body);
        auto [reqRes, resPtr] = client->sendRequest(request);
        if (reqRes != drogon::ReqResult::Ok) {
            std::cerr << "Request Failed\n";
            std::cerr << std::format("Url => {}\n\n", request->getPath());
        }
    };
    return callback;
}
std::function<void(const std::string&, std::string&, std::atomic<bool>&)> Poll::getImagePoller() {
    auto poller = [this](const std::string& _enpoint, std::string& img_buf,
                         std::atomic<bool>& is_new_data_available) {
        HttpRequestPtr request = HttpRequest::newHttpRequest();
        request->setMethod(drogon::Get);
        request->setPath(_enpoint);

        auto [reqRes, resPtr] = client->sendRequest(request);
        img_buf = resPtr->getBody();
        is_new_data_available.store(true);
    };
    return poller;
}
void Poll::pollImage(const std::string& _endpoint, std::string& img_buf,
                     std::atomic<bool>& is_new_data_available) {
    HttpRequestPtr img_request = HttpRequest::newHttpRequest();
    img_request->setMethod(drogon::HttpMethod::Get);
    img_request->setPath(_endpoint);

    client->sendRequest(img_request,
                        [&is_new_data_available, &img_buf](drogon::ReqResult reqRes,
                                                           const drogon::HttpResponsePtr& resPtr) {
                            if (reqRes == drogon::ReqResult::Ok) {
                                img_buf = resPtr->getBody();
                                is_new_data_available.store(true);
                            } else {
                                std::println("Image request failed");
                            }
                        });
}

}  // namespace HttpPoll