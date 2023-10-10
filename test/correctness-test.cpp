#include "bimap.h"
#include "test-classes.h"

#include <gtest/gtest.h>

#include <random>

template class bimap<int, non_default_constructible>;
template class bimap<non_default_constructible, int>;

TEST(bimap, simple) {
  bimap<int, int> b;
  b.insert(4, 4);
  EXPECT_EQ(b.at_right(4), b.at_left(4));
}

TEST(bimap, leak_check) {
  static constexpr size_t N = 10'000;

  bimap<unsigned long, unsigned long> b;

  std::mt19937 rng(std::mt19937::default_seed);
  std::uniform_int_distribution<unsigned long> dist(0, N);

  for (size_t i = 0; i < N; i++) {
    unsigned long left = dist(rng);
    unsigned long right = dist(rng);
    b.insert(left, right);
  }
}

TEST(bimap, custom_comparator) {
  bimap<int, int, std::greater<>> b;
  b.insert(3, 4);
  b.insert(1, 5);
  b.insert(10, -10);

  int prev = *b.begin_left();
  for (auto it = ++b.begin_left(); it != b.end_left(); it++) {
    EXPECT_GT(prev, *it);
  }
  prev = *b.begin_right();
  for (auto it = ++b.begin_right(); it != b.end_right(); it++) {
    EXPECT_LT(prev, *it);
  }
}

TEST(bimap, custom_parametrized_comparator) {
  using vec = std::pair<int, int>;
  bimap<vec, vec, vector_compare, vector_compare> b((vector_compare(vector_compare::manhattan)));
  b.insert({0, 1}, {35, 3});
  b.insert({20, -20}, {20, -20});
  b.insert({35, 3}, {3, -1});
  b.insert({3, -1}, {0, 1});

  std::vector<vec> correct_left = {
      { 0,   1},
      { 3,  -1},
      {35,   3},
      {20, -20}
  };
  std::vector<vec> correct_right = {
      { 0,   1},
      { 3,  -1},
      {20, -20},
      {35,   3}
  };

  auto lit = b.begin_left();
  auto rit = b.begin_right();
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*lit++, correct_left[i]);
    EXPECT_EQ(*rit++, correct_right[i]);
  }
}

TEST(bimap, state_comparator) {
  bimap<int, int, state_comparator, state_comparator> a(state_comparator(true));
  a.insert(1, 2);
  a.insert(3, 4);
  a.insert(5, 6);
  EXPECT_EQ(*a.begin_left(), 5);
  EXPECT_EQ(*a.begin_right(), 2);

  auto a_copy = a;
  EXPECT_EQ(a, a_copy);

  bimap<int, int, state_comparator, state_comparator> b(state_comparator(false), state_comparator(true));
  b.insert(11, 12);
  b.insert(13, 14);
  b.insert(15, 16);
  EXPECT_EQ(*b.begin_left(), 11);
  EXPECT_EQ(*b.begin_right(), 16);

  using std::swap;
  swap(a, b);
  EXPECT_EQ(*a.begin_left(), 11);
  EXPECT_EQ(*a.begin_right(), 16);
  EXPECT_EQ(*b.begin_left(), 5);
  EXPECT_EQ(*b.begin_right(), 2);
}

TEST(bimap, non_copyable_comparator) {
  bimap<int, int, non_copyable_comparator, non_copyable_comparator> a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  bimap<int, int, non_copyable_comparator, non_copyable_comparator> b = std::move(a);
  EXPECT_EQ(b.size(), 4);
  EXPECT_NE(a.size(), 4);

  a = std::move(b);
  EXPECT_EQ(a.size(), 4);
  EXPECT_NE(b.size(), 4);
}

TEST(bimap, non_default_constructible_comparator) {
  using cmp = non_default_constructible_comparator;

  bimap<int, int, cmp, cmp> a(cmp::create(), cmp::create());
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  bimap<int, int, cmp, cmp> b = a;
  EXPECT_EQ(a, b);

  bimap<int, int, cmp, cmp> c = std::move(a);
  EXPECT_EQ(b, c);
}

TEST(bimap, copies) {
  bimap<int, int> b;
  b.insert(3, 4);

  bimap<int, int> b1 = b;
  EXPECT_EQ(*b.find_left(3).flip(), 4);

  b1.insert(4, 5);
  EXPECT_EQ(b.find_left(4), b.end_left());

  b1.insert(10, -10);
  b = b1;
  EXPECT_NE(b.find_right(-10), b.end_right());
}

TEST(bimap, throwing_in_copy_constructor) {
  {
    bimap<address_checking_object, int> a;
    a.insert(1, 2);
    a.insert(3, 4);
    a.insert(5, 6);
    a.insert(7, 8);
    a.insert(9, 10);

    address_checking_object::set_copy_throw_countdown(3);
    try {
      bimap<address_checking_object, int> b = a;
    } catch (const std::runtime_error& error) {}
  }
  address_checking_object::expect_no_instances();
}

