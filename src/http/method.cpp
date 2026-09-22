#include "dmn/http/method.hpp"

#include <domino/global.h>
#include <domino/dsapi.h>

#include <utility>

using dmn::http::method;

static_assert(std::to_underlying(method::none) == kRequestNone);
static_assert(std::to_underlying(method::head) == kRequestHEAD);
static_assert(std::to_underlying(method::get) == kRequestGET);
static_assert(std::to_underlying(method::post) == kRequestPOST);
static_assert(std::to_underlying(method::put) == kRequestPUT);
static_assert(std::to_underlying(method::del) == kRequestDELETE);
static_assert(std::to_underlying(method::trace) == kRequestTRACE);
static_assert(std::to_underlying(method::connect) == kRequestCONNECT);
static_assert(std::to_underlying(method::options) == kRequestOPTIONS);
static_assert(std::to_underlying(method::unknown) == kRequestUNKNOWN);
static_assert(std::to_underlying(method::bad) == kRequestBAD);