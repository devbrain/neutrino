#pragma once

#include <neutrino/modules/physics/detail/quad_tree/quad_tree_generic.hh>

namespace neutrino::ecs::physics {

	template <typename T, typename GetBox, typename Equal = std::equal_to<T>, typename Float = float>
	class quad_tree {
		static_assert(std::is_convertible_v<std::invoke_result_t<GetBox, const T&>, Box<Float>>,
					  "GetBox must be a callable of signature Box<Float>(const T&)");

	 public:
		explicit quad_tree(const Box<Float>& box, const GetBox& getBox = GetBox(),
						   const Equal& equal = Equal())
			: m_tree(box, equal),
			  m_get_box(getBox) {
		}

		void add(const T& value) {
			m_tree.add(value, m_get_box);
		}

		void remove(const T& value) {
			m_tree.remove(value, m_get_box);
		}

		template <class Callback>
		void query(const Box<Float>& box, const Callback& cbk) const {
			m_tree.query(box, cbk, m_get_box);
		}

		std::vector<T> query(const Box<Float>& box) const {
			return m_tree.query(box, m_get_box);
		}

		template <class Callback>
		void find_all_intersections(const Callback& cbk) const {
			m_tree.find_all_intersections(cbk, m_get_box);
		}

		std::vector<std::pair<T, T>> find_all_intersections() const {
			return m_tree.find_all_intersections(m_get_box);
		}

		Box<Float> get_box() const noexcept {
			return m_tree.get_box();
		}

	 private:
		quad_tree_generic<T, Equal, Float> m_tree;
		GetBox m_get_box;

	};

}
