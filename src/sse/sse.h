// Handles server send events and parsing, try not to modify the http polling
#include <curl/curl.h>
#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <json/value.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
namespace Sse {
struct UserData {
    std::queue<Json::Value> responses;
    std::mutex response_mtx;
    std::string buffer;
    std::atomic<bool> is_new_data_available = 0;
};

Json::Value extractPayload(std::string_view buffer) noexcept;

size_t sse_curl_callback(char* ptr, size_t size, size_t nmemb, void* userdata);

class SSE {
protected:
    // Don't access UserData.buffer outside the connection thread
    UserData data;
    std::string remote_url, endpoint, remote;
    uint64_t port;
    CURL* curl;
    // have a drogon client for standard http requests
    drogon::HttpClientPtr client; 

public:
    SSE(std::string_view remote_url_, std::string_view endpoint_, int port = 80);
    bool is_data_available() const noexcept;
    void pollImage(const std::string& endpoint, std::string& img_buf, std::mutex& img_buf_mtx, std::atomic<bool>& is_new_data_available);
    std::optional<Json::Value> getJson();
    ~SSE();

private:
    std::thread connection_thread;
};
}  // namespace Sse