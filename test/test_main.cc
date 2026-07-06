#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <cstdlib>

#ifdef _WIN32
namespace {
    void set_env(const char* name, const char* value) {
        _putenv_s(name, value);
    }
}
#else
namespace {
    void set_env(const char* name, const char* value) {
        setenv(name, value, 1);
    }
}
#endif

int main(int argc, char** argv) {
    set_env("SDL_AUDIODRIVER", "dummy");
    set_env("SDL_AUDIO_DRIVER", "dummy");
    set_env("SDL_VIDEODRIVER", "dummy");
    set_env("SDL_VIDEO_DRIVER", "dummy");
    // The dummy video driver starts SDL's raw evdev input backend, which logs
    // a permission error for every /dev/input/event* device it cannot open.
    set_env("SDL_LOGGING", "input=critical");

    return doctest::Context(argc, argv).run();
}
