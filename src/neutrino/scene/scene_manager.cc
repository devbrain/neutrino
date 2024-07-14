//
// Created by igor on 7/14/24.
//
#include <neutrino/scene/scene_manager.hh>

namespace neutrino {
	scene_manager::scene_manager() = default;

	scene_manager::~scene_manager() = default;

	void scene_manager::push(std::shared_ptr<scene> scene_) {
		m_stack.push(scene_);
	}

	std::shared_ptr<scene> scene_manager::pop() {
		if (m_stack.empty()) {
			return nullptr;
		}
		auto curr = m_stack.top();
		m_stack.pop();
		return curr;
	}

	std::shared_ptr<scene> scene_manager::top() {
		if (m_stack.empty()) {
			return nullptr;
		}
		return m_stack.top();
	}

	bool scene_manager::empty() const {
		return m_stack.empty();
	}
}
