#include "dmn/detail/hook.hpp"

#include <polyhook2/Detour/x64Detour.hpp>
#include <domino/global.h>
#include <domino/osmem.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#include <cstdint>
#include <memory>
#include <string>

#include "dmn/detail/thread_context.hpp"
#include "dmn/error.hpp"

namespace hook = dmn::detail::hook;

namespace {
using mem_alloc_fn = STATUS(LNPUBLIC*)(WORD, DWORD, int, DHANDLE far*);
using mem_free_fn = STATUS(LNPUBLIC*)(DHANDLE);

struct hook_state {
  uint64_t alloc_trampoline = 0;
  uint64_t free_trampoline = 0;

  std::unique_ptr<PLH::x64Detour> alloc_detour;
  std::unique_ptr<PLH::x64Detour> free_detour;
};

auto get_state() -> hook_state& {
  static hook_state state;
  return state;
}

auto get_proc_address(const char* name) -> void* {
#ifdef _WIN32
  auto* module = GetModuleHandleW(L"nnotes.dll");
  if (module == nullptr) {
    throw dmn::hook_error("nnotes.dll is not loaded");
  }

  const auto proc = GetProcAddress(module, name);
  if (proc == nullptr) {
    throw dmn::hook_error("Failed to find Domino function: " + std::string{name});
  }

  return reinterpret_cast<void*>(proc);
#else
  dlerror();

  auto* proc = dlsym(RTLD_DEFAULT, name);

  if (const auto* error = dlerror(); error != nullptr) {
    throw dmn::hook_error("Failed to find Domino function " + std::string{name});
  }

  return proc;
#endif
}

auto original_mem_alloc() -> mem_alloc_fn {
  return std::bit_cast<mem_alloc_fn>(get_state().alloc_trampoline);
}

auto original_mem_free() -> mem_free_fn {
  return std::bit_cast<mem_free_fn>(get_state().free_trampoline);
}

auto LNPUBLIC hooked_mem_alloc(WORD type, DWORD size, int a3, DHANDLE far* handle) -> STATUS {
  const auto result = original_mem_alloc()(type, size, a3, handle);
  if (result == NOERROR && handle != nullptr && *handle != NULLHANDLE) {
    auto& ctx = dmn::detail::thread_context::current();
    auto& state = ctx.handles()[*handle];
    ++state.generation;
    state.alive = true;
  }

  return result;
}

auto LNPUBLIC hooked_mem_free(DHANDLE handle) -> STATUS {
  const auto result = original_mem_free()(handle);
  if (result == NOERROR) {
    auto& ctx = dmn::detail::thread_context::current();
    ctx.handles()[handle].alive = false;
  }

  return result;
}
}  // namespace

void hook::install() {
  auto& state = get_state();
  auto* alloc = get_proc_address("OSMemAllocExtended");
  auto* free = get_proc_address("OSMemFree");

  state.alloc_detour = std::make_unique<PLH::x64Detour>(
    reinterpret_cast<uint64_t>(alloc), reinterpret_cast<uint64_t>(&hooked_mem_alloc),
    &state.alloc_trampoline
  );

  state.free_detour = std::make_unique<PLH::x64Detour>(
    reinterpret_cast<uint64_t>(free), reinterpret_cast<uint64_t>(&hooked_mem_free),
    &state.free_trampoline
  );

  if (!state.alloc_detour->hook()) {
    state.alloc_detour.reset();
    state.free_detour.reset();

    throw dmn::hook_error("Failed to hook OSMemAllocExtended");
  }

  if (!state.free_detour->hook()) {
    state.alloc_detour->unHook();

    state.alloc_detour.reset();
    state.free_detour.reset();

    throw dmn::hook_error("Failed to hook OSMemFree");
  }
}

void hook::uninstall() {
  auto& state = get_state();

  if (state.free_detour != nullptr) {
    state.free_detour->unHook();
    state.free_detour.reset();
  }

  if (state.alloc_detour != nullptr) {
    state.alloc_detour->unHook();
    state.alloc_detour.reset();
  }

  state.alloc_trampoline = 0;
  state.free_trampoline = 0;
}

auto hook::is_valid(detail::dhandle_t handle, std::optional<uint32_t> generation) -> bool {
  auto& ctx = detail::thread_context::current();
  const auto& handles = ctx.handles();
  const auto it = handles.find(handle);
  if (it == handles.end() || !it->second.alive) {
    return false;
  }

  return generation ? it->second.generation == generation : true;
}

auto hook::get_generation(detail::dhandle_t handle) -> uint32_t {
  auto& ctx = detail::thread_context::current();
  return is_valid(handle) ? ctx.handles()[handle].generation : 0;
}