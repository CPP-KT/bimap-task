#include "bimap.h"
#include "test-classes.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <random>

template class bimap<int, non_default_constructible>;
template class bimap<non_default_constructible, int>;

TEST_CASE("Simple") {
  bimap<int, int> b;
  b.insert(4, 4);
  CHECK(b.at_left(4) == 4);
  CHECK(b.at_right(4) == 4);
}

TEST_CASE("Leaks") {
  static constexpr size_t N = 10'000;

  bimap<unsigned long, unsigned long> b;

  std::mt19937 rng(std::mt19937::default_seed);
  std::uniform_int_distribution<unsigned long> dist(0, N);

  for (size_t i = 0; i < N; i++) {
    unsigned long left = dist(rng);
    unsigned long right = dist(rng);
    b.insert(left, right);
  }

  SUCCEED();
}

TEST_CASE("Custom comparator") {
  bimap<int, int, std::greater<>> b;
  b.insert(3, 4);
  b.insert(1, 5);
  b.insert(10, -10);

  int prev = *b.begin_left();
  for (auto it = std::next(b.begin_left()); it != b.end_left(); it++) {
    REQUIRE(prev > *it);
    prev = *it;
  }
  prev = *b.begin_right();
  for (auto it = std::next(b.begin_right()); it != b.end_right(); it++) {
    REQUIRE(prev < *it);
    prev = *it;
  }
}

TEST_CASE("Custom parameterized comparator") {
  using vec = std::pair<int, int>;
  bimap<vec, vec, vector_compare, vector_compare> b(vector_compare{vector_compare::manhattan});
  b.insert({0, 1}, {35, 3});
  b.insert({20, -20}, {20, -20});
  b.insert({35, 3}, {3, -1});
  b.insert({3, -1}, {0, 1});

  std::vector<vec> correct_left = {{0, 1}, {3, -1}, {35, 3}, {20, -20}};
  std::vector<vec> correct_right = {{0, 1}, {3, -1}, {20, -20}, {35, 3}};

  auto lit = b.begin_left();
  auto rit = b.begin_right();
  for (int i = 0; i < 4; i++) {
    REQUIRE(*lit++ == correct_left[i]);
    REQUIRE(*rit++ == correct_right[i]);
  }
}

TEST_CASE("Comparator with state") {
  bimap<int, int, state_comparator, state_comparator> a(state_comparator(true));
  a.insert(1, 2);
  a.insert(3, 4);
  a.insert(5, 6);
  CHECK(*a.begin_left() == 5);
  CHECK(*a.begin_right() == 2);

  bimap a_copy = a;
  CHECK(a == a_copy);

  bimap<int, int, state_comparator, state_comparator> b(state_comparator(false), state_comparator(true));
  b.insert(11, 12);
  b.insert(13, 14);
  b.insert(15, 16);
  CHECK(*b.begin_left() == 11);
  CHECK(*b.begin_right() == 16);

  using std::swap;
  swap(a, b);
  CHECK(*a.begin_left() == 11);
  CHECK(*a.begin_right() == 16);
  CHECK(*b.begin_left() == 5);
  CHECK(*b.begin_right() == 2);
}

TEST_CASE("Non-copyable comparator") {
  bimap<int, int, non_copyable_comparator, non_copyable_comparator> a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  bimap<int, int, non_copyable_comparator, non_copyable_comparator> b = std::move(a);
  CHECK(b.size() == 4);
  CHECK(a.size() == 0);

  a = std::move(b);
  CHECK(a.size() == 4);
  CHECK(b.size() == 0);
}

TEST_CASE("Non-default-constructible comparator") {
  using cmp = non_default_constructible_comparator;

  bimap<int, int, cmp, cmp> a(cmp::create(), cmp::create());
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  bimap<int, int, cmp, cmp> b = a;
  CHECK(a == b);

  bimap<int, int, cmp, cmp> c = std::move(a);
  CHECK(b == c);
}

