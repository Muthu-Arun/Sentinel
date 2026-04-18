// Handles server send events and parsing, try not to modify the http polling
#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <json/value.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string_view>
namespace Sse {
Json::Value extractPayload(std::string_view buffer);

class SSE {
protected:
    std::queue<std::string> responses;
    std::unique_ptr<std::mutex> responses_mtx;
    std::atomic<bool> is_new_data_available = 0;
    std::string remote_url, endpoint;
    uint64_t port;

public:
    SSE(std::string_view remote_url_, std::string_view endpoint_, int port = 80);
    bool is_data_available() const noexcept;

private:
    size_t sse_curl_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
};
}  // namespace Sse