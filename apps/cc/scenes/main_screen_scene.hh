//
// Created by igor on 7/16/24.
//

#ifndef  MAIN_SCREEN_SCENE_HH
#define  MAIN_SCREEN_SCENE_HH

#include <chrono>
#include <neutrino/scene/scene.hh>

class main_screen_scene : public neutrino::scene {
	public:
		main_screen_scene(neutrino::texture_id_t upper,
		                  neutrino::texture_id_t lower,
		                  double rows_per_second,
		                  std::chrono::seconds wait_time
		);

	private:
		void update(std::chrono::milliseconds delta_time) override;
		void render(neutrino::sdl::renderer& renderer) override;
		void initialize() override;

	private:
		neutrino::texture_id_t m_upper;
		neutrino::texture_id_t m_lower;
		double m_rows_per_second;
		std::chrono::seconds m_wait_time;

		std::chrono::milliseconds m_time_in_state;
		bool m_hit_bottom;

		enum move_direction_t {
			NONE,
			UP,
			DOWN
		};

		move_direction_t m_direction;
		const neutrino::sdl::texture* m_up{};
		neutrino::sdl::rect m_up_dims;


		const neutrino::sdl::texture* m_down{};
		neutrino::sdl::rect m_down_dims;
		neutrino::sdl::rect m_window;
};

#endif
