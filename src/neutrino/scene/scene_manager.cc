//
// Created by igor on 7/14/24.
//
#include <neutrino/scene/scene_manager.hh>
#include "events/neutrino_events.hh"
#include <bsw/override.hh>

namespace neutrino {
	scene_manager::scene_manager() = default;

	scene_manager::~scene_manager() = default;

	void scene_manager::push(const std::shared_ptr<scene>& scene_) {
		auto prev_scene = top();
		if (prev_scene) {
			prev_scene->deactivate();
		}

		scene_->setup_scene_manager(this);
		scene_->activate();
		m_stack.push_back(scene_);
	}

	std::shared_ptr<scene> scene_manager::pop() {
		if (m_stack.empty()) {
			return nullptr;
		}
		auto curr = m_stack.back();
		curr->evict();
		m_stack.pop_back();
		return curr;
	}

	std::shared_ptr<scene> scene_manager::top() {
		if (m_stack.empty()) {
			return nullptr;
		}
		return m_stack.back();
	}

	bool scene_manager::empty() const {
		return m_stack.empty();
	}

	void scene_manager::clear_events() {
		for (auto& scene_ptr : m_stack) {
			scene_ptr->m_events_reactor.reset();
		}
	}

	void scene_manager::render(sdl::renderer& renderer) {
		std::size_t start_idx = m_stack.size() - 1;
		for (auto i = m_stack.rbegin(); i != m_stack.rend(); ++i) {
			auto& scene_ptr = *i;

			if (!scene_ptr->get_flags().contains(scene::flags::TRANSPARENT)) {
				break;
			}
			start_idx--;
		}
		for (auto i=start_idx; i<m_stack.size(); i++) {
			m_stack[i]->render(renderer);
		}
	}

	void scene_manager::handle_input(const sdl::events::event_t& ev) {
		auto neutrino_event = parse_event(ev);

		auto is_neutrino_event = std::visit(bsw::overload(
			[this](const push_scene_event& e) {
				push(e.scene_obj);
				return true;
			},
			[this](const pop_scene_event& e) {
				pop();
				return true;
			},
			[this](const replace_scene_event& e) {
				pop();
				push(e.scene_obj);
				return true;
			},
			[this](const scene_timer_event& e) {
				update_timers(e);
				return true;
			},
			[](const auto&) {
				return false;
			}
			), neutrino_event);
		if (is_neutrino_event) {
			return;
		}
		auto i = m_stack.rbegin();
		bool is_system_event = (*i)->handle_system_event(ev);
		if (is_system_event) {
			++i;
			for (; i != m_stack.rend(); ++i) {
				(*i)->handle_system_event(ev);
			}
		} else {
			for (; i != m_stack.rend(); ++i) {
				auto& scene_ptr = *i;
				scene_ptr->handle_input(ev);
				if (!scene_ptr->get_flags().contains(scene::flags::PROPAGATE_EVENTS)) {
					break;
				}
			}
		}
	}

	void scene_manager::update(std::chrono::milliseconds delta_time) {
		for (auto i = m_stack.rbegin(); i != m_stack.rend(); ++i) {
			auto& scene_ptr = *i;
			scene_ptr->update(delta_time);
			if (!scene_ptr->get_flags().contains(scene::flags::PROPAGATE_UPDATES)) {
				break;
			}
		}
	}

	void scene_manager::update_timers(const scene_timer_event& e) {
		for (auto& scene_ptr : m_stack) {
			if (scene_ptr.get() == e.destination) {
				scene_ptr->on_timer(e.data);
				break;
			}
		}
	}
}
