//
// Created by igor on 15/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_ANIMATION_DESCRIPTION_HH
#define INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_ANIMATION_DESCRIPTION_HH

#include <map>
#include <vector>
#include <chrono>

#include <neutrino/assets/tiles/tile_handle.hh>

namespace neutrino::assets {

  class animation_description {
      using frames_vec_t = std::vector<std::tuple<tile_handle, std::chrono::milliseconds>>;
    public:
      enum kind_t {
        LINEAR,
        CIRCULAR,
        DESTRUCTABLE
      };

      class sequence {
          friend class animation_description;

        public:
          [[nodiscard]] animation_seq_id_t key () const noexcept;
          [[nodiscard]] std::size_t size () const noexcept;
          [[nodiscard]] bool empty () const noexcept;
          [[nodiscard]] std::tuple<tile_handle, std::chrono::milliseconds> operator [] (std::size_t idx) const;
          [[nodiscard]] kind_t kind () const;
          sequence& add (tile_handle th, std::chrono::milliseconds duration);
          sequence& add (atlas_id_t atlas_id, cell_id_t cell, std::chrono::milliseconds duration);
        private:
          sequence (animation_seq_id_t key, const kind_t& kind, frames_vec_t& frames);
        private:
          animation_seq_id_t m_key;
          const kind_t& m_kind;
          frames_vec_t& m_frames;
      };

      class const_sequence {
          friend class animation_description;

        public:
          [[nodiscard]] animation_seq_id_t key () const noexcept;
          [[nodiscard]] std::size_t size () const noexcept;
          [[nodiscard]] bool empty () const noexcept;
          [[nodiscard]] std::tuple<tile_handle, std::chrono::milliseconds> operator [] (std::size_t idx) const;
          [[nodiscard]] kind_t kind () const;
        private:
          const_sequence (animation_seq_id_t key, kind_t kind, frames_vec_t frames);
        private:
          animation_seq_id_t m_key;
          const kind_t m_kind;
          frames_vec_t m_frames;
      };

    public:
      animation_description ();
      [[nodiscard]] sequence create (kind_t kind);
      [[nodiscard]] const_sequence get (animation_seq_id_t id) const;
      [[nodiscard]] sequence get (animation_seq_id_t id);
      [[nodiscard]] bool exists (animation_seq_id_t id) const;
      [[nodiscard]] bool exists (const sequence& seq) const;
      void remove (animation_seq_id_t id);
      void remove (const sequence& seq);
    private:
      using data_t = std::pair<kind_t, frames_vec_t>;
      std::map<animation_seq_id_t, data_t> m_data;
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_ANIMATION_DESCRIPTION_HH
