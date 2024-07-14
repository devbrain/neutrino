//
// Created by igor on 7/13/24.
//

#include <iostream>
#include <neutrino/events/events.hh>
#include <sdlpp/sdlpp.hh>

struct ev_a {};
bool f(const neutrino::sdl::events::keyboard& ev, ev_a&) {
	if (ev.pressed && ev.scan_code == neutrino::sdl::A) {
		return true;
	}
	return false;
}

int main(int argc, char* argv[]) {
	using namespace neutrino;

	try {

		using namespace neutrino;
		sdl::system initializer (sdl::init_flags::VIDEO);
		int w = 640;
		int h = 480;

		sdl::window window(w, h, sdl::window::flags::SHOWN);
		sdl::renderer renderer(window, sdl::renderer::flags::ACCELERATED);

		events_reactor r;
		register_event(r, [] (const sdl::events::keyboard& ev, ev_a&) {
				if (ev.pressed && ev.scan_code == sdl::LCTRL) {
					return true;
				}
				return false;
				});

		bool done = false;

		while (!done) {

			r.reset();
			while (true) {
				SDL_Event sdl_event;
				int rc = SDL_PollEvent(&sdl_event);
				if (rc == 1) {
					if (sdl_event.type == SDL_QUIT) {
						done = true;
						break;
					}
					auto internal_event = sdl::map_event(sdl_event);
					r.handle(internal_event);
				} else {
					break;
				}
			}

			if (const auto* e = get_event<ev_a>(r)) {
				std::cout << "A" << std::endl;
			}

			renderer.set_active_color(neutrino::sdl::colors::black);
			renderer.clear();
			renderer.present();
		}

	} catch (std::exception& e) {
		std::cerr << "Error occured : " << e.what() << std::endl;
	}
}