TEST_CASE("Copying") {
  bimap<int, int> b;
  b.insert(3, 4);

  bimap<int, int> b1 = b;
  CHECK(*b.find_left(3).flip() == 4);

  b1.insert(4, 5);
  CHECK(b.find_left(4) == b.end_left());

  b1.insert(10, -10);
  b = b1;
  CHECK(b.find_right(-10) != b.end_right());
}

TEST_CASE("Insert") {
  bimap<int, int> b;
  auto it1 = b.insert(4, 10);
  auto it2 = b.insert(10, 4);
  CHECK(b.find_left(4) == it1);
  CHECK(b.find_right(4).flip() == it2);
  CHECK(*b.find_right(4).flip() == 10);
  CHECK(b.at_left(10) == 4);
}

TEST_CASE("Insert existing") {
  bimap<int, int> b;
  b.insert(1, 2);
  b.insert(2, 3);
  b.insert(3, 4);
  CHECK(b.size() == 3);

  SECTION("Existing left") {
    auto it = b.insert(2, -1);
    CHECK(it == b.end_left());
    CHECK(b.at_left(2) == 3);
    CHECK(b.size() == 3);
  }

  SECTION("Existing right") {
    auto it = b.insert(-1, 2);
    CHECK(it == b.end_left());
    CHECK(b.at_right(2) == 1);
    CHECK(b.size() == 3);
  }
}

TEST_CASE("Insert xvalue") {
  SECTION("Move right") {
    bimap<int, test_object> b;
    test_object x1(3), x2(3);
    b.insert(4, std::move(x1));
    CHECK(x1.a == 0);
    CHECK(b.at_right(x2) == 4);
    CHECK(b.at_left(4) == x2);
  }
  SECTION("Move left") {
    bimap<test_object, int> b;
    test_object x1(4), x2(4);
    b.insert(std::move(x1), 3);
    CHECK(x1.a == 0);
    CHECK(b.at_left(x2) == 3);
    CHECK(b.at_right(3) == x2);
  }
  SECTION("Move both") {
    bimap<test_object, test_object> b;
    test_object x1(6), x2(2);
    b.insert(std::move(x1), std::move(x2));
    CHECK(x1.a == 0);
    CHECK(x2.a == 0);
    auto it = b.find_left(test_object(6));
    CHECK(it->a == 6);
    CHECK(it.flip()->a == 2);
  }
  SECTION("Already exists") {
    bimap<test_object, test_object> b;
    b.insert(test_object(5), test_object(2));

    test_object x1(6), x2(2);
    b.insert(std::move(x1), std::move(x2));

    CHECK(x1.a == 6);
    CHECK(x2.a == 2);
    CHECK(b.size() == 1);

    auto it = b.find_right(test_object(2));
    CHECK(it.flip()->a == 5);
  }
}

TEST_CASE("At") {
  bimap<int, int> b;
  b.insert(4, 3);

  CHECK_THROWS_AS(b.at_left(1), std::out_of_range);
  CHECK_THROWS_AS(b.at_right(300), std::out_of_range);
  CHECK(b.at_left(4) == 3);
  CHECK(b.at_right(3) == 4);
}

TEST_CASE("At-or-default") {
  bimap<int, int> b;
  b.insert(4, 2);

  CHECK(b.at_left_or_default(4) == 2);
  CHECK(b.at_right_or_default(2) == 4);

  CHECK(b.at_left_or_default(5) == 0);
  CHECK(b.at_right(0) == 5);

  CHECK(b.at_right_or_default(1) == 0);
  CHECK(b.at_left(0) == 1);

  CHECK(b.at_left_or_default(42) == 0); // (5, 0) is replaced with (42, 0)
  CHECK(b.at_right(0) == 42);
  CHECK(b.at_left(42) == 0);

  CHECK(b.at_left_or_default(-42) == 0); // (42, 0) is replaced with (-42, 0)
  CHECK(b.at_right(0) == -42);
  CHECK(b.at_left(-42) == 0);

  CHECK(b.at_right_or_default(1000) == 0); // (0, 1) is replaced with (0, 1000)
  CHECK(b.at_left(0) == 1000);
  CHECK(b.at_right(1000) == 0);

  CHECK(b.at_right_or_default(-1000) == 0); // (0, 1000) is replaced with (0, -1000)
  CHECK(b.at_left(0) == -1000);
  CHECK(b.at_right(-1000) == 0);
}

