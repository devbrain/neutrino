//
// Created by igor on 7/14/24.
//

#ifndef NEUTRINO_SCENE_MANAGER_HH
#define NEUTRINO_SCENE_MANAGER_HH

#include <memory>
#include <stack>
#include <neutrino/scene/scene.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino {
	class NEUTRINO_EXPORT scene_manager {
		public:
			scene_manager();
			~scene_manager();

			scene_manager(const scene_manager&) = delete;
			scene_manager& operator =(const scene_manager&) = delete;

			void push(std::shared_ptr<scene> scene_);
			std::shared_ptr<scene> pop();
			std::shared_ptr<scene> top();

			[[nodiscard]] bool empty() const;
		private:
			std::stack <std::shared_ptr<scene>> m_stack;
	};
}

#endif
