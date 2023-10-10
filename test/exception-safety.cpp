#include "bimap.h"
#include "test-utils.h"

#include <gtest/gtest.h>

#include <random>


TEST(exception_safety, non_throwing_default_ctor) {
  faulty_run([] {
    try {
      container c;
    } catch (...) {
      fault_injection_disable dg;
      ADD_FAILURE() << "default constructor should not throw";
      throw;
    }
  });
}

TEST(exception_safety, copy_constructor) {
  faulty_run([] {
    container c1;
    mass_insert(c1, {
                        {1, 2},
                        {8, 7},
                        {5, 6},
                        {4, 3}
    });

    strong_exception_safety_guard sg(c1);

    container c2(c1);
    expect_eq(c1, c2);
  });
}

TEST(exception_safety, copy_assignment_empty) {
  faulty_run([] {
    container c1;
    mass_insert(c1, {
                        {1, 2},
                        {8, 7},
                        {5, 6},
                        {4, 3}
    });

    strong_exception_safety_guard sg(c1);

    container c2;
    c2 = c1;

    expect_eq(c1, c2);
  });
}

TEST(exception_safety, copy_assignment) {
  faulty_run([] {
    container c1;
    mass_insert(c1, {
                        {5, 6},
                        {3, 4},
                        {1, 2}
    });

    strong_exception_safety_guard sg(c1);

    container c2;
    mass_insert(c2, {
                        {11, 12},
                        { 9, 10},
                        { 7,  8}
    });
    c1 = c2;

    expect_eq(c1, c2);
  });
}

TEST(exception_safety, non_throwing_swap) {
  faulty_run([] {
    container c1;
    mass_insert(c1, {
                        {1, 2},
                        {8, 7},
                        {5, 6},
                        {4, 3}
    });

    container c2;
    mass_insert(c1, {
                        {3, 4},
                        {1, 2},
                        {7, 8},
                        {5, 6}
    });

    try {
      swap(c1, c2);
    } catch (...) {
      fault_injection_disable dg;
      ADD_FAILURE() << "swap should not throw";
      throw;
    }
  });
}

TEST(exception_safety, non_throwing_move_constructor) {
  faulty_run([] {
    container c1;
    mass_insert(c1, {
                        {1, 2},
                        {8, 7},
                        {5, 6},
                        {4, 3}
    });

    try {
      container c2(std::move(c1));

      expect_empty(c1);
      expect_equals(c2, {
                            {1, 2},
                            {4, 3},
                            {5, 6},
                            {8, 7}
      });
    } catch (...) {
      fault_injection_disable dg;
      ADD_FAILURE() << "move constructor should not throw";
      throw;
    }
  });
}

TEST(exception_safety, non_throwing_move_assignment) {
  faulty_run([] {
    container c1;
    mass_insert(c1, {
                        {1, 2},
                        {8, 7},
                        {5, 6},
                        {4, 3}
    });

    try {
      container c2;

      c2 = std::move(c1);

      expect_empty(c1);
      expect_equals(c2, {
                            {1, 2},
                            {4, 3},
                            {5, 6},
                            {8, 7}
      });
    } catch (...) {
      fault_injection_disable dg;
      ADD_FAILURE() << "move assignment should not throw";
      throw;
    }
  });
}

TEST(exception_safety, non_throwing_clear) {
  faulty_run([] {
    container c;
    mass_insert(c, {
                       {3, 4},
                       {5, 6},
                       {1, 2},
                       {0, 7}
    });
    try {
      c.clear();
    } catch (...) {
      fault_injection_disable dg;
      ADD_FAILURE() << "clear() should not throw";
      throw;
    }
  });
}

TEST(exception_safety, insert) {
  faulty_run([] {
    container c;
    mass_insert(c, {
                       {4, 2},
                       {5, 6},
                       {1, 3},
                       {2, 4}
    });

    strong_exception_safety_guard sg(c);
    c.insert(3, 7);

    expect_equals(c, {
                         {1, 3},
                         {2, 4},
                         {3, 7},
                         {4, 2},
                         {5, 6}
    });
  });
}