TEST_CASE("At-or-default does not invoke copy assignment") {
  bimap<non_copy_assignable, non_copy_assignable> b;
  b.insert(non_copy_assignable(4), non_copy_assignable(2));

  CHECK(b.at_left_or_default(non_copy_assignable(4)) == non_copy_assignable(2));
  CHECK(b.at_right_or_default(non_copy_assignable(2)) == non_copy_assignable(4));

  CHECK(b.at_left_or_default(non_copy_assignable(5)) == non_copy_assignable(0));
  CHECK(b.at_right_or_default(non_copy_assignable(1)) == non_copy_assignable(0));
}

TEST_CASE("Flip end iterator") {
  bimap<int, int> b;
  CHECK(b.end_left().flip() == b.end_right());
  CHECK(b.end_right().flip() == b.end_left());

  b.insert(1, 2);
  b.insert(-3, 5);
  b.insert(1000, -100000);

  CHECK(b.end_left().flip() == b.end_right());
  CHECK(b.end_right().flip() == b.end_left());
}

TEST_CASE("Flip end iterator with custom comparator") {
  bimap<int, int, state_comparator, state_comparator> b;
  CHECK(b.end_left().flip() == b.end_right());
  CHECK(b.end_right().flip() == b.end_left());

  b.insert(1, 2);
  b.insert(-3, 5);
  b.insert(1000, -100000);

  CHECK(b.end_left().flip() == b.end_right());
  CHECK(b.end_right().flip() == b.end_left());
}

TEST_CASE("Double flip") {
  bimap<int, int> b;
  b.insert(100, -100);
  b.insert(-100, 100);
  b.insert(-10, 10);
  b.insert(-12, -10);

  auto lit = b.begin_left();
  auto rit = b.begin_right();
  while (lit != b.end_left() && rit != b.end_right()) {
    CHECK(lit.flip().flip() == lit);
    CHECK(rit.flip().flip() == rit);
    ++lit;
    ++rit;
  }
}

TEST_CASE("Find") {
  bimap<int, int> b;
  b.insert(3, 4);
  b.insert(4, 5);
  b.insert(42, 1000);

  CHECK(*b.find_left(3).flip() == 4);
  CHECK(*b.find_right(5).flip() == 4);
  CHECK(b.find_left(3436) == b.end_left());
  CHECK(b.find_right(-1000) == b.end_right());
}

TEST_CASE("Find with non-copyable type") {
  bimap<test_object, test_object> b;
  b.insert(test_object(3), test_object(4));
  b.insert(test_object(4), test_object(5));
  b.insert(test_object(42), test_object(1000));

  CHECK(*b.find_right(test_object(5)).flip() == test_object(4));
  CHECK(*b.find_left(test_object(3)).flip() == test_object(4));
  CHECK(b.find_left(test_object(3436)) == b.end_left());
  CHECK(b.find_right(test_object(-1000)) == b.end_right());
}

TEST_CASE("Empty") {
  bimap<int, int> b;
  CHECK(b.empty());

  auto it = b.insert(1, 1);
  CHECK_FALSE(b.empty());

  b.erase_left(it);
  CHECK(b.empty());
}

TEST_CASE("Erase iterator") {
  bimap<int, int> b;

  auto it = b.insert(1, 2);
  b.insert(5, 10);
  b.insert(100, 200);

  auto it1 = b.erase_left(it);
  CHECK(b.size() == 2);
  CHECK(*it1 == 5);

  it = b.insert(-1, -2);
  auto itr = b.erase_right(it.flip());
  CHECK(b.size() == 2);
  CHECK(*itr == 10);
}

