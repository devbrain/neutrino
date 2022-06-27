//
// Created by igor on 26/06/2022.
//

#ifndef APPS_FON_TEST_UTILS_HH
#define APPS_FON_TEST_UTILS_HH

#include <iostream>

#include "bm.hh"

class TestFixture1 {
  public:
    TestFixture1()
    : a(create_a()),
      b(create_b()) {
    }

    void print_words() const {
      a.print_words();
      std::cout << std::endl;
      b.print_words();
      std::cout << std::endl;

    }
  protected:
    bitmask<8> a;
    bitmask<8> b;
  private:
    static bitmask<8> create_a() {
      bitmask<8> a (9, 9);
      a.set (1, 0);
      a.set (4, 0);
      a.set (1, 1);
      a.set (6, 1);
      a.set (8, 1);
      a.set (1, 2);
      a.set (2, 2);
      a.set (2, 3);
      a.set (3, 3);
      a.set (4, 3);
      a.set (7, 3);
      a.set (1, 4);
      a.set (3, 4);
      a.set (6, 4);
      a.set (0, 5);
      a.set (2, 5);
      a.set (6, 5);
      a.set (7, 5);
      a.set (8, 5);
      a.set (1, 6);
      a.set (4, 6);
      a.set (8, 6);
      a.set (3, 7);
      a.set (5, 7);
      a.set (6, 7);
      a.set (7, 7);
      a.set (1, 8);
      a.set (2, 8);
      a.set (4, 8);
      a.set (7, 8);
      return a;
    }

    static bitmask<8> create_b() {
      bitmask<8> b (9, 9);
      b.set (1, 0);
      b.set (2, 0);
      b.set (4, 0);
      b.set (5, 1);
      b.set (6, 1);
      b.set (1, 2);
      b.set (2, 2);
      b.set (3, 2);
      b.set (5, 2);
      b.set (7, 2);
      b.set (3, 3);
      b.set (7, 3);
      b.set (8, 3);
      b.set (1, 4);
      b.set (2, 4);
      b.set (5, 4);
      b.set (7, 4);
      b.set (1, 5);
      b.set (4, 5);
      b.set (5, 5);
      b.set (6, 5);
      b.set (8, 5);
      b.set (1, 6);
      b.set (3, 6);
      b.set (4, 6);
      b.set (5, 6);
      b.set (0, 7);
      b.set (3, 7);
      b.set (6, 7);
      b.set (7, 7);
      b.set (4, 8);
      b.set (5, 8);
      b.set (6, 8);
      b.set (7, 8);
      return b;
    }
};


#endif //APPS_FON_TEST_UTILS_HH
