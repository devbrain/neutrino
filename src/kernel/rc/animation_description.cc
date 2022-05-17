//
// Created by igor on 15/05/2022.
//

#include <neutrino/kernel/rc/animation_description.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::kernel {

  animation_seq_id_t animation_description::sequence::key() const noexcept {
    return m_key;
  }

  std::size_t animation_description::sequence::size() const noexcept {
    return m_frames.size();
  }

  bool animation_description::sequence::empty() const noexcept {
    return m_frames.empty();
  }

  std::tuple<tile_handle, std::chrono::milliseconds> animation_description::sequence::operator [] (const std::size_t idx) const {
    return m_frames.at (idx);
  }

  animation_description::sequence& animation_description::sequence::add(tile_handle th, std::chrono::milliseconds duration) {
    m_frames.emplace_back (th, duration);
    return *this;
  }

  animation_description::sequence& animation_description::sequence::add(atlas_id_t atlas_id, cell_id_t cell, std::chrono::milliseconds duration) {
    m_frames.emplace_back (tile_handle(atlas_id, cell), duration);
    return *this;
  }

  animation_description::kind_t animation_description::sequence::kind() const {
    return m_kind;
  }


  animation_description::sequence::sequence(animation_seq_id_t key, const kind_t& kind, frames_vec_t& frames)
  : m_key(key), m_kind(kind), m_frames(frames) {}


  animation_seq_id_t animation_description::const_sequence::key() const noexcept {
    return m_key;
  }

  std::size_t animation_description::const_sequence::size() const noexcept {
    return m_frames.size();
  }

  bool animation_description::const_sequence::empty() const noexcept {
    return m_frames.empty();
  }

  std::tuple<tile_handle, std::chrono::milliseconds> animation_description::const_sequence::operator [] (const std::size_t idx) const {
    return m_frames.at (idx);
  }

  animation_description::kind_t animation_description::const_sequence::kind() const {
    return m_kind;
  }


  animation_description::const_sequence::const_sequence(animation_seq_id_t key, kind_t kind, frames_vec_t frames)
      : m_key(key), m_kind(kind), m_frames(std::move(frames)) {}

  animation_description::animation_description() = default;

  animation_description::sequence animation_description::create(kind_t kind) {
    animation_seq_id_t k(static_cast<uint16_t>(m_data.size()));
    auto itr = std::get<0>(m_data.insert (std::make_pair (k, std::make_pair(kind, frames_vec_t{}))));
    return {itr->first, std::get<0>(itr->second), std::get<1>(itr->second)};
  }

  animation_description::sequence animation_description::get(animation_seq_id_t id) {
    auto itr = m_data.find (id);
    ENFORCE (itr != m_data.end());
    return {itr->first, std::get<0>(itr->second), std::get<1>(itr->second)};
  }

  animation_description::const_sequence animation_description::get(animation_seq_id_t id) const {
    auto itr = m_data.find (id);
    ENFORCE (itr != m_data.end());
    return {itr->first, std::get<0>(itr->second), std::get<1>(itr->second)};
  }

  bool animation_description::exists(animation_seq_id_t id) const {
    auto itr = m_data.find (id);
    return itr != m_data.end();
  }

  bool animation_description::exists(const sequence& seq) const {
    return exists (seq.key());
  }

  void animation_description::remove(animation_seq_id_t id) {
    auto itr = m_data.find (id);
    if (itr != m_data.end()) {
      m_data.erase (itr);
    }
  }

  void animation_description::remove(const sequence& seq) {
    remove (seq.key());
  }
}