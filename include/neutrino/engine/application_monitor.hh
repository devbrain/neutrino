//
// Created by igor on 07/06/2021.
//

#ifndef NEUTRINO_ENGINE_APPLICATION_MONITOR_HH
#define NEUTRINO_ENGINE_APPLICATION_MONITOR_HH

#include <neutrino/hal/application.hh>

namespace neutrino::engine {
    class application_monitor {
    public:
        virtual ~application_monitor();

        virtual void on_terminating () = 0;
        virtual void on_low_memory () = 0;
        virtual void on_will_enter_background () = 0;
        virtual void on_in_background () = 0;
        virtual void on_in_foreground () = 0;
    };
}


#endif
