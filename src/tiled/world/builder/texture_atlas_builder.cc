//
// Created by igor on 27/08/2021.
//

#include <neutrino/tiled/world/builder/texture_atlas_builder.hh>

#include <neutrino/hal/video/surface.hh>
#include <neutrino/utils/exception.hh>
#include <neutrino/utils/override.hh>
#include <neutrino/hal/video/image_loader.hh>
#include <neutrino/utils/io/memory_stream_buf.hh>

#include <utility>


namespace neutrino::tiled {

  namespace {
    math::rect get_coords (unsigned id, unsigned canvas_w, unsigned canvas_h, const tile_sheet_info& inf)  {
      auto margin = inf.margin();
      auto spacing = inf.spacing();
      auto tilewidth = inf.tile_width();
      auto tileheight = inf.tile_height();
      auto offset_x = inf.offset_x();
      auto offset_y = inf.offset_y();

      unsigned width = (canvas_w - 2 * margin + spacing) / (tilewidth + spacing); // number of tiles
      unsigned height = (canvas_h - 2 * margin + spacing) / (tileheight + spacing); // number of tiles

      unsigned tu = id % width;
      unsigned tv = id / width;
      ENFORCE(tv < height);

      unsigned du = margin + tu * spacing + offset_x;
      unsigned dv = margin + tv * spacing + offset_y;
      ENFORCE(((tu + 1) * tilewidth + du) <= canvas_w);
      ENFORCE(((tv + 1) * tileheight + dv) <= canvas_h);

      return {(int) (tu * tilewidth + du), (int) (tv * tileheight + dv), (int) tilewidth, (int) tileheight};
    }
  }

  tile_sheet_info::tile_sheet_info (image_source_t image_source,
                                    unsigned tile_width,
                                    unsigned tile_height,
                                    unsigned spacing,
                                    unsigned margin,
                                    unsigned offset_x,
                                    unsigned offset_y,
                                    std::size_t num_tiles)
      : m_image_source{std::move (image_source)}, m_tile_width{tile_width}, m_tile_height{tile_height}, m_spacing{
      spacing}, m_margin{margin}, m_offset_x(offset_x), m_offset_y(offset_y), m_num_tiles(num_tiles) {

  }

  const image_source_t& tile_sheet_info::image_source () const noexcept {
    return m_image_source;
  }

  unsigned tile_sheet_info::tile_width () const noexcept {
    return m_tile_width;
  }

  unsigned tile_sheet_info::tile_height () const noexcept {
    return m_tile_height;
  }

  unsigned tile_sheet_info::spacing () const noexcept {
    return m_spacing;
  }

  unsigned tile_sheet_info::margin () const noexcept {
    return m_margin;
  }

  unsigned int tile_sheet_info::offset_x () const noexcept {
    return m_offset_x;
  }

  unsigned int tile_sheet_info::offset_y () const noexcept {
    return m_offset_y;
  }

  size_t tile_sheet_info::num_tiles () const noexcept {
    return m_num_tiles;
  }

  detail::base_image::base_image (std::optional<hal::color> transparent_color, std::optional<math::point2d> dimensions,
                                  std::optional<std::string> format)
      : m_transparent_color (transparent_color),
        m_dimensions (dimensions),
        m_format (std::move (format)) {

  }

  std::optional<hal::color> detail::base_image::transparent_color () const noexcept {
    return m_transparent_color;
  }

  std::optional<math::point2d> detail::base_image::dimensions () const noexcept {
    return m_dimensions;
  }

  std::optional<std::string> detail::base_image::format () const noexcept {
    return m_format;
  }

  named_image::named_image (const std::optional<hal::color>& transparent_color,
                            const std::optional<math::point2d>& dimensions, const std::optional<std::string>& format,
                            std::string name)
      : base_image (transparent_color, dimensions, format), m_name (std::move (name)) {
  }

  const std::string& named_image::name () const {
    return m_name;
  }

  image_data::image_data (const std::optional<hal::color>& transparent_color,
                          const std::optional<math::point2d>& dimensions, const std::optional<std::string>& format,
                          std::vector<char> data)
      : base_image (transparent_color, dimensions, format), m_data (std::move (data)) {
  }

  const std::vector<char>& image_data::data () const {
    return m_data;
  }

  texture_atlas_builder::texture_atlas_builder (std::vector<tile_sheet_info> info)
      : m_info (std::move (info)) {
  }

  void texture_atlas_builder::add (tile_sheet_info&& inf) {
    m_info.push_back (std::move (inf));
  }

    texture_atlas texture_atlas_builder::build (hal::renderer& renderer,
                                              std::function<hal::surface (const std::string&)> loader) const {

      texture_atlas out;
      for (const auto& inf : m_info) {
      const auto& img_source = inf.image_source ();
      std::optional<hal::color> transparent_color;
      hal::surface surface;
      std::visit (
          utils::overload (
              [&transparent_color, &surface, &loader] (const named_image& data) {
                transparent_color = data.transparent_color ();
                surface = loader (data.name ());
                if (!surface) {
                  RAISE_EX("Failed to load image from ", data.name());
                }
              },
              [&transparent_color, &surface] (const image_data& data) {
                transparent_color = data.transparent_color ();
                const auto& buffer = data.data();
                utils::io::memory_input_stream is(buffer.data(), buffer.size());
                surface = hal::load (is);
                if (!surface) {
                  RAISE_EX("Failed to load image from buffer");
                }
              }
          ),
          img_source
      );
      if (transparent_color) {
        surface.color_key (*transparent_color);
      }

      hal::texture texture(renderer, surface);
      unsigned w, h;
      std::tie(std::ignore, std::ignore, w, h) = surface.pixels_data();
      std::vector<math::rect> coords;
      coords.resize(inf.num_tiles());

      for (std::size_t tile_id = 0; tile_id < inf.num_tiles(); tile_id++) {
        coords.push_back (get_coords (tile_id, w, h, inf));
      }
      out.add (std::move(texture), std::move (coords));
    }
    return out;
  }

  texture_atlas_builder& operator << (texture_atlas_builder& builder, tile_sheet_info&& inf) {
    builder.add (std::move (inf));
    return builder;
  }
}
