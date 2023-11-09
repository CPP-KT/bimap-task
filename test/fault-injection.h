#pragma once

#include <functional>
#include <stdexcept>

struct injected_fault : std::runtime_error {
  using runtime_error::runtime_error;
};

bool should_inject_fault();
void fault_injection_point();
void faulty_run(const std::function<void()>& f);

struct fault_injection_disable {
  fault_injection_disable();

  fault_injection_disable(const fault_injection_disable&) = delete;
  fault_injection_disable& operator=(const fault_injection_disable&) = delete;

  ~fault_injection_disable();

private:
  bool was_disabled;
};
