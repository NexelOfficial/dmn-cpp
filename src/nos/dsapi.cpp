#include "dmn/nos/dsapi.hpp"

#include <domino/global.h>
#include <domino/dsapi.h>

using dmn::dsapi::method;

static_assert(static_cast<uint8_t>(method::none) == kRequestNone);
static_assert(static_cast<uint8_t>(method::head) == kRequestHEAD);
static_assert(static_cast<uint8_t>(method::get) == kRequestGET);
static_assert(static_cast<uint8_t>(method::post) == kRequestPOST);
static_assert(static_cast<uint8_t>(method::put) == kRequestPUT);
static_assert(static_cast<uint8_t>(method::del) == kRequestDELETE);
static_assert(static_cast<uint8_t>(method::trace) == kRequestTRACE);
static_assert(static_cast<uint8_t>(method::connect) == kRequestCONNECT);
static_assert(static_cast<uint8_t>(method::options) == kRequestOPTIONS);
static_assert(static_cast<uint8_t>(method::unknown) == kRequestUNKNOWN);
static_assert(static_cast<uint8_t>(method::bad) == kRequestBAD);