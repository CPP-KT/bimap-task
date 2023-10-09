#pragma once

#include "element.h"
#include "bimap.h"
#include "fault-injection.h"

#include <gtest/gtest.h>

#include <bit>
#include <concepts>
#include <initializer_list>
#include <ostream>

template class bimap<element, element>;
using container = bimap<element, element>;

template <typename F, typename = std::enable_if_t<std::is_invocable_v<F, std::ostream&>>>
decltype(auto) operator<<(std::ostream& out, const F& f) {
  return f(out);
}

template <typename C>
void mass_insert(C& c, std::initializer_list<std::pair<int, int>> elems) {
  for (const auto& e : elems) {
    c.insert(e.first, e.second);
  }
}


template <class Actual, class Expected>
void expect_eq(const Actual& actual, const Expected& expected) {
  fault_injection_disable dg;

  EXPECT_EQ(expected.size(), actual.size());

  EXPECT_TRUE(expected == actual) << [&](std::ostream& out) {
    out << '[';

    auto e_it = expected.begin_left();
    for (;std::next(e_it) != expected.end_left(); ++e_it) {
      out << "{" << *e_it << ", " << *e_it.flip() << "}";
      out << ", ";
    }
    out << "{" << *e_it << ", " << *e_it.flip() << "}";

    out << "] != [";

    auto a_it = actual.begin_left();
    for (;std::next(a_it) != actual.end_left(); ++a_it) {
      out << "{" << *a_it << ", " << *a_it.flip() << "}";
      out << ", ";
    }
    out << "{" << *a_it << ", " << *a_it.flip() << "}";

    out << "]\n";
  };
}

template <class Actual>
void expect_equals(const Actual& actual, std::initializer_list<std::pair<int, int>> elems) {
  fault_injection_disable dg;

  EXPECT_EQ(actual.size(), elems.size());

  auto compare = [&](){
    auto a_it = actual.begin_left();
    auto e_it = elems.begin();
    for (; a_it != actual.end_left(); ++a_it, ++e_it) {
      if (*a_it != (*e_it).first) return false;
      if (*a_it.flip() != (*e_it).second) return false;
    }
    return true;
  };

  EXPECT_TRUE(compare()) << [&](std::ostream& out) {
    out << '[';

    auto e_it = elems.begin();
    for (;std::next(e_it) != elems.end(); ++e_it) {
      out << "{" << (*e_it).first << ", " << (*e_it).second << "}";
      out << ", ";
    }
    out << "{" << (*e_it).first << ", " << (*e_it).second << "}";

    out << "] != [";

    auto a_it = actual.begin_left();
    for (;std::next(a_it) != actual.end_left(); ++a_it) {
      out << "{" << *a_it << ", " << *a_it.flip() << "}";
      out << ", ";
    }
    out << "{" << *a_it << ", " << *a_it.flip() << "}";

    out << "]\n";
  };
}

template <typename C>
void expect_empty(const C& c) {
  EXPECT_TRUE(c.empty());
  EXPECT_EQ(0, c.size());
}

template <class It>
class reverse_view {
public:
  template <class R>
  reverse_view(const R& r) noexcept : reverse_view(r.begin(), r.end(), r.size()) {}

  reverse_view(It begin, It end, size_t size) noexcept : base_begin(begin), base_end(end), base_size(size) {}

  auto begin() const noexcept {
    return std::make_reverse_iterator(base_end);
  }

  auto end() const noexcept {
    return std::make_reverse_iterator(base_begin);
  }

  size_t size() const noexcept {
    return base_size;
  }

private:
  It base_begin;
  It base_end;
  size_t base_size;
};

template <class R>
reverse_view(const R& r) -> reverse_view<decltype(r.begin())>;

template <typename C>
class strong_exception_safety_guard {
public:
  explicit strong_exception_safety_guard(const C& c) noexcept : ref(c), expected((fault_injection_disable{}, c)) {}

  strong_exception_safety_guard(const strong_exception_safety_guard&) = delete;

  ~strong_exception_safety_guard() {
    if (std::uncaught_exceptions() > 0) {
      expect_eq(expected, ref);
    }
  }

private:
  const C& ref;
  C expected;
};

class base_test : public ::testing::Test {
protected:
  element::no_new_instances_guard instances_guard;
};
