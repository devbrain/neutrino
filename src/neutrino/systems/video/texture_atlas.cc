//
// Created by igor on 7/15/24.
//

#include <neutrino/systems/video/texture_atlas.hh>

namespace neutrino {
	static std::size_t counter = 0;

	texture_id_t texture_atlas::add(sdl::texture&& texture) {
		texture_id_t tid(counter++);
		m_textures.insert(std::make_pair(tid, tiles_map({}, std::move(texture))));
		return tid;
	}

	texture_id_t texture_atlas::add(sdl::texture&& texture, std::vector<sdl::rect>&& rects) {
		texture_id_t tid(counter++);
		m_textures.insert(std::make_pair(tid, tiles_map(std::move(rects), std::move(texture))));
		return tid;
	}

	texture_id_t texture_atlas::add(sdl::surface&& srf, const sdl::renderer& renderer) {
		return add (sdl::texture(renderer, srf));
	}

	texture_id_t texture_atlas::add(sdl::surface&& srf, std::vector<sdl::rect>&& rects, const sdl::renderer& renderer) {
		return add (sdl::texture(renderer, srf), std::move(rects));
	}

	std::tuple<const sdl::texture*, sdl::rect> texture_atlas::get(texture_id_t tid) const {
		if (tid == INVALID_TEXTURE_VALUE) {
			return {nullptr, {}};
		}
		auto i = m_textures.find(tid);
		if (i == m_textures.end()) {
			return {nullptr, {}};
		}
		return {&i->second.m_texture, i->second.m_dims};
	}

	std::tuple<const sdl::texture*, sdl::rect> texture_atlas::get(const tile& tlid) const {
		if (tlid.m_id == EMPTY_TILE_VALUE || tlid.m_texture == INVALID_TEXTURE_VALUE) {
			return {nullptr, {}};
		}
		const auto ridx = tlid.m_id.value_of();
		auto i = m_textures.find(tlid.m_texture);
		return {&i->second.m_texture, i->second.m_rects[ridx]};
	}

	void texture_atlas::remove(texture_id_t tid) {
		auto i = m_textures.find(tid);
		if (i != m_textures.end()) {
			m_textures.erase(i);
		}
	}

	texture_atlas::tiles_map::tiles_map(std::vector <sdl::rect>&& rects, sdl::texture&& texture)
		: m_rects(std::move(rects)), m_texture(std::move(texture)) {
		auto [px, a, w, h] = m_texture.query();
		m_dims.x = 0;
		m_dims.y = 0;
		m_dims.w = static_cast <int>(w);
		m_dims.h = static_cast <int>(h);
	}
}