TEST_CASE("Erase value") {
  bimap<int, int> b;

  b.insert(111, 222);
  b.insert(333, 444);
  CHECK(b.erase_left(111));
  CHECK(b.size() == 1);
  CHECK_FALSE(b.erase_right(333333));
  CHECK(b.size() == 1);
  CHECK(b.erase_right(444));
  CHECK(b.empty());
}

TEST_CASE("Erase range") {
  bimap<int, int> b;

  b.insert(1, 2);
  auto f = b.insert(2, 3);
  b.insert(3, 4);
  auto l = b.insert(4, 5);
  b.insert(5, 6);

  auto it = b.erase_left(f, l);
  CHECK(*it == 4);
  CHECK(b.size() == 3);

  auto f1 = b.insert(100, 4).flip();
  auto l1 = b.insert(200, 10).flip();

  auto it1 = b.erase_right(f1, l1);
  CHECK(*it1 == 10);
  CHECK(b.size() == 2);

  b.erase_left(b.begin_left(), b.end_left());
  CHECK(b.empty());
}

TEST_CASE("Lower bound") {
  std::vector<std::pair<int, int>> data = {{1, 2}, {2, 3}, {3, 4}, {8, 16}, {32, 66}};

  std::sort(data.begin(), data.end());

  do {
    bimap<int, int> b;

    for (const auto& p : data) {
      b.insert(p.first, p.second);
    }

    CHECK(*b.lower_bound_left(5) == 8);
    CHECK(*b.lower_bound_right(4) == 4);
    CHECK(*b.lower_bound_left(4).flip() == 16);
    CHECK(b.lower_bound_right(100) == b.end_right());
    CHECK(b.lower_bound_left(100) == b.end_left());

  } while (std::next_permutation(data.begin(), data.end()));
}

TEST_CASE("Lower bound with non-copyable type") {
  bimap<test_object, test_object> b;
  b.insert(test_object(1), test_object(2));
  b.insert(test_object(2), test_object(3));
  b.insert(test_object(3), test_object(4));
  b.insert(test_object(8), test_object(16));
  b.insert(test_object(32), test_object(66));

  CHECK(*b.lower_bound_left(test_object(5)) == test_object(8));
  CHECK(*b.lower_bound_right(test_object(4)) == test_object(4));
  CHECK(*b.lower_bound_left(test_object(4)).flip() == test_object(16));
  CHECK(b.lower_bound_right(test_object(100)) == b.end_right());
  CHECK(b.lower_bound_left(test_object(100)) == b.end_left());
}

TEST_CASE("Upper bound") {
  std::vector<std::pair<int, int>> data = {{1, 2}, {2, 3}, {3, 4}, {8, 16}, {32, 66}};

  std::sort(data.begin(), data.end());

  do {
    bimap<int, int> b;

    for (const auto& p : data) {
      b.insert(p.first, p.second);
    }

    CHECK(*b.upper_bound_left(5) == 8);
    CHECK(*b.upper_bound_right(-100) == 2);
    CHECK(b.upper_bound_right(100) == b.end_right());
    CHECK(b.upper_bound_left(400) == b.end_left());

  } while (std::next_permutation(data.begin(), data.end()));
}

TEST_CASE("Upper bound with non-copyable type") {
  bimap<test_object, test_object> b;
  b.insert(test_object(1), test_object(2));
  b.insert(test_object(2), test_object(3));
  b.insert(test_object(3), test_object(4));
  b.insert(test_object(8), test_object(16));
  b.insert(test_object(32), test_object(66));

  CHECK(*b.upper_bound_left(test_object(5)) == test_object(8));
  CHECK(*b.upper_bound_right(test_object(-100)) == test_object(2));
  CHECK(b.upper_bound_right(test_object(100)) == b.end_right());
  CHECK(b.upper_bound_left(test_object(400)) == b.end_left());
}

TEST_CASE("Copy constructor") {
  bimap<int, int> a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  auto b = a;
  CHECK(a.size() == b.size());
  CHECK(a == b);
}

TEST_CASE("Copy assignment") {
  bimap<int, int> a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  bimap<int, int> b;
  b.insert(2, 5);
  b.insert(5, 2);

  b = a;
  CHECK(a.size() == b.size());
  CHECK(a == b);
}

