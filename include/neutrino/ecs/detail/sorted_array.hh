//
// Created by igor on 7/22/24.
//

#ifndef NEUTRINO_ECS_DETAIL_SORTED_ARRAY_HH
#define NEUTRINO_ECS_DETAIL_SORTED_ARRAY_HH

#include <vector>
#include <array>
#include <algorithm>
#include <bsw/exception.hh>

namespace neutrino::ecs::detail {
	template<typename T>
	class sorted_array {
		public:
			sorted_array () = default;
			explicit sorted_array (std::size_t capacity) {
				m_array.reserve(capacity);
			}
			sorted_array(const T& min_val, const T& max_val) {
				for (T i=min_val; i != max_val; i++) {
					m_array.emplace_back(i);
				}
			}
			void insert(const T& value) {
				m_array.insert(std::upper_bound(m_array.begin(), m_array.end(), value), value);
			}

			[[nodiscard]] std::size_t size() const {
				return m_array.size();
			}

			[[nodiscard]] bool empty() const {
				return m_array.empty();
			}

			[[nodiscard]] bool exists(const T& value) const {
				return std::binary_search(m_array.begin(), m_array.end(), value);
			}

			bool remove(const T& value, T* old_val = nullptr) {
				auto low = std::lower_bound(m_array.begin(), m_array.end(), value);
				if (low == m_array.end() || !(*low == value)) {
					return false;
				}
				if (old_val) {
					*old_val = *low;
				}
				m_array.erase(low);
				return true;
			}

			void remove_by_index(std::size_t idx) {
				ENFORCE(idx < m_array.size());
				m_array.erase(m_array.begin() + idx);
			}

			[[nodiscard]] std::size_t index_of(const T& value) const {
				// index of the first element, greater than or equal to value
				auto low = std::lower_bound(m_array.begin(), m_array.end(), value);
				if (low == m_array.end()) {
					return m_array.size();
				}
				if (!(*low == value)) {
					return m_array.size();
				}
				return low - m_array.begin();
			}

			[[nodiscard]] bool is_valid_index(std::size_t index) const {
				return index < m_array.size();
			}

			[[nodiscard]] const T& get(std::size_t index) const {
				return m_array[index];
			}

			[[nodiscard]] T& get(std::size_t index) {
				return m_array[index];
			}

			T pop_back() {
				ENFORCE(!empty());
				T out = m_array.back();
				m_array.pop_back();
				return out;
			}
		private:
			using array_t = std::vector <T>;
			array_t m_array;
	};

	template<typename T, std::size_t N>
	class fixed_sorted_array {
		public:
			fixed_sorted_array()
				: m_size(0) {

			}

			bool insert(const T& value) {
				if (m_size < N) {
					const auto index = std::upper_bound(m_array.begin(), m_array.begin() + m_size, value) - m_array.begin();
					for (size_t i = N; i > index; --i) {
						m_array[i] = m_array[i - 1];
					}
					m_array[index] = value;
					m_size++;
					return true;
				}
				return false;
			}

			[[nodiscard]] std::size_t size() const {
				return m_size;
			}

			[[nodiscard]] bool empty() const {
				return m_size == 0;
			}

			[[nodiscard]] bool exists(const T& value) const {
				return std::binary_search(m_array.begin(), m_array.begin() + m_size, value);
			}

			[[nodiscard]] std::size_t index_of(const T& value) const {
				// index of the first element, greater than or equal to value
				auto low = std::lower_bound(m_array.begin(), m_array.begin() + m_size, value);
				if (low == m_array.begin() + m_size) {
					return N;
				}
				if (*low != value) {
					return N;
				}
				return low - m_array.begin();
			}

			[[nodiscard]] const T& get(std::size_t index) const {
				return m_array[index];
			}

			[[nodiscard]] T& get(std::size_t index) {
				return m_array[index];
			}

			[[nodiscard]] bool is_valid_index(std::size_t index) const {
				return index < m_size;
			}
		private:
			using array_t = std::array <T, N+1>;
			array_t m_array;
			std::size_t m_size;
	};
}

#endif