TEST(bimap, throwing_in_copy_assignment) {
  {
    bimap<address_checking_object, int> a;
    a.insert(1, 2);
    a.insert(3, 4);
    a.insert(5, 6);
    a.insert(7, 8);
    a.insert(9, 10);

    address_checking_object::set_copy_throw_countdown(3);
    bimap<address_checking_object, int> b;
    try {
      b = a;
    } catch (const std::runtime_error& error) {}
    EXPECT_EQ(b.size(), 0); // Checking strong guarantee.
  }
  address_checking_object::expect_no_instances();
}

TEST(bimap, insert) {
  bimap<int, int> b;
  b.insert(4, 10);
  b.insert(10, 4);
  EXPECT_EQ(*b.find_right(4).flip(), 10);
  EXPECT_EQ(b.at_left(10), 4);
}

TEST(bimap, insert_move) {
  bimap<int, test_object> b;
  test_object x(3), x2(3);
  b.insert(4, std::move(x));
  EXPECT_EQ(x.a, 0);
  EXPECT_EQ(b.at_right(x2), 4);
  EXPECT_EQ(b.at_left(4), x2);

  bimap<test_object, int> b2;
  test_object y(4), y2(4);
  b2.insert(std::move(y), 3);
  EXPECT_EQ(y.a, 0);
  EXPECT_EQ(b2.at_left(y2), 3);
  EXPECT_EQ(b2.at_right(3), y2);

  bimap<test_object, test_object> b3;
  test_object z(6), z1(2);
  b3.insert(std::move(z), std::move(z1));
  EXPECT_EQ(z.a, 0);
  EXPECT_EQ(z1.a, 0);
  auto it = b3.find_left(test_object(6));
  EXPECT_EQ(it->a, 6);
  EXPECT_EQ(it.flip()->a, 2);
}

TEST(bimap, at) {
  bimap<int, int> b;
  b.insert(4, 3);

  EXPECT_THROW(b.at_left(1), std::out_of_range);
  EXPECT_THROW(b.at_right(300), std::out_of_range);
  EXPECT_EQ(b.at_left(4), 3);
  EXPECT_EQ(b.at_right(3), 4);
}

TEST(bimap, at_or_default) {
  bimap<int, int> b;
  b.insert(4, 2);

  EXPECT_EQ(b.at_left_or_default(4), 2);
  EXPECT_EQ(b.at_right_or_default(2), 4);

  EXPECT_EQ(b.at_left_or_default(5), 0);
  EXPECT_EQ(b.at_right(0), 5);

  EXPECT_EQ(b.at_right_or_default(1), 0);
  EXPECT_EQ(b.at_left(0), 1);

  // b has (5, 0)
  EXPECT_EQ(b.at_left_or_default(42), 0);
  // (5, 0) is replaced with (42, 0)
  EXPECT_EQ(b.at_right(0), 42);

  // b has (0, 1)
  EXPECT_EQ(b.at_right_or_default(1000), 0);
  // (0, 1) is replaced with (0, 1000)
  EXPECT_EQ(b.at_left(0), 1000);
}

TEST(bimap, end_flip) {
  bimap<int, int> b;
  EXPECT_EQ(b.end_left().flip(), b.end_right());
  EXPECT_EQ(b.end_right().flip(), b.end_left());

  b.insert(1, 2);
  b.insert(-3, 5);
  b.insert(1000, -100000);

  EXPECT_EQ(b.end_left().flip(), b.end_right());
  EXPECT_EQ(b.end_right().flip(), b.end_left());
}

TEST(bimap, total_flip) {
  bimap<int, int> b;
  b.insert(100, -100);
  b.insert(-100, 100);
  b.insert(-10, 10);
  b.insert(-12, -10);

  auto rit = b.begin_right();
  auto lit = b.begin_left();
  for (; rit != b.end_right() && lit != b.end_left(); rit++, lit++) {
    EXPECT_EQ(lit.flip().flip(), lit);
    EXPECT_EQ(rit.flip().flip(), rit);
  }
}

TEST(bimap, find) {
  bimap<int, int> b;
  b.insert(3, 4);
  b.insert(4, 5);
  b.insert(42, 1000);

  EXPECT_EQ(*b.find_right(5).flip(), 4);
  EXPECT_EQ(*b.find_left(3).flip(), 4);
  EXPECT_EQ(b.find_left(3436), b.end_left());
  EXPECT_EQ(b.find_right(-1000), b.end_right());
}

