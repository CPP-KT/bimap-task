#pragma once

#include <ostream>
#include <set>

struct element {
  struct no_new_instances_guard;

  element() = delete;
  element(int data);
  element(const element& other);
  ~element();

  element& operator=(const element& c);
  operator int() const;

  friend bool operator==(const element& a, const element& b);
  friend bool operator!=(const element& a, const element& b);
  friend bool operator<(const element& a, const element& b);
  friend bool operator<=(const element& a, const element& b);
  friend bool operator>(const element& a, const element& b);
  friend bool operator>=(const element& a, const element& b);

  friend bool operator==(const element& a, int b);
  friend bool operator!=(const element& a, int b);
  friend bool operator<(const element& a, int b);
  friend bool operator<=(const element& a, int b);
  friend bool operator>(const element& a, int b);
  friend bool operator>=(const element& a, int b);

  friend bool operator==(int a, const element& b);
  friend bool operator!=(int a, const element& b);
  friend bool operator<(int a, const element& b);
  friend bool operator<=(int a, const element& b);
  friend bool operator>(int a, const element& b);
  friend bool operator>=(int a, const element& b);

private:
  void add_instance();
  void delete_instance();
  void assert_exists() const;

private:
  int data;

  static std::set<const element*> instances;
};

struct element::no_new_instances_guard {
  no_new_instances_guard();

  no_new_instances_guard(const no_new_instances_guard&) = delete;
  no_new_instances_guard& operator=(const no_new_instances_guard&) = delete;

  ~no_new_instances_guard();

  void expect_no_instances();

private:
  std::set<const element*> old_instances;
};
