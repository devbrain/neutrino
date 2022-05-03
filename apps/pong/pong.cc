//
// Created by igor on 02/05/2022.
//

#include <neutrino/kernel/application.hh>
#include <neutrino/utils/override.hh>
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
    }

  private:
    [[nodiscard]] neutrino::application_description describe() const noexcept override{
      neutrino::main_window_description d(640, 480);
      d.resizable (true);
      return {d, 60};
    }

    void init(neutrino::hal::renderer& renderer) override{
      m_renderer = &renderer;

      input_config().when_pressed (neutrino::key_mod_t::ALT, neutrino::scan_code_t::RETURN, EV_FULLSCREEN);
      input_config().when_pressed (neutrino::scan_code_t::ESCAPE, EV_EXIT);

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
      registry().for_each<phys_component>([this, ms](auto id) {
        auto& pc = registry ().get_component<phys_component> (id);
        float s = ms.count ();
        auto new_pos = pc.aabb.point + pc.speed * (s / 200.0f);
        if (new_pos.x > 0 && new_pos.y > 0 && new_pos.x + pc.aabb.dims.x < 640 && new_pos.y + pc.aabb.dims.y < 480)
        {
          pc.aabb.point = new_pos;
          std::visit (
            neutrino::utils::overload (
                [this, new_pos](neutrino::math::rect& r) {
                  r.point.x = new_pos.x;
                  r.point.y = new_pos.y;
                },
                [this, new_pos](circle& c) {
                  c.center.x = new_pos.x;
                  c.center.y = new_pos.y;
                  }
                ),
                pc.shape
            );
        }
      });
    }

    void draw_frame() override {
      // m_renderer->active_color({0xFF,0,0,0xFF});
      registry().for_each<shape_component, phys_component>([this](auto id) {
        const auto& shape = registry().template get_component<shape_component>(id);
        const auto& phys = registry().template get_component<phys_component>(id);
        std::visit (
            neutrino::utils::overload (
                [this, &shape](const neutrino::math::rect& r) {
                  m_renderer->active_color(shape.color);
                  this->m_renderer->rectangle (r);
                },
                [this, &shape](const circle& c) {
                    m_renderer->active_color(shape.color);
                    this->m_renderer->aa_circle (c.center, c.radius);
                  }
                ),
                phys.shape
            );
      });
    }

    neutrino::hal::renderer* m_renderer;

  private:
    neutrino::ecs::id_t create_object(circle c, neutrino::hal::color color, neutrino::math::point2f speed) {
       auto id = create_entity();
       shape_component shp{color};
       registry().attach_component (id, shp);
       float x1 = c.center.x - c.radius;
       float y1 = c.center.y - c.radius;

       phys_component pc{{x1,y1, (float)2*c.radius, (float)2*c.radius}, speed, c};
       registry().attach_component (id, pc);
       return id;
    }

    neutrino::ecs::id_t create_object(neutrino::math::rect c, neutrino::hal::color color, neutrino::math::point2f speed) {
      auto id = create_entity();
      shape_component shp{color};
      registry().attach_component (id, shp);

      phys_component pc{neutrino::math::convert<float>(c), speed, c};
      registry().attach_component (id, pc);
      return id;
    }

    void create_components() {
      create_object (circle{{50,50}, 10}, neutrino::hal::color (0x00,0xFF, 0xFF), {-1.0, 2.0});
      create_object (circle{{150,150}, 10}, neutrino::hal::color (0xFF,0x00, 0xFF), {1.0, -2.0});
      create_object (neutrino::math::rect{{10,10}, 10, 50}, neutrino::hal::color (0x00,0x00, 0xFF), {1.0, -2.0});
    }


};

int main(int argc, char* argv[]) {
  app a;
  a.execute();
  return 0;
}

