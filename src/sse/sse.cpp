#include "sse.h"
#include <json/version.h>

namespace Sse {

    Json::Value extractPayload(std::string_view buffer){
        size_t startPos = buffer.find("data:");
        size_t endPos = buffer.find("\n\n");
        return Json::Value(std::string(buffer.substr(startPos, endPos)));
    }
}