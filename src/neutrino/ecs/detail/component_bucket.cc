//
// Created by igor on 7/22/24.
//
#include <cstdlib>
#include <neutrino/ecs/detail/component_bucket.hh>
#include <bsw/exception.hh>

namespace neutrino::ecs::detail {
	component_bucket_iterator::component_bucket_iterator(const component_bucket& owner)
		: m_owner(owner),
		  m_cur_block(0),
		  m_free_pos(owner.m_capacity),
		  m_free_idx(0),
		  m_blocks_seen(0),
	      m_last_taken(0) {
		if (!m_owner.m_free.empty()) {
			m_free_pos = m_owner.m_free.get(m_free_idx);
		}
	}

	bool component_bucket_iterator::has_next() const {
		return m_blocks_seen < m_owner.size();
	}

	std::tuple<char*, std::size_t> component_bucket_iterator::next() {
		if (m_cur_block == m_free_pos) {
			while (true) {
				m_free_idx++;
				m_cur_block++;
				if (m_free_idx >= m_owner.m_free.size()) {
					break;
				}
				auto next_free = m_owner.m_free.get(m_free_idx);
				if (next_free - m_free_pos == 1) {
					m_free_pos = next_free;
				} else {
					m_free_pos = next_free;
					break;
				}
			}
		}
		m_last_taken = m_cur_block;
		char* out = m_owner.m_storage + m_cur_block * m_owner.m_size_of_element;
		m_blocks_seen++;
		m_cur_block++;
		return {out, m_last_taken};
	}

	component_bucket::key_t component_bucket_iterator::get_key() const {
		return m_owner.get_key_by_index(m_last_taken);
	}

	component_bucket::component_bucket(std::size_t alignment, std::size_t size_of_element, uint16_t count,
		void (*destructor) (component_bucket& bucket, entity_id_t entity_id))
		: m_storage(static_cast <char*>(std::aligned_alloc(alignment, size_of_element * count))),
		  m_size_of_element(size_of_element),
		  m_capacity(count),
		  m_names_map(count),
		  m_free(0, count),
		  m_destructor(destructor) {
	}

	component_bucket::~component_bucket() {
		std::free(m_storage);
	}

	std::tuple <component_bucket::index_t, char*> component_bucket::get_free() const {
		if (m_free.empty()) {
			return {0, nullptr};
		}
		auto idx = m_free.pop_back();
		return {idx, m_storage + idx * m_size_of_element};
	}

	char* component_bucket::get_block_by_name(key_t name) const {
		index_t idx;
		if (m_names_map.get_value_by_key(name, idx)) {
			return m_storage + idx * m_size_of_element;
		}
		return nullptr;
	}

	component_bucket::key_t component_bucket::get_key_by_index(index_t idx) const {
		return m_names_map.get_key_by_value(idx);
	}

	void component_bucket::mark_occupied(key_t name, index_t index) {
		m_names_map.bind(name, index);
	}

	void component_bucket::mark_free(uint16_t name) {
		names_map_t::kv_t removed(0, 0);
		if (m_names_map.remove_by_key(name, &removed)) {
			m_free.insert(removed.val);
		} else {
			RAISE_EX("Name ", name, " does not exists");
		}
	}

	std::size_t component_bucket::capacity() const {
		return m_capacity;
	}

	std::size_t component_bucket::size() const {
		return m_capacity - m_free.size();
	}

	void component_bucket::destruct(entity_id_t entity_id) {
		if (m_destructor) {
			m_destructor(*this, entity_id);
		}
	}
}
