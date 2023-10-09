#include "fault-injection.h"

#include <cassert>
#include <iostream>
#include <vector>

namespace {

void* injected_allocate(size_t count) {
  if (should_inject_fault()) {
    throw std::bad_alloc();
  }

  void* ptr = malloc(count);
  if (!ptr) {
    throw std::bad_alloc();
  }

  return ptr;
}

void injected_deallocate(void* ptr) {
  free(ptr);
}

template <typename T>
struct fault_injection_allocator {
  using value_type = T;

  fault_injection_allocator() = default;

  T* allocate(size_t count) {
    return static_cast<T*>(injected_allocate(count * sizeof(T)));
  }

  void deallocate(void* ptr, size_t) {
    injected_deallocate(ptr);
  }
};

struct fault_injection_context {
  std::vector<size_t, fault_injection_allocator<size_t>> skip_ranges;
  size_t error_index = 0;
  size_t skip_index = 0;
  bool fault_registered = false;
};

thread_local bool disabled = false;
thread_local fault_injection_context* context = nullptr;

void dump_state() {
#if 0
  fault_injection_disable dg;
  std::cout << "skip_ranges: {";
  if (!context->skip_ranges.empty()) {
    std::cout << context->skip_ranges[0];
    for (size_t i = 1; i != context->skip_ranges.size(); ++i) {
      std::cout << ", " << context->skip_ranges[i];
    }
  }
  std::cout << "}\nerror_index: " << context->error_index << "\nskip_index: " << context->skip_index << '\n'
            << std::flush;
#endif
}

} // namespace

bool should_inject_fault() {
  if (!context) {
    return false;
  }

  if (disabled) {
    return false;
  }

  assert(context->error_index <= context->skip_ranges.size());
  if (context->error_index == context->skip_ranges.size()) {
    fault_injection_disable dg;
    ++context->error_index;
    context->skip_ranges.push_back(0);
    context->fault_registered = true;
    return true;
  }

  assert(context->skip_index <= context->skip_ranges[context->error_index]);

  if (context->skip_index == context->skip_ranges[context->error_index]) {
    ++context->error_index;
    context->skip_index = 0;
    context->fault_registered = true;
    return true;
  }

  ++context->skip_index;
  return false;
}

void fault_injection_point() {
  if (should_inject_fault()) {
    fault_injection_disable dg;
    throw injected_fault("injected fault");
  }
}

void faulty_run(const std::function<void()>& f) {
  assert(!context);
  fault_injection_context ctx;
  context = &ctx;
  for (;;) {
    try {
      f();
    } catch (...) {
      fault_injection_disable dg;
      dump_state();
      ctx.skip_ranges.resize(ctx.error_index);
      ++ctx.skip_ranges.back();
      ctx.error_index = 0;
      ctx.skip_index = 0;
      assert(ctx.fault_registered);
      ctx.fault_registered = false;
      continue;
    }
    assert(!ctx.fault_registered);
    break;
  }
  context = nullptr;
}

fault_injection_disable::fault_injection_disable() : was_disabled(disabled) {
  disabled = true;
}

fault_injection_disable::~fault_injection_disable() {
  disabled = was_disabled;
}

void* operator new(size_t count) {
  return injected_allocate(count);
}

void* operator new[](size_t count) {
  return injected_allocate(count);
}

void operator delete(void* ptr) noexcept {
  injected_deallocate(ptr);
}

void operator delete[](void* ptr) noexcept {
  injected_deallocate(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
  injected_deallocate(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
  injected_deallocate(ptr);
}
