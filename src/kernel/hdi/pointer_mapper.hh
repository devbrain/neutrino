//
// Created by igor on 19/04/2022.
//

#ifndef SRC_KERNEL_HDI_POINTER_MAPPER_HH
#define SRC_KERNEL_HDI_POINTER_MAPPER_HH

#include <neutrino/kernel/hdi/pointer_config.hh>
#include <neutrino/kernel/hdi/events.hh>

namespace neutrino {
  class pointer_mapper : public pointer_config_base {
    public:
      pointer_mapper(events_holder& ev);

      void ignore_motion(bool f) override;
      void ignore_clicks(bool f) override;
      void ignore_releases(bool f) override;

      void handle_event(const hal::events::pointer& ev);
    private:
      events_holder& m_events;
      bool m_ignore_motion;
      bool m_ignore_clicks;
      bool m_ignore_releases;
  };
}

#endif //SRC_KERNEL_HDI_POINTER_MAPPER_HH
