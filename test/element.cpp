#include "element.h"

#include "fault-injection.h"

#include <gtest/gtest.h>

element::element(int data) : data(data) {
  add_instance();
}

element::element(const element& other) : data(other.data) {
  add_instance();
}

element::~element() {
  delete_instance();
}

element& element::operator=(const element& c) {
  assert_exists();
  fault_injection_point();

  data = c.data;
  return *this;
}

element::operator int() const {
  assert_exists();
  fault_injection_point();

  return data;
}

bool operator==(const element& a, const element& b) {
  a.assert_exists();
  b.assert_exists();
  fault_injection_point();
  return a.data == b.data;
}

bool operator!=(const element& a, const element& b) {
  a.assert_exists();
  b.assert_exists();
  fault_injection_point();
  return a.data != b.data;
}

bool operator<(const element& a, const element& b) {
  a.assert_exists();
  b.assert_exists();
  fault_injection_point();
  return a.data < b.data;
}

bool operator<=(const element& a, const element& b) {
  a.assert_exists();
  b.assert_exists();
  fault_injection_point();
  return a.data <= b.data;
}

bool operator>(const element& a, const element& b) {
  a.assert_exists();
  b.assert_exists();
  fault_injection_point();
  return a.data > b.data;
}

bool operator>=(const element& a, const element& b) {
  a.assert_exists();
  b.assert_exists();
  fault_injection_point();
  return a.data >= b.data;
}

bool operator==(const element& a, int b) {
  a.assert_exists();
  fault_injection_point();
  return a.data == b;
}

bool operator!=(const element& a, int b) {
  a.assert_exists();
  fault_injection_point();
  return a.data != b;
}

bool operator<(const element& a, int b) {
  a.assert_exists();
  fault_injection_point();
  return a.data < b;
}

bool operator<=(const element& a, int b) {
  a.assert_exists();
  fault_injection_point();
  return a.data <= b;
}

bool operator>(const element& a, int b) {
  a.assert_exists();
  fault_injection_point();
  return a.data > b;
}

bool operator>=(const element& a, int b) {
  a.assert_exists();
  fault_injection_point();
  return a.data >= b;
}

bool operator==(int a, const element& b) {
  b.assert_exists();
  fault_injection_point();
  return a == b.data;
}

bool operator!=(int a, const element& b) {
  b.assert_exists();
  fault_injection_point();
  return a != b.data;
}

bool operator<(int a, const element& b) {
  b.assert_exists();
  fault_injection_point();
  return a < b.data;
}

bool operator<=(int a, const element& b) {
  b.assert_exists();
  fault_injection_point();
  return a <= b.data;
}

bool operator>(int a, const element& b) {
  b.assert_exists();
  fault_injection_point();
  return a > b.data;
}

bool operator>=(int a, const element& b) {
  b.assert_exists();
  fault_injection_point();
  return a >= b.data;
}

void element::add_instance() {
  fault_injection_point();
  fault_injection_disable dg;
  auto p = instances.insert(this);
  if (!p.second) {
    std::stringstream ss;
    ss << "A new object is created at the address " << static_cast<void*>(this)
       << " while the previous object at this address was not destroyed";
    throw std::logic_error(ss.str());
  }
}

void element::delete_instance() {
  fault_injection_disable dg;
  size_t erased = instances.erase(this);
  if (erased != 1) {
    std::stringstream ss;
    ss << "Attempt of destroying non-existing object at address " << static_cast<void*>(this);
    throw std::logic_error(ss.str());
  }
}

void element::assert_exists() const {
  fault_injection_disable dg;
  bool exists = instances.find(this) != instances.end();
  if (!exists) {
    std::stringstream ss;
    ss << "Accessing a non-existing object at address " << static_cast<const void*>(this);
    throw std::logic_error(ss.str());
  }
}

std::set<const element*> element::instances;

element::no_new_instances_guard::no_new_instances_guard() : old_instances(instances) {}

element::no_new_instances_guard::~no_new_instances_guard() {
  EXPECT_EQ(old_instances, instances);
}

void element::no_new_instances_guard::expect_no_instances() {
  EXPECT_EQ(old_instances, instances);
}