TEST_CASE("Copy assignment to self") {
  bimap<int, int> a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  bimap<int, int> a_copy = a;
  a = a;
  CHECK(a.size() == a_copy.size());
  CHECK(a == a_copy);
}

TEST_CASE("Move constructor") {
  bimap<address_checking_object, int> a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  {
    bimap<address_checking_object, int> a_copy = a;
    address_checking_object::set_copy_throw_countdown(1);

    bimap<address_checking_object, int> b = std::move(a);
    CHECK(b.size() == a_copy.size());
    CHECK(b == a_copy);
  }

  address_checking_object::expect_no_instances();
  address_checking_object::set_copy_throw_countdown(0);
}

TEST_CASE("Move constructor with expiring comparator") {
  using bimap = bimap<int, int, expiring_comparator, expiring_comparator>;

  bimap a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  bimap a_copy = a;

  bimap b = std::move(a);
  CHECK(b == a_copy);
}

TEST_CASE("Move constructor with tracking comparator") {
  using bimap = bimap<int, int, tracking_comparator, tracking_comparator>;

  bool cmp_left = false;
  bool cmp_right = false;

  bimap a(tracking_comparator{&cmp_left}, tracking_comparator{&cmp_right});
  a.insert(1, 4);
  a.insert(8, 8);

  bimap b = std::move(a);

  cmp_left = cmp_right = false;

  CHECK(b.at_left(1) == 4);
  CHECK(b.at_left(8) == 8);
  CHECK(cmp_left == true);
  CHECK(cmp_right == false);

  CHECK(b.at_right(4) == 1);
  CHECK(b.at_right(8) == 8);
  CHECK(cmp_right == true);
}

TEST_CASE("Move assignment") {
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
  CHECK(b.size() == a_copy.size());
  CHECK(b == a_copy);

  address_checking_object::set_copy_throw_countdown(0);
}

TEST_CASE("Move assignment to self") {
  bimap<int, int> a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  bimap<int, int> a_copy = a;
  a = std::move(a);
  CHECK(a.size() == a_copy.size());
  CHECK(a == a_copy);
}

TEST_CASE("Move assignment with expiring comparator") {
  using bimap = bimap<int, int, expiring_comparator, expiring_comparator>;

  bimap a;
  a.insert(1, 4);
  a.insert(8, 8);
  a.insert(25, 17);
  a.insert(13, 37);

  bimap b;
  b.insert(2, 5);
  b.insert(5, 2);

  bimap a_copy = a;

  b = std::move(a);
  CHECK(b == a_copy);
}

TEST_CASE("Move assignment with tracking comparator") {
  using bimap = bimap<int, int, tracking_comparator, tracking_comparator>;

  bool cmp1_left = false;
  bool cmp1_right = false;

  bimap a(tracking_comparator{&cmp1_left}, tracking_comparator{&cmp1_right});
  a.insert(1, 4);
  a.insert(8, 8);

  bool cmp2_left = false;
  bool cmp2_right = false;
  bimap b(tracking_comparator{&cmp2_left}, tracking_comparator{&cmp2_right});
  b.insert(2, 5);
  b.insert(5, 2);

  b = std::move(a);

  cmp1_left = cmp1_right = cmp2_left = cmp2_right = false;

  CHECK(b.at_left(1) == 4);
  CHECK(b.at_left(8) == 8);
  CHECK(cmp1_left == true);
  CHECK(cmp1_right == false);

  CHECK(b.at_right(4) == 1);
  CHECK(b.at_right(8) == 8);
  CHECK(cmp1_right == true);

  CHECK(cmp2_left == false);
  CHECK(cmp2_right == false);
}

