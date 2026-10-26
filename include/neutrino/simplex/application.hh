//
// Created by igor on 18/10/2026.
//

#pragma once

#include <string>
#include <memory>
#include <neutrino/neutrino_export.h>
#include <neutrino/input/input_snapshot.hh>

namespace sdlpp {
    class abstract_application;
}

namespace neutrino::simplex {
    namespace detail {
        struct application_access;
    }

    /// Include <neutrino/simplex/entry_point.hh> in the main source file and use
    /// SIMPLEX_MAIN(DerivedApplication, constructor_arguments...) to run the app.
    class NEUTRINO_EXPORT application {
        friend struct detail::application_access;
        public:
            /// Fixed logical canvas size; scale sets the initial window size
            /// (fractional window points are truncated). Presentation uses integer scaling.
            /// Throws std::invalid_argument for invalid or unrepresentable dimensions.
            application(int width_px, int height_px, const std::string& title, float scale = 1.0);

            virtual ~application();

            /// Runs once with drawing directed to the canvas. False means startup failure.
            virtual bool on_create();
            /// Runs once per frame with elapsed seconds and an immutable input snapshot.
            /// Input is captured before binding the canvas; pointer().render is in logical
            /// canvas coordinates. The reference is valid only during this callback.
            /// Canvas contents persist; clear explicitly when needed.
            /// False requests exit with no further updates.
            virtual bool on_update(float dt, const input_snapshot& input) = 0;
            /// Runs once if on_create was entered, while rendering services are alive.
            /// Exceptions are logged and contained; cleanup is never retried.
            virtual void on_destroy();

        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;

            [[nodiscard]] sdlpp::abstract_application* get_application() noexcept;
    };
}
