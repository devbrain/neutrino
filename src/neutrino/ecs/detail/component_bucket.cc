//
// Created by igor on 7/22/24.
//
#include <cstdlib>
#include <neutrino/ecs/detail/component_bucket.hh>
#include <bsw/exception.hh>
namespace neutrino::ecs::detail {
	component_bucket::component_bucket(std::size_t alignment, std::size_t size_of_element, uint16_t count)
		: m_storage(static_cast <char*>(std::aligned_alloc(alignment, size_of_element * count))),
		  m_size_of_element(size_of_element),
		  m_capacity(count),
		  m_size(0) {
	}

	component_bucket::~component_bucket() {
		std::free(m_storage);
	}

	std::tuple<uint16_t, char*> component_bucket::get_free() const {
		if (m_size < m_capacity) {
			return {m_size, m_storage + m_size*m_size_of_element};
		}
		return {0, nullptr};
	}

	void component_bucket::mark_occupied(uint16_t name, uint16_t index) {
		m_descriptors.insert({name, index});
	}

	void component_bucket::mark_free(uint16_t name) {
		auto idx = m_descriptors.index_of({name, 0});

		auto& d = m_descriptors.get(idx);
	}
}
