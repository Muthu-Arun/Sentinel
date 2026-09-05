#include "curlPoll.h"

#include <curl/multi.h>

#include <thread>

namespace CPoll {
namespace CMulti {

CURLM* async_handle;
std::thread async_thread;
std::mutex async_mtx;
void init() {
    async_handle = curl_multi_init();
    async_thread = std::thread(curl_multi_loop);
}

void cleanup(){
    curl_multi_cleanup(async_handle);
}

void curl_multi_loop() {}
}  // namespace CMulti


}  // namespace CPoll