//
// Created by igor on 01/07/2020.
//

#ifndef NEUTRINO_UTILS_SINGLETON_HH
#define NEUTRINO_UTILS_SINGLETON_HH
#include <neutrino/utils/register_at_exit.hh>

namespace neutrino::utils {
  template <class T>
  class singleton {
    public:
      static T& instance () {
        static holder initializer;
        return *initializer.object;
      }

    private:
      struct holder {
        T* object;

        holder () {
          object = new T;
          delete_at_exit (object);
        }
      };
  };
}

#endif
