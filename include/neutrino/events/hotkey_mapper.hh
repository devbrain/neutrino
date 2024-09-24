//
// Created by igor on 9/12/24.
//

#ifndef NEUTRINO_EVENTS_HOTKEY_MAPPER_HH
#define NEUTRINO_EVENTS_HOTKEY_MAPPER_HH


#include <vector>
#include <neutrino/neutrino_export.hh>
#include <neutrino/config/config_hotkey.hh>
#include <neutrino/events/events_reactor.hh>

namespace neutrino {

    struct NEUTRINO_EXPORT hotkey_pressed_event {
        hotkey_pressed_event()
            : hotkey_id(-1) {}
        int hotkey_id;
		std::vector<bool> hotkeys;
    };



    class NEUTRINO_EXPORT hotkey_mapper {
        public:
            hotkey_mapper();
            void register_hotkey(const config_hotkey& hotkey, int hotkey_id);
            void unregister_hotkey(int hotkey_id);
            void add_to_reactor(events_reactor& reactor);
        private:
            bool handle(hotkey_pressed_event& ev);
        private:
            using hotkey_entry_t = std::pair<config_hotkey, int>;
            std::vector<hotkey_entry_t> m_hotkeys;
			std::size_t m_max_id;
    };
}

#endif