TEST(bimap, empty) {
  bimap<int, int> b;
  EXPECT_TRUE(b.empty());

  auto it = b.insert(1, 1);
  EXPECT_FALSE(b.empty());
  b.erase_left(it);
  EXPECT_TRUE(b.empty());
}

TEST(bimap, insert_exists) {
  bimap<int, int> b;
  b.insert(1, 2);
  b.insert(2, 3);
  b.insert(3, 4);
  EXPECT_EQ(b.size(), 3);

  auto it = b.insert(2, -1);
  EXPECT_EQ(it, b.end_left());
  EXPECT_EQ(b.at_left(2), 3);
  EXPECT_EQ(b.size(), 3);
}

TEST(bimap, erase_iterator) {
  bimap<int, int> b;

  auto it = b.insert(1, 2);
  b.insert(5, 10);
  b.insert(100, 200);

  auto it1 = b.erase_left(it);
  EXPECT_EQ(b.size(), 2);
  EXPECT_EQ(*it1, 5);

  it = b.insert(-1, -2);
  auto itr = b.erase_right(it.flip());
  EXPECT_EQ(b.size(), 2);
  EXPECT_EQ(*itr, 10);
}

TEST(bimap, erase_value) {
  bimap<int, int> b;

  b.insert(111, 222);
  b.insert(333, 444);
  EXPECT_TRUE(b.erase_left(111));
  EXPECT_EQ(b.size(), 1);
  EXPECT_FALSE(b.erase_right(333333));
  EXPECT_EQ(b.size(), 1);
  EXPECT_TRUE(b.erase_right(444));
  EXPECT_TRUE(b.empty());
}

TEST(bimap, erase_range) {
  bimap<int, int> b;

  b.insert(1, 2);
  auto f = b.insert(2, 3);
  b.insert(3, 4);
  auto l = b.insert(4, 5);
  b.insert(5, 6);

  auto it = b.erase_left(f, l);
  EXPECT_EQ(*it, 4);
  EXPECT_EQ(b.size(), 3);

  auto f1 = b.insert(100, 4).flip();
  auto l1 = b.insert(200, 10).flip();

  auto it1 = b.erase_right(f1, l1);
  EXPECT_EQ(*it1, 10);
  EXPECT_EQ(b.size(), 2);

  b.erase_left(b.begin_left(), b.end_left());
  EXPECT_TRUE(b.empty());
}

TEST(bimap, lower_bound) {
  std::vector<std::pair<int, int>> data = {
      { 1,  2},
      { 2,  3},
      { 3,  4},
      { 8, 16},
      {32, 66}
  };

  std::sort(data.begin(), data.end());

  do {
    bimap<int, int> b;

    for (const auto& p : data) {
      b.insert(p.first, p.second);
    }

    EXPECT_EQ(*b.lower_bound_left(5), 8);
    EXPECT_EQ(*b.lower_bound_right(4), 4);
    EXPECT_EQ(*b.lower_bound_left(4).flip(), 16);
    EXPECT_EQ(b.lower_bound_right(100), b.end_right());
    EXPECT_EQ(b.lower_bound_left(100), b.end_left());

  } while (std::next_permutation(data.begin(), data.end()));
}

TEST(bimap, upper_bound) {
  std::vector<std::pair<int, int>> data = {
      { 1,  2},
      { 2,  3},
      { 3,  4},
      { 8, 16},
      {32, 66}
  };

  std::sort(data.begin(), data.end());

  do {
    bimap<int, int> b;

    for (const auto& p : data) {
      b.insert(p.first, p.second);
    }

    EXPECT_EQ(*b.upper_bound_left(5), 8);
    EXPECT_EQ(*b.upper_bound_right(-100), 2);
    EXPECT_EQ(b.upper_bound_right(100), b.end_right());
    EXPECT_EQ(b.upper_bound_left(400), b.end_left());

  } while (std::next_permutation(data.begin(), data.end()));
}

TEST(bimap, copy_ctor) {
  bimap<int, int> a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  auto b = a;
  EXPECT_EQ(a.size(), b.size());
  EXPECT_EQ(a, b);
}

TEST(bimap, copy_assignment) {
  bimap<int, int> a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  bimap<int, int> b;
  b.insert(2, 5);
  b.insert(5, 2);

  b = a;
  EXPECT_EQ(a.size(), b.size());
  EXPECT_EQ(a, b);
}

TEST(bimap, copy_assignment_self) {
  bimap<int, int> a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  bimap<int, int> a_copy = a;
  a = a;
  EXPECT_EQ(a.size(), a_copy.size());
  EXPECT_EQ(a, a_copy);
}

