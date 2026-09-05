#pragma once

#include <curl/multi.h>

#include <mutex>
#include <thread>
namespace CPoll {
namespace CMulti {
void curl_multi_loop();
extern CURLM* async_handle;
extern std::thread async_thread;
extern std::mutex async_mtx;

}  // namespace CMulti

}  // namespace CPoll