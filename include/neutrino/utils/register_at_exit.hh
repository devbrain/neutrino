#ifndef NEUTRINO_UTILS_REGISTER_AT_EXIT_HH
#define NEUTRINO_UTILS_REGISTER_AT_EXIT_HH

#include <functional>
#include <queue>
#include <cstdlib>

namespace {
  using queue_t = std::queue<std::function<void ()>>;
  static queue_t* callbacks = nullptr;

  struct queue_init {

    queue_init () {
      if (!callbacks) {
        callbacks = new queue_t;
        std::atexit ([] () {
          if (callbacks) {
            while (!callbacks->empty ()) {
              try {
                callbacks->front () ();
              }
              catch (...) {

              }
              callbacks->pop ();
            }
            delete callbacks;
          }
        });
      }
    }

    void add (std::function<void ()> f) {
      callbacks->push (f);
    }
  };

}

namespace neutrino {
  inline
  void register_at_exit (std::function<void ()> callback) {
    static queue_init static_initializer;
    static_initializer.add (callback);
  }

  template <typename T>
  void delete_at_exit (T* ptr) {
    register_at_exit ([ptr] () { delete ptr; });
  }
}

#endif
