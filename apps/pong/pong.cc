//
// Created by igor on 02/05/2022.
//

#include <neutrino/kernel/application.hh>
#include <neutrino/utils/override.hh>
#include <neutrino/math/quad_tree.hh>
#include <variant>

struct circle {
  neutrino::math::point2d center;
  unsigned radius;
};

struct phys_component {
  neutrino::math::box2d<float> aabb;
  neutrino::math::point2f speed;
  std::variant<circle, neutrino::math::rect> shape;
};

struct shape_component {
  neutrino::hal::color color;
};

// ===========================================================================
class app : public neutrino::application {
    static constexpr auto EV_EXIT = "exit";
    static constexpr auto EV_FULLSCREEN = "fullscreen";
  public:
    app () : m_renderer (nullptr) {
      m_box_getter = std::make_unique<box_getter>(registry());
    }

  private:
    [[nodiscard]] neutrino::application_description describe() const noexcept override{
      neutrino::main_window_description d(640, 480);

      d.resizable (true);
      return {d, 60};
    }

    void init(neutrino::kernel::context& ctx) override{
      m_renderer = &ctx.video().renderer();

      input_config().when_pressed (neutrino::key_mod_t::ALT, neutrino::scan_code_t::RETURN, EV_FULLSCREEN);
      input_config().when_pressed (neutrino::scan_code_t::ESCAPE, EV_EXIT);

      neutrino::math::box2d<float> screen(0,0, 640, 480);
      m_quad_tree = std::make_unique<quad_tree_t>(screen, *m_box_getter);

      create_components();
    }

    void update_logic(std::chrono::milliseconds ms) override {
      if (events()[EV_EXIT]) {
        this->close();
      }
      if (events()[EV_FULLSCREEN]) {
        this->toggle_fullscreen();
      }

      update_physics (ms);
    }

    void update_physics(std::chrono::milliseconds ms) {
      auto cols = m_quad_tree->find_all_intersections();
      if (!cols.empty()) {
        for (const auto& [e1, e2] : cols) {
          auto& p1 = registry().get_component<phys_component>(e1);
          p1.speed = -p1.speed;
          auto& p2 = registry().get_component<phys_component>(e2);
          p2.speed = -p2.speed;
        }
      }

      registry().for_each<phys_component>([this, ms](neutrino::ecs::id_t id, phys_component& pc) {
        float s = ms.count ();

        auto delta_s = pc.speed * (s / 50.0f);
        auto new_pos = pc.aabb.point + delta_s;
        bool do_update = false;
        if (new_pos.x > 0 && new_pos.y > 0 && new_pos.x + pc.aabb.dims.x < 640 && new_pos.y + pc.aabb.dims.y < 480)
        {
          do_update = true;
        } else {
          if (new_pos.x < 0 || new_pos.x + pc.aabb.dims.x >= 640) {
            pc.speed.x = -pc.speed.x;
          }
          if (new_pos.y < 0 || new_pos.y + pc.aabb.dims.y >= 480) {
            pc.speed.y = -pc.speed.y;
          }
        }
        if (do_update) {
          pc.aabb.point = new_pos;
          m_quad_tree->remove (id);
          m_quad_tree->add (id);
          std::visit (
              neutrino::utils::overload (
                  [new_pos] (neutrino::math::rect& r) {
                    r.point.x = new_pos.x;
                    r.point.y = new_pos.y;
                  },
                  [delta_s, new_pos] (circle& c) {

                    c.center.x = new_pos.x + c.radius;
                    c.center.y = new_pos.y + c.radius;
                  }
              ),
              pc.shape
          );
        }
      });
    }

    void draw_frame() override {
      // m_renderer->active_color({0xFF,0,0,0xFF});
      registry().for_each<shape_component, phys_component>([this](const shape_component& shape, const phys_component& phys) {
        std::visit (
            neutrino::utils::overload (
                [this, &shape](const neutrino::math::rect& r) {
                  m_renderer->active_color(shape.color);
                  this->m_renderer->rectangle (r);
                },
                [this, &shape, &phys](const circle& c) {
                    m_renderer->active_color(shape.color);
                    this->m_renderer->aa_circle (c.center, c.radius);
                  }
                ),
                phys.shape
            );
      });
    }

    neutrino::hal::renderer* m_renderer;

    struct box_getter {
      explicit box_getter(neutrino::ecs::registry& reg)
      : m_reg(reg) {}

      neutrino::ecs::registry& m_reg;
      neutrino::math::box2d<float> operator () (neutrino::ecs::id_t id) const {
        return m_reg.get_component<phys_component>(id).aabb;
      }
    };

    using quad_tree_t = neutrino::math::quad_tree<neutrino::ecs::id_t, box_getter>;
    std::unique_ptr<box_getter> m_box_getter;
    std::unique_ptr<quad_tree_t> m_quad_tree;
  private:
    neutrino::ecs::id_t create_object(circle c, neutrino::hal::color color, neutrino::math::point2f speed) {
       auto id = create_entity();
       shape_component shp{color};
       registry().attach_component (id, shp);
       float x1 = c.center.x - c.radius;
       float y1 = c.center.y - c.radius;
       neutrino::math::box2d<float> aabb(x1, y1, (float)2*c.radius, (float)2*c.radius);
       phys_component pc{aabb, speed, c};
       registry().attach_component (id, pc);
       m_quad_tree->add (id);
       return id;
    }

    neutrino::ecs::id_t create_object(neutrino::math::rect c, neutrino::hal::color color, neutrino::math::point2f speed) {
      auto id = create_entity();
      shape_component shp{color};
      registry().attach_component (id, shp);

      phys_component pc{neutrino::math::convert<float>(c), speed, c};
      registry().attach_component (id, pc);
      m_quad_tree->add (id);
      return id;
    }

    void create_components() {
      create_object (circle{{450,450}, 10}, neutrino::hal::color (0x00,0xFF, 0xFF), {-1.0, 3.0});
      create_object (circle{{150,150}, 10}, neutrino::hal::color (0xFF,0x00, 0xFF), {1.0, -1.0});
      create_object (neutrino::math::rect{{10,10}, 10, 50}, neutrino::hal::color (0x00,0x00, 0xFF), {4.0, -2.0});
    }
};

int main(int argc, char* argv[]) {
  app a;
  a.execute();
  return 0;
}

