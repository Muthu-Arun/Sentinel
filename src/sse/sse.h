// Handles server send events and parsing, try not to modify the http polling
#include <json/value.h>
namespace Sse {
    Json::Value extractPayload(std::string_view buffer);
}