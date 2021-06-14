//
// Created by igor on 07/06/2021.
//

#ifndef NEUTRINO_ENGINE_OBSERVER_HH
#define NEUTRINO_ENGINE_OBSERVER_HH

#include <neutrino/utils/mp/typelist.hh>
#include <neutrino/utils/mp/constexpr_for.hh>
#include <neutrino/utils/mp/callable/callable.hpp>

#include <tuple>
#include <vector>
#include <algorithm>
#include <type_traits>
#include <set>
#include <variant>
#include <functional>

namespace neutrino::engine {
    namespace detail {
        // ======================================================================================
        template <typename EventType>
        class observer {
        public:
            virtual ~observer() = default;
            virtual void on_event(const EventType& event) = 0;
        };
        // ======================================================================================
        class observer_monitor {
        public:
            virtual ~observer_monitor() = default;

            virtual void on_subscribed ();
            virtual void on_unsubscribed();
        };
        // ======================================================================================
        template <typename EventType>
        class functional_observer;

        template <typename EventType>
        class functional_observer <std::function<void(const EventType&)>>: public observer_monitor, public observer<EventType> {
        public:
            explicit functional_observer(std::function<void(const EventType&)>&& f) : m_runner(std::move(f)) {
            }


            void on_event(const EventType& event) override {
                m_runner(event);
            }

            void on_unsubscribed() override {
                delete this;
            }
        private:
            std::function<void(const EventType&)> m_runner;
        };


        template<typename F, typename ... Events>
        inline constexpr bool is_callable_supported_v = std::disjunction_v<std::is_invocable<F,Events>...>;
        // ======================================================================================
        template <typename ... Events>
        class basic_publisher {
        public:
            basic_publisher()
                    : m_holder{std::vector<observer<Events>*>{}...} {
            }

            virtual ~basic_publisher() {
                for (auto* monitor : m_monitors) {
                    monitor->on_unsubscribed();
                }
            }

            template<typename Functor>
            std::enable_if_t<is_callable_supported_v<Functor, Events...>, void>
            attach(Functor&& func) {
                auto f = mp::to_stdfunction(func);
                using func_t = decltype(f);
                this->template attach(new functional_observer<func_t>(std::move(f)));
            }

            template<typename Event>
            std::enable_if_t<is_callable_supported_v<void (*)(const Event&), Events...>, void>
            attach(void (*func)(const Event&) ) {
                auto f = mp::to_stdfunction(func);
                using func_t = decltype(f);
                this->template attach(new functional_observer<func_t>(std::move(f)));
            }

            template<typename Observer>
            void attach(Observer* obs) {
                bool attached = false;
                mp::constexpr_for<0, supported_events_t::size(), 1>([this, obs, &attached](auto i) {
                    using type_to_test_t = mp::type_list_at_t<i.value, supported_events_t>;
                    if (auto observer_to_test = dynamic_cast<observer<type_to_test_t>*>(obs))
                    {
                        auto& container = std::get<i.value>(m_holder);
                        auto itr = std::find(container.begin(), container.end(), observer_to_test);
                        if (itr == container.end())
                        {
                            container.push_back(observer_to_test);
                            attached = true;
                        }
                    }
                });
                if (attached) {
                    if (auto* monitor = dynamic_cast<observer_monitor*>(obs)) {
                        m_monitors.insert(monitor);
                        monitor->on_subscribed();
                    }
                }
            }

            template<class Observer>
            void detach(Observer* obs) {
                bool detached = false;
                mp::constexpr_for<0, supported_events_t::size(), 1>([this, obs, &detached](auto i) {
                    using type_to_test_t = mp::type_list_at_t<i.value, supported_events_t>;
                    if (auto observer_to_test = dynamic_cast<observer<type_to_test_t>*>(obs))
                    {
                        auto& container = std::get<i.value>(m_holder);
                        auto itr = std::find(container.begin(), container.end(), observer_to_test);
                        if (itr != container.end())
                        {
                            container.erase(observer_to_test);
                            detached = true;
                        }
                    }
                });
                if (detached) {
                    if (auto* monitor = dynamic_cast<observer_monitor*>(obs)) {
                        m_monitors.erase(monitor);
                        monitor->on_unsubscribed();
                    }
                }
            }

            template<typename Event>
            void notify(const Event& ev) {
                constexpr auto idx = mp::type_list_find_first_v<Event, supported_events_t>;
                static_assert(idx != supported_events_t::npos, "This event is unsupported");
                auto& container = std::get<idx>(m_holder);
                for (auto* obs : container) {
                    obs->on_event(ev);
                }
            }

            template<typename ... Event>
            void notify (const std::variant<Event...>& ev_variant) {
                using supplied_t = mp::type_list<Event...>;
                mp::constexpr_for<0, supplied_t ::size(), 1>([this, &ev_variant](auto i) {
                    using type_t = mp::type_list_at_t<i.value, supplied_t>;
                    if constexpr(!std::is_same_v<std::monostate, type_t>) {
                        if (auto pval = std::get_if<type_t>(&ev_variant)) {
                            this->template notify(*pval);
                        }
                    }
                });
            }

        private:
            using supported_events_t = mp::type_list<Events...>;
            using holder_t = std::tuple<std::vector<observer<Events>*>...>;

            holder_t m_holder;
            std::set<observer_monitor*> m_monitors;
        };

    }

    template <typename EventType, typename ... RestEventTypes>
    class observer : public detail::observer_monitor,
                     public detail::observer<EventType>,
                     public detail::observer<RestEventTypes>... {
    public:
        using detail::observer<EventType>::on_event;
        using detail::observer<RestEventTypes>::on_event...;
        using detail::observer_monitor::on_subscribed;
        using detail::observer_monitor::on_unsubscribed;

        virtual ~observer() = default;
    };

    // ==================================================================================
    template <typename ... Events>
    class publisher : public detail::basic_publisher<Events...> {

    };

    template <typename ... Events>
    class publisher<mp::type_list<Events...>> : public detail::basic_publisher<Events...> {

    };

    template <typename ... Events>
    class publisher<std::variant<Events...>> : public detail::basic_publisher<Events...> {
    };

}

#endif
