//
// Created by igor on 10/4/24.
//

#ifndef NEUTRINO_INCLUDE_NEUTRINO_MODULES_INPUT_SYSTEMS_INPUT_SYSTEM_HH_
#define NEUTRINO_INCLUDE_NEUTRINO_MODULES_INPUT_SYSTEMS_INPUT_SYSTEM_HH_

#include <neutrino/config/config_hotkey.hh>
#include <neutrino/events/hotkey_mapper.hh>
#include <neutrino/ecs/abstract_system.hh>
#include <libassert/assert.hpp>
#include <vector>
#include <map>

namespace neutrino::ecs {

	template <typename Enum>
	class input_system : public abstract_system {
	 public:
		void bind_hotkey(Enum key_id, const config_hotkey& hotkey) {
			m_hotkeys.insert(std::make_pair(key_id, hotkey));
		}

		config_hotkey& get_hotkey(Enum key_id) const {
			auto itr = m_hotkeys.find(key_id);
			ASSERT(itr != m_hotkeys.end());
			return itr->second;
		}

	 protected:
		bool check(Enum key_id) const {
			auto itr = m_hotkeys.find(key_id);
			DEBUG_ASSERT(itr != m_hotkeys.end());
			return neutrino::hotkey_mapper::is_pressed(itr->second);
		}

		virtual void handle_input(registry& registry) = 0;

	 private:
		void update(registry& registry, [[maybe_unused]] std::chrono::milliseconds delta_t) override{
			handle_input(registry);
		}

	 private:
		std::map<Enum, config_hotkey> m_hotkeys;
	};
}

#endif