TEST(exception_safety, mass_insert) {
  std::mt19937 rng(std::mt19937::default_seed);
  std::uniform_int_distribution<unsigned long> dist(0, 1'000);

  std::set<int> keys;
  std::set<int> values;

  for (int i = 0; i < 16; ++i) {
    keys.insert(dist(rng));
    values.insert(dist(rng));
  }

  std::vector<int> keys_v(keys.begin(), keys.end());
  std::vector<int> vals_v(values.begin(), values.end());
  size_t len = std::min(keys_v.size(), vals_v.size());

  faulty_run([&]() {
    container c;
    for (int i = len - 1; i > 0; --i) {
      {
        strong_exception_safety_guard sg(c);
        c.insert(keys_v[i], vals_v[i]);
      }
      EXPECT_EQ(c.size(), len - i);
      for (int j = i; j < len; ++j) {
        auto& actual = *c.find_left(keys_v[j]).flip();
        EXPECT_EQ(actual, vals_v[j]);
      }
    }
  });
}

TEST(exception_safety, erase_left) {
  faulty_run([] {
    container c;
    mass_insert(c, {
                       {1,  6},
                       {4,  3},
                       {7,  8},
                       {2,  2},
                       {5,  4},
                       {3,  7},
                       {6, 10}
    });

    strong_exception_safety_guard sg(c);
    element val = 6;
    c.erase_left(c.find_left(val));

    expect_equals(c, {
                         {1, 6},
                         {2, 2},
                         {3, 7},
                         {4, 3},
                         {5, 4},
 //                      {6, 10},
                         {7, 8}
    });
  });
}

TEST(exception_safety, erase_right) {
  faulty_run([] {
    container c;
    mass_insert(c, {
                       {1,  6},
                       {4,  3},
                       {7,  8},
                       {2,  2},
                       {5,  4},
                       {3,  7},
                       {6, 10}
    });

    strong_exception_safety_guard sg(c);
    element val = 7;
    c.erase_right(c.find_right(val));

    expect_equals(c, {
                         {1,  6},
                         {2,  2},
 //                      {3,  7},
                         {4,  3},
                         {5,  4},
                         {6, 10},
                         {7,  8}
    });
  });
}

TEST(exception_safety, erase_left_and_right) {
  faulty_run([] {
    container c;
    mass_insert(c, {
                       {1,  6},
                       {2,  2},
                       {3,  7},
                       {4,  3},
                       {5,  4},
                       {6, 10},
                       {7,  8}
    });

    {
      strong_exception_safety_guard sg(c);

      element val = 3;
      c.erase_left(c.find_left(val));
      expect_equals(c, {
                           {1,  6},
                           {2,  2},
 //                        {3,  7},
                           {4,  3},
                           {5,  4},
                           {6, 10},
                           {7,  8}
      });
    }

    {
      strong_exception_safety_guard sg(c);

      element val = 4;
      c.erase_right(c.find_right(val));
      expect_equals(c, {
                           {1,  6},
                           {2,  2},
 //                        {3,  7},
                           {4,  3},
 //                        {5,  4},
                           {6, 10},
                           {7,  8}
      });
    }
  });
}

TEST(exception_safety, mass_erase) {
  std::mt19937 rng(std::mt19937::default_seed);
  std::uniform_int_distribution<unsigned long> dist(0, 1'000);

  std::set<int> keys;
  std::set<int> values;

  for (int i = 0; i < 14; ++i) {
    keys.insert(dist(rng));
    values.insert(dist(rng));
  }

  std::vector<int> keys_v(keys.begin(), keys.end());
  std::vector<int> vals_v(values.begin(), values.end());
  size_t len = std::min(keys_v.size(), vals_v.size());

  container a;
  for (int i = 0; i < len; ++i) {
    a.insert(keys_v[i], vals_v[i]);
  }

  faulty_run([&]() {
    container c = a;
    for (int i = len - 1; i >= 0; --i) {
      {
        strong_exception_safety_guard sg(c);
        if (i % 2) {
          c.erase_left(keys_v[i]);
        } else {
          c.erase_right(vals_v[i]);
        }
      }
      EXPECT_EQ(c.size(), i);
      for (int j = 0; j < i; ++j) {
        auto& actual_value = *c.find_left(keys_v[j]).flip();
        EXPECT_EQ(actual_value, vals_v[j]);
      }
    }
    expect_empty(c);
  });
}