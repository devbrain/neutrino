//
// Created by igor on 26/06/2022.
//

#ifndef APPS_FON_TEST_UTILS_HH
#define APPS_FON_TEST_UTILS_HH

#include <iostream>
#include <doctest/doctest.h>
#include <neutrino/assets/image/bitmask.hh>

class test_fixture_8bit {
  public:
    test_fixture_8bit()
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
    neutrino::assets::impl::bitmask<8> a;
    neutrino::assets::impl::bitmask<8> b;
  private:
    static neutrino::assets::impl::bitmask<8> create_a();
    static neutrino::assets::impl::bitmask<8> create_b();
};


class test_fixture_16bit {
  public:
    test_fixture_16bit ()
        : a (create_a ()),
          b (create_b ()) {
    }

    void print_words () const {
      a.print_words ();
      std::cout << std::endl;
      b.print_words ();
      std::cout << std::endl;

    }

  protected:
    neutrino::assets::impl::bitmask<16> a;
    neutrino::assets::impl::bitmask<16> b;
  private:
    static neutrino::assets::impl::bitmask<16> create_a();
    static neutrino::assets::impl::bitmask<16> create_b();
};




#endif //APPS_FON_TEST_UTILS_HH
