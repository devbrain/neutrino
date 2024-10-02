#pragma once

#include <libassert/assert.hpp>
#include <algorithm>
#include <array>
#include <memory>
#include <type_traits>
#include <vector>
#include <sdlpp/video/geometry.hh>

namespace neutrino::ecs::physics {
	template <typename T>
	using Box = sdl::generic::rect<T>;

	template <typename T, typename Equal = std::equal_to<T>, typename Float = float>
	class quad_tree_generic {
		static_assert(std::is_convertible_v<std::invoke_result_t<Equal, const T&, const T&>, bool>,
					  "Equal must be a callable of signature bool(const T&, const T&)");
		static_assert(std::is_arithmetic_v<Float>);

	 public:
		explicit quad_tree_generic(const Box<Float>& box, const Equal& equal = Equal())
			: m_box(box),
			  m_root(std::make_unique<Node>()),
			  m_equal(equal) {
		}

		template <typename GetBoxFn>
		void add(const T& value, const GetBoxFn& getBoxFn) {
			add(m_root.get(), 0, m_box, value, getBoxFn);
		}

		template <typename GetBoxFn>
		void remove(const T& value, const GetBoxFn& getBoxFn) {
			remove(m_root.get(), m_box, value, getBoxFn);
		}

		template <class Callback, typename GetBoxFn>
		void query(const Box<Float>& box, const Callback& cbk, const GetBoxFn& getBoxFn) const {
			query(m_root.get(), m_box, box, cbk, getBoxFn);
		}

		template <typename GetBoxFn>
		std::vector<T> query(const Box<Float>& box, const GetBoxFn& getBoxFn) const {
			auto values = std::vector<T>();
			query(m_root.get(), m_box, box, [&values](const auto& i) {
				values.emplace_back(i);
			}, getBoxFn);
			return values;
		}

		template <class Callback, class GetBoxFn>
		void find_all_intersections(const Callback& cbk, const GetBoxFn& getBoxFn) const {
			find_all_intersections(m_root.get(), cbk, getBoxFn);
		}

		template <typename GetBoxFn>
		std::vector<std::pair<T, T>> find_all_intersections(const GetBoxFn& getBoxFn) const {
			auto intersections = std::vector<std::pair<T, T>>();
			find_all_intersections(m_root.get(), [&intersections](const auto& i, const auto& j){
			  intersections.emplace_back(i, j);
			}, getBoxFn);
			return intersections;
		}

		Box<Float> get_box() const noexcept {
			return m_box;
		}

	 private:
		static constexpr auto Threshold = std::size_t(16);
		static constexpr auto MaxDepth = std::size_t(8);

		struct Node {
			std::array<std::unique_ptr<Node>, 4> children;
			std::vector<T> values;
		};

		Box<Float> m_box;
		std::unique_ptr<Node> m_root;
		Equal m_equal;

		bool is_leaf(const Node* node) const {
			return !static_cast<bool>(node->children[0]);
		}

		Box<Float> compute_box(const Box<Float>& box, int i) const {
			auto origin = box.offset();
			auto childSize = box.area() / static_cast<Float>(2);
			switch (i) {
				// North West
				case 0:return Box<Float>(origin, childSize);
					// Norst East
				case 1:return Box<Float>(math::vector<Float, 2>(origin.x + childSize.w, origin.y), childSize);
					// South West
				case 2:return Box<Float>(math::vector<Float, 2>(origin.x, origin.y + childSize.h), childSize);
					// South East
				case 3:
					return Box<Float>(math::vector<Float, 2>(
						origin.x + childSize.w, origin.y + childSize.h), childSize);
				default:UNREACHABLE("Invalid child index");
					return Box<Float>();
			}
		}

		int get_quadrant(const Box<Float>& nodeBox, const Box<Float>& valueBox) const {
			auto center = nodeBox.center();
			// West
			if (valueBox.right() < center.x) {
				// North West
				if (valueBox.bottom() < center.y)
					return 0;
					// South West
				else if (valueBox.top() >= center.y)
					return 2;
					// Not contained in any quadrant
				else
					return -1;
			}
				// East
			else if (valueBox.left() >= center.x) {
				// North East
				if (valueBox.bottom() < center.y)
					return 1;
					// South East
				else if (valueBox.top() >= center.y)
					return 3;
					// Not contained in any quadrant
				else
					return -1;
			}
				// Not contained in any quadrant
			else
				return -1;
		}

		template <typename GetBoxFn>
		void add(Node* node, std::size_t depth, const Box<Float>& box, const T& value, const GetBoxFn& getBoxFn) {
			DEBUG_ASSERT(node != nullptr);
			DEBUG_ASSERT(box.contains(getBoxFn(value)));
			if (is_leaf(node)) {
				// Insert the value in this node if possible
				if (depth >= MaxDepth || node->values.size() < Threshold) {
					node->values.push_back(value);
					// Otherwise, we split and we try again
				} else {
					split(node, box, getBoxFn);
					add(node, depth, box, value, getBoxFn);
				}
			} else {
				auto i = get_quadrant(box, getBoxFn(value));
				// Add the value in a child if the value is entirely contained in it
				if (i != -1) {
					add(node->children[static_cast<std::size_t>(i)].get(), depth + 1, compute_box(box, i), value, getBoxFn);
					// Otherwise, we add the value in the current node
				} else {
					node->values.push_back(value);
				}
			}
		}

		template <typename GetBoxFn>
		void split(Node* node, const Box<Float>& box, const GetBoxFn& getBoxFn) {
			DEBUG_ASSERT(node != nullptr);
			DEBUG_ASSERT(is_leaf(node) && "Only leaves can be split");
			// Create children
			for (auto& child : node->children)
				child = std::make_unique<Node>();
			// Assign values to children
			auto newValues = std::vector<T>(); // New values for this node
			for (const auto& value : node->values) {
				auto i = get_quadrant(box, getBoxFn(value));
				if (i != -1) {
					node->children[static_cast<std::size_t>(i)]->values.push_back(value);
				} else {
					newValues.push_back(value);
				}
			}
			node->values = std::move(newValues);
		}

		template <typename GetBoxFn>
		bool remove(Node* node, const Box<Float>& box, const T& value, const GetBoxFn& getBoxFn) {
			DEBUG_ASSERT(node != nullptr);
			DEBUG_ASSERT(box.contains(getBoxFn(value)));
			if (is_leaf(node)) {
				// Remove the value from node
				remove_value(node, value);
				return true;
			} else {
				// Remove the value in a child if the value is entirely contained in it
				auto i = get_quadrant(box, getBoxFn(value));
				if (i != -1) {
					if (remove(node->children[static_cast<std::size_t>(i)].get(), compute_box(box, i), value, getBoxFn))
						return try_merge(node);
				}
					// Otherwise, we remove the value from the current node
				else {
					remove_value(node, value);
				}
				return false;
			}
		}

		void remove_value(Node* node, const T& value) {
			// Find the value in node->values
			auto it = std::find_if(std::begin(node->values), std::end(node->values),
								   [this, &value](const auto& rhs) { return m_equal(value, rhs); });
			ASSERT(it != std::end(node->values), "Trying to remove a value that is not present in the node");
			// Swap with the last element and pop back
			*it = std::move(node->values.back());
			node->values.pop_back();
		}

		bool try_merge(Node* node) {
			DEBUG_ASSERT(node != nullptr);
			DEBUG_ASSERT(!is_leaf(node) && "Only interior nodes can be merged");
			auto nbValues = node->values.size();
			for (const auto& child : node->children) {
				if (!is_leaf(child.get()))
					return false;
				nbValues += child->values.size();
			}
			if (nbValues <= Threshold) {
				node->values.reserve(nbValues);
				// Merge the values of all the children
				for (const auto& child : node->children) {
					for (const auto& value : child->values)
						node->values.push_back(value);
				}
				// Remove the children
				for (auto& child : node->children) {
					child.reset();
				}
				return true;
			} else {
				return false;
			}
		}

		template <typename Callable, typename GetBoxFn>
		void query(Node* node, const Box<Float>& box, const Box<Float>& queryBox, const Callable& cbk, const GetBoxFn& getBoxFn) const {
			DEBUG_ASSERT(node != nullptr);
			DEBUG_ASSERT(queryBox.intersects(box));
			for (const auto& value : node->values) {
				if (queryBox.intersects(getBoxFn(value))) {
					cbk(value);
				}
			}
			if (!is_leaf(node)) {
				for (auto i = std::size_t(0); i < node->children.size(); ++i) {
					auto childBox = compute_box(box, static_cast<int>(i));
					if (queryBox.intersects(childBox)) {
						query(node->children[i].get(), childBox, queryBox, cbk, getBoxFn);
					}
				}
			}
		}

		template <class Callback, typename GetBoxFn>
		void find_all_intersections(Node* node, const Callback& cbk, const GetBoxFn& getBoxFn) const {
			// Find intersections between values stored in this node
			// Make sure to not report the same intersection twice
			for (auto i = std::size_t(0); i < node->values.size(); ++i) {
				for (auto j = std::size_t(0); j < i; ++j) {
					if (getBoxFn(node->values[i]).intersects(getBoxFn(node->values[j])))
						cbk(node->values[i], node->values[j]);
				}
			}
			if (!is_leaf(node)) {
				// Values in this node can intersect values in descendants
				for (const auto& child : node->children) {
					for (const auto& value : node->values) {
						find_intersections_in_descendants(child.get(), value, cbk, getBoxFn);
					}
				}
				// Find intersections in children
				for (const auto& child : node->children) {
					find_all_intersections(child.get(), cbk, getBoxFn);
				}
			}
		}

		template <typename Callback, typename GetBoxFn>
		void find_intersections_in_descendants(Node* node, const T& value, const Callback& cbk, const GetBoxFn& getBoxFn) const {
			// Test against the values stored in this node
			for (const auto& other : node->values) {
				if (getBoxFn(value).intersects(getBoxFn(other)))
					cbk(value, other);
			}
			// Test against values stored into descendants of this node
			if (!is_leaf(node)) {
				for (const auto& child : node->children) {
					find_intersections_in_descendants(child.get(), value, cbk, getBoxFn);
				}
			}
		}
	};

}