TEST(bimap, move_ctor) {
  bimap<address_checking_object, int> a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  {
    bimap<address_checking_object, int> a_copy = a;
    address_checking_object::set_copy_throw_countdown(1);

    bimap<address_checking_object, int> b = std::move(a);
    EXPECT_EQ(b.size(), a_copy.size());
    EXPECT_EQ(b, a_copy);
  }

  address_checking_object::expect_no_instances();
  address_checking_object::set_copy_throw_countdown(0);
}

TEST(bimap, move_assignment) {
  bimap<address_checking_object, int> a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  bimap<address_checking_object, int> b;
  b.insert(2, 5);
  b.insert(5, 2);

  bimap<address_checking_object, int> a_copy = a;
  address_checking_object::set_copy_throw_countdown(1);

  b = std::move(a);
  EXPECT_EQ(b.size(), a_copy.size());
  EXPECT_EQ(b, a_copy);

  address_checking_object::set_copy_throw_countdown(0);
}

TEST(bimap, move_assignment_self) {
  bimap<int, int> a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  bimap<int, int> a_copy = a;
  a = std::move(a);
  EXPECT_EQ(a.size(), a_copy.size());
  EXPECT_EQ(a, a_copy);
}

TEST(bimap, equivalence) {
  bimap<int, int> a;
  bimap<int, int> b;
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);

  a.insert(1, 2);
  a.insert(3, 4);
  b.insert(1, 2);
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);

  b.erase_left(1);
  b.insert(1, 4);
  b.insert(3, 2);
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);

  EXPECT_EQ(a.end_left().flip(), a.end_right());
  EXPECT_EQ(a.end_right().flip(), a.end_left());

  a.erase_left(1);
  a.erase_right(4);
  a.insert(3, 2);
  a.insert(1, 4);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(bimap, equivalence_with_custom_comparator) {
  bimap<modified_int, modified_int, modified_int_custom_comparator, modified_int_custom_comparator> a;
  bimap<modified_int, modified_int, modified_int_custom_comparator, modified_int_custom_comparator> b;
  a.insert(1, 2);
  a.insert(3, 4);
  b.insert(1, 2);
  b.insert(3, 4);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(bimap, iterator_traits) {
  using bm = bimap<int, double>;
  static_assert(std::bidirectional_iterator<bm::left_iterator>);
  static_assert(std::bidirectional_iterator<bm::right_iterator>);
  EXPECT_TRUE((std::is_same_v<std::iterator_traits<bm::left_iterator>::value_type, int>));
  EXPECT_TRUE((std::is_same_v<std::iterator_traits<bm::right_iterator>::value_type, double>));
  EXPECT_TRUE((std::is_same_v<std::iterator_traits<bm::left_iterator>::reference, const int&>));
  EXPECT_TRUE((std::is_same_v<std::iterator_traits<bm::right_iterator>::reference, const double&>));
  EXPECT_TRUE((std::is_same_v<std::iterator_traits<bm::left_iterator>::pointer, const int*>));
  EXPECT_TRUE((std::is_same_v<std::iterator_traits<bm::right_iterator>::pointer, const double*>));
}

TEST(bimap, iterator_ops) {
  bimap<int, int> b;
  b.insert(3, 4);
  b.insert(100, 10);
  auto it = b.insert(-10, 100);

  auto it_next = it;
  EXPECT_EQ(it_next++, it);

  EXPECT_EQ(++it, it_next--);
  EXPECT_EQ(--it, it_next);
}

TEST(bimap, iterating) {
  bimap<int, int> b;
  b.insert(1, 0);
  b.insert(2, 10);
  b.insert(3, 100);

  std::vector<int> left_values;
  for (auto it = b.begin_left(); it != b.end_left(); ++it) {
    left_values.push_back(*it);
  }
  std::vector<int> left_values_inv;
  for (auto it = b.end_left(); it != b.begin_left();) {
    --it;
    left_values_inv.push_back(*it);
  }
  std::reverse(left_values_inv.begin(), left_values_inv.end());
  EXPECT_EQ(left_values, left_values_inv);

  std::vector<int> right_values;
  for (auto it = b.begin_right(); it != b.end_right(); ++it) {
    right_values.push_back(*it);
  }
  std::vector<int> right_values_inv;
  for (auto it = b.end_right(); it != b.begin_right();) {
    --it;
    right_values_inv.push_back(*it);
  }
  std::reverse(right_values_inv.begin(), right_values_inv.end());
  EXPECT_EQ(right_values, right_values_inv);
}

TEST(bimap, swap) {
  bimap<int, int> b, b1;
  b.insert(3, 4);
  b1.insert(4, 3);
  EXPECT_EQ(*b.find_left(3), 3);
  EXPECT_EQ(*b1.find_right(3), 3);

  using std::swap;
  swap(b, b1);
  EXPECT_EQ(*b1.find_left(3), 3);
  EXPECT_EQ(*b.find_right(3), 3);
}
