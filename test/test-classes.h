#pragma once

#include <cmath>
#include <functional>
#include <stdexcept>
#include <unordered_set>
#include <utility>

class test_object {
public:
  test_object() = default;

  explicit test_object(int b) : a(b) {}

  test_object(const test_object&) = delete;

  test_object(test_object&& other) noexcept {
    std::swap(a, other.a);
  }

  test_object& operator=(const test_object&) = delete;

  test_object& operator=(test_object&& other) noexcept {
    std::swap(a, other.a);
    return *this;
  }

  friend bool operator<(const test_object& c, const test_object& b) {
    return c.a < b.a;
  }

  friend bool operator==(const test_object& c, const test_object& b) {
    return c.a == b.a;
  }

  int a = 0;
};

class vector_compare {
public:
  using vec = std::pair<int, int>;

  enum distance_type {
    euclidean,
    manhattan
  };

  explicit vector_compare(distance_type p = euclidean) : type(p) {}

  bool operator()(vec a, vec b) const {
    if (type == euclidean) {
      return euc(a) < euc(b);
    } else {
      return man(a) < man(b);
    }
  }

private:
  static double euc(vec x) {
    return sqrt(x.first * x.first + x.second * x.second);
  }

  static double man(vec x) {
    return abs(x.first) + abs(x.second);
  }

  distance_type type;
};

class non_default_constructible {
public:
  non_default_constructible() = delete;

  explicit non_default_constructible(int b) : a(b) {}

  non_default_constructible(const non_default_constructible&) = default;
  non_default_constructible& operator=(const non_default_constructible&) = default;

  friend bool operator<(const non_default_constructible& c, const non_default_constructible& b) {
    return c.a < b.a;
  }

  friend bool operator==(const non_default_constructible& c, const non_default_constructible& b) {
    return c.a == b.a;
  }

private:
  int a;
};

class address_checking_object {
private:
  static std::unordered_set<const address_checking_object*> addresses;

  void add_instance() const;
  void remove_instance() const;
  void assert_exists() const;

  int value;

  static size_t copy_throw_countdown;
  static void process_copying();

public:
  static void expect_no_instances();

  static void set_copy_throw_countdown(size_t new_countdown);

  operator int() const;

  address_checking_object();
  address_checking_object(int value);

  address_checking_object(const address_checking_object& other);
  address_checking_object& operator=(const address_checking_object& other);

  ~address_checking_object();
};

class state_comparator {
public:
  explicit state_comparator(bool flag = false) : is_inverted(flag) {}

  bool operator()(int a, int b) const {
    return is_inverted ? b < a : a < b;
  }

private:
  bool is_inverted;
};

class non_copyable_comparator : public std::less<> {
public:
  non_copyable_comparator() = default;

  non_copyable_comparator(const non_copyable_comparator&) = delete;
  non_copyable_comparator(non_copyable_comparator&&) = default;

  non_copyable_comparator& operator=(const non_copyable_comparator&) = delete;
  non_copyable_comparator& operator=(non_copyable_comparator&&) = default;

  ~non_copyable_comparator() = default;
};

class non_default_constructible_comparator : public std::less<> {
private:
  non_default_constructible_comparator() = default;

public:
  static non_default_constructible_comparator create() noexcept {
    return {};
  }

  non_default_constructible_comparator(const non_default_constructible_comparator&) = default;
  non_default_constructible_comparator(non_default_constructible_comparator&&) = default;

  non_default_constructible_comparator& operator=(const non_default_constructible_comparator&) = default;
  non_default_constructible_comparator& operator=(non_default_constructible_comparator&&) = default;

  ~non_default_constructible_comparator() = default;
};

class no_use_after_move_comparator {
  bool has_moved = false;

public:
  no_use_after_move_comparator() = default;

  no_use_after_move_comparator(const no_use_after_move_comparator&) = default;

  no_use_after_move_comparator(no_use_after_move_comparator&& that) noexcept {
    that.has_moved = true;
  }

  no_use_after_move_comparator& operator=(const no_use_after_move_comparator&) = default;

  no_use_after_move_comparator& operator=(no_use_after_move_comparator&& that) noexcept {
    has_moved = that.has_moved;
    that.has_moved = true;
    return *this;
  }

  template <typename L, typename R>
  bool operator()(L&& left, R&& right) const {
    if (has_moved) {
      throw std::runtime_error("used of moved comparator");
    }
    return std::less<>()(std::forward<L>(left), std::forward<R>(right));
  }

  ~no_use_after_move_comparator() = default;
};

class modified_int_custom_comparator;

class modified_int {
public:
  modified_int(int a) : val(a) {}

  bool operator==(const modified_int&) const {
    throw std::bad_function_call(); // you shouldn't use it while custom
                                    // comparator exists
  }

  bool operator<(const modified_int&) const {
    throw std::bad_function_call(); // you shouldn't use it while custom
                                    // comparator exists
  }

  int val;
};

class modified_int_custom_comparator {
public:
  bool operator()(const modified_int& a, const modified_int& b) const {
    return a.val < b.val;
  }
};

constexpr int complex_cmp_default = 0;

template <int type_id = complex_cmp_default>
class unique_comparator {
  inline static int last_used_id{};

  int id;

public:
  explicit unique_comparator(int id = -1) : id(id) {}

  unique_comparator(unique_comparator&& that) : id(that.id) {
    that.id = -1;
  }

  unique_comparator& operator=(unique_comparator&& that) {
    id = that.id;
    that.id = -1;
    return *this;
  }

  template <typename L, typename R>
  bool operator()(L&& left, R&& right) const {
    last_used_id = id;
    return std::less<>()(std::forward<L>(left), std::forward<R>(right));
  }

  static int get_last_used_id() {
    return last_used_id;
  }

  static void reset_last_used_id() {
    last_used_id = -1;
  }
};