TEST_CASE("Equivalence") {
  bimap<int, int> a;
  bimap<int, int> b;
  CHECK(a == b);
  CHECK_FALSE(a != b);

  a.insert(1, 2);
  a.insert(3, 4);
  b.insert(1, 2);
  CHECK_FALSE(a == b);
  CHECK(a != b);

  b.erase_left(1);
  b.insert(1, 4);
  b.insert(3, 2);
  CHECK_FALSE(a == b);
  CHECK(a != b);

  CHECK(a.end_left().flip() == a.end_right());
  CHECK(a.end_right().flip() == a.end_left());

  a.erase_left(1);
  a.erase_right(4);
  a.insert(3, 2);
  a.insert(1, 4);
  CHECK(a == b);
  CHECK_FALSE(a != b);
}

TEST_CASE("Equivalence with custom comparator") {
  using element = incomparable_int;
  using comparator = incomparable_int_custom_comparator;
  bimap<element, element, comparator, comparator> a;
  bimap<element, element, comparator, comparator> b;
  a.insert(1, 2);
  a.insert(3, 4);
  b.insert(1, 2);
  b.insert(3, 4);
  CHECK(a == b);
  CHECK_FALSE(a != b);
}

TEST_CASE("Iterator traits") {
  using bm = bimap<int, double>;
  STATIC_CHECK(std::bidirectional_iterator<bm::left_iterator>);
  STATIC_CHECK(std::bidirectional_iterator<bm::right_iterator>);
  STATIC_CHECK(std::is_same_v<std::iterator_traits<bm::left_iterator>::value_type, int>);
  STATIC_CHECK(std::is_same_v<std::iterator_traits<bm::right_iterator>::value_type, double>);
  STATIC_CHECK(std::is_same_v<std::iterator_traits<bm::left_iterator>::reference, const int&>);
  STATIC_CHECK(std::is_same_v<std::iterator_traits<bm::right_iterator>::reference, const double&>);
  STATIC_CHECK(std::is_same_v<std::iterator_traits<bm::left_iterator>::pointer, const int*>);
  STATIC_CHECK(std::is_same_v<std::iterator_traits<bm::right_iterator>::pointer, const double*>);
}

TEST_CASE("Iterator sizeof") {
  using bm = bimap<int, double>;
  STATIC_CHECK(sizeof(bm::left_iterator) <= sizeof(void*));
  STATIC_CHECK(sizeof(bm::right_iterator) <= sizeof(void*));
}

TEST_CASE("Iterator operations") {
  bimap<int, int> b;
  b.insert(3, 4);
  b.insert(100, 10);
  auto it = b.insert(-10, 100);

  auto it_next = it;
  CHECK(it_next++ == it);

  CHECK(++it == it_next--);
  CHECK(--it == it_next);
}

TEST_CASE("Iteration") {
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
  CHECK(left_values == left_values_inv);

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
  CHECK(right_values == right_values_inv);
}

TEST_CASE("Swap") {
  bimap<int, int> b, b1;
  b.insert(3, 4);
  b1.insert(4, 3);
  CHECK(*b.find_left(3) == 3);
  CHECK(*b1.find_right(3) == 3);

  using std::swap;
  swap(b, b1);
  CHECK(*b1.find_left(3) == 3);
  CHECK(*b.find_right(3) == 3);
}

TEST_CASE("Swap with tracking comparator") {
  using bimap = bimap<int, int, tracking_comparator, tracking_comparator>;

  bool cmp1_left = false;
  bool cmp1_right = false;
  bool cmp2_left = false;
  bool cmp2_right = false;

  bimap b1(tracking_comparator{&cmp1_left}, tracking_comparator{&cmp1_right});
  bimap b2(tracking_comparator{&cmp2_left}, tracking_comparator{&cmp2_right});
  b2.insert(3, 4);
  b2.insert(4, 5);

  using std::swap;
  swap(b1, b2);

  cmp1_left = cmp1_right = cmp2_left = cmp2_right = false;

  CHECK(b1.at_left(3) == 4);
  CHECK(b1.at_left(4) == 5);
  CHECK(cmp2_left == true);
  CHECK(cmp2_right == false);

  CHECK(b1.at_right(4) == 3);
  CHECK(b1.at_right(5) == 4);
  CHECK(cmp2_right == true);

  CHECK(cmp1_left == false);
  CHECK(cmp1_right == false);
}
