//
// Created by igor on 7/22/24.
//

#ifndef COMPONENT_BUCKET_HH
#define COMPONENT_BUCKET_HH

#include <cstdint>
#include <tuple>
#include <neutrino/neutrino_export.hh>
#include <neutrino/ecs/types.hh>
#include <neutrino/ecs/detail/sorted_array.hh>

namespace neutrino::ecs::detail {
	struct entity_descriptor {
		uint16_t name;
		uint16_t index;
	};

	inline bool operator < (const entity_descriptor& a, const entity_descriptor& b) {
		return a.name < b.name;
	}

	/**
	 * REQUIREMENTS:
	 * Continous storage of blocks
	 * Find name of block
	 * Find block by name
	 */
	class NEUTRINO_EXPORT component_bucket {
		public:
			component_bucket(std::size_t alignment, std::size_t size_of_element, uint16_t count);
			~component_bucket();

			[[nodiscard]] std::tuple<uint16_t, char*> get_free() const;
			[[nodiscard]] char* get_block_by_name(uint16_t name) const;


			void mark_occupied(uint16_t name, uint16_t index);
			void mark_free(uint16_t name);

			void collect_garbage();
		private:
			char* m_storage;
			std::size_t m_size_of_element;
			std::size_t m_capacity;
			uint16_t m_size;
			sorted_array<entity_descriptor> m_descriptors;
	};
}

#endif
