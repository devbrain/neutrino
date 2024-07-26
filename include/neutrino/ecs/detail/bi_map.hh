//
// Created by igor on 7/23/24.
//

#ifndef NEUTRINO_ECS_DETAIL_BI_MAP_HH
#define NEUTRINO_ECS_DETAIL_BI_MAP_HH

#include <neutrino/ecs/detail/sorted_array.hh>

namespace neutrino::ecs::detail {
	template<typename K, typename V>
	struct key_to_val {
		explicit key_to_val(const K& key)
			: key(key),
			  val{} {
		}

		key_to_val(const K& key, const V& val)
			: key(key),
			  val(val) {
		}

		K key;
		V val;
	};

	template<typename K, typename V>
	bool operator <(const key_to_val <K, V>& a, const key_to_val <K, V>& b) {
		return a.key < b.key;
	}

	template<typename K, typename V>
	bool operator ==(const key_to_val <K, V>& a, const key_to_val <K, V>& b) {
		return a.key == b.key;
	}

	template<typename K, typename V>
	class bi_map {
		public:
			using kv_t = key_to_val <K, V>;
			using vk_t = key_to_val <V, K>;
		public:
			bi_map() = default;

			explicit bi_map(std::size_t capacity)
				: m_keys(capacity),
				  m_vals(capacity) {
			}

			void bind(const K& key, const V& v) {
				m_keys.insert(kv_t(key, v));
				m_vals.insert(vk_t(v, key));
			}

			[[nodiscard]] V get_value_by_key(const K& key) const {
				auto idx = m_keys.index_of(kv_t(key));
				ENFORCE(m_keys.is_valid_index(idx));
				return m_keys.get(idx).val;
			}

			[[nodiscard]] bool get_value_by_key(const K& key, V& out) const {
				auto idx = m_keys.index_of(kv_t(key));
				if (m_keys.is_valid_index(idx)) {
					out = m_keys.get(idx).val;
					return true;
				}
				return false;
			}

			[[nodiscard]] K get_key_by_value(const V& v) const {
				auto idx = m_vals.index_of(vk_t(v));
				ENFORCE(m_vals.is_valid_index(idx));
				return m_vals.get(idx).val;
			}

			[[nodiscard]] bool get_key_by_value(const V& v, K& out) const {
				auto idx = m_vals.index_of(vk_t(v));
				if (m_vals.is_valid_index(idx)) {
					out = m_vals.get(idx).val;
					return true;
				}
				return false;
			}

			[[nodiscard]] bool key_exists(const K& key) const {
				auto idx = m_keys.index_of(kv_t(key));
				return m_keys.is_valid_index(idx);
			}

			[[nodiscard]] bool value_exists(const V& v) const {
				auto idx = m_vals.index_of(vk_t(v));
				return m_vals.is_valid_index(idx);
			}

			bool remove_by_key(const K& k, kv_t* removed = nullptr) {
				kv_t old(k);
				if (m_keys.remove(old, &old)) {
					if (removed) {
						*removed = old;
					}
					m_vals.remove(vk_t(old.val));

					return true;
				}
				return false;
			}

			bool remove_by_value(const V& v, vk_t* removed = nullptr) {
				vk_t old(v);
				if (m_vals.remove(old, &old)) {
					if (removed) {
						*removed = old;
					}
					m_keys.remove(kv_t(old.val));
					return true;
				}
				return false;
			}

			[[nodiscard]] std::size_t size() const {
				ENFORCE(m_keys.size() == m_vals.size());
				return m_keys.size();
			}

			[[nodiscard]] bool empty() const {
				ENFORCE(m_keys.empty() == m_vals.empty());
				return m_keys.empty();
			}

		private:
			sorted_array <kv_t> m_keys;
			sorted_array <vk_t> m_vals;
	};
}

#endif
