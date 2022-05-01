//
// Created by igor on 28/04/2022.
// https://github.com/pvigier/Quadtree/blob/master/include/Quadtree.h
//

#ifndef INCLUDE_NEUTRINO_MATH_QUAD_TREE_HH
#define INCLUDE_NEUTRINO_MATH_QUAD_TREE_HH

#include <cassert>
#include <algorithm>
#include <array>
#include <memory>
#include <type_traits>
#include <vector>
#include <neutrino/math/rect.hh>

namespace neutrino::math {
  template<typename T, typename Getbox2d, typename Equal = std::equal_to<T>, typename Float = float>
  class quad_tree
  {
    public:
      using box_t = box2d<Float>;
    private:
      static_assert(std::is_convertible_v<std::invoke_result_t<Getbox2d, const T&>, box_t>,
      "Getbox2d must be a callable of signature box2d<Float>(const T&)");
      static_assert(std::is_convertible_v<std::invoke_result_t<Equal, const T&, const T&>, bool>,
                    "Equal must be a callable of signature bool(const T&, const T&)");
      static_assert(std::is_arithmetic_v<Float>);
      using vector2_t = typename box_t ::point_t ;

    public:
      explicit quad_tree(const box_t& box, const Getbox2d& getbox2d = Getbox2d(),
                const Equal& equal = Equal()) :
          mbox2d(box), mRoot(std::make_unique<Node>()), mGetbox2d(getbox2d), mEqual(equal)
      {

      }

      void add(const T& value)
      {
        add(mRoot.get(), 0, mbox2d, value);
      }

      void remove(const T& value)
      {
        remove(mRoot.get(), mbox2d, value);
      }

      std::vector<T> query(const box_t& box) const
      {
        auto values = std::vector<T>();
        query(mRoot.get(), mbox2d, box, values);
        return values;
      }

      std::vector<std::pair<T, T>> find_all_intersections() const
      {
        auto intersections = std::vector<std::pair<T, T>>();
        all_intersections (mRoot.get (), intersections);
        return intersections;
      }

    private:
      static constexpr auto Threshold = std::size_t(16);
      static constexpr auto MaxDepth = std::size_t(8);

      struct Node
      {
        std::array<std::unique_ptr<Node>, 4> children;
        std::vector<T> values;
      };

      box_t mbox2d;
      std::unique_ptr<Node> mRoot;
      Getbox2d mGetbox2d;
      Equal mEqual;

      bool is_leaf(const Node* node) const
      {
        return !static_cast<bool>(node->children[0]);
      }

      box_t computebox2d(const box_t& box, int i) const
      {
        const auto origin = box.top_left();
        const auto child_size = box.size() / static_cast<Float>(2);
        switch (i)
        {
          // North West
          case 0:
            return box_t(origin, child_size);
            // Norst East
          case 1:
            return box_t(vector2_t(origin.x + child_size.x, origin.y), child_size);
            // South West
          case 2:
            return box_t(vector2_t(origin.x, origin.y + child_size.y), child_size);
            // South East
          case 3:
            return box_t(origin + child_size, child_size);
          default:
            assert(false && "Invalid child index");
            return box_t();
        }
      }

      int get_quadrant(const box_t& nodebox2d, const box_t& valuebox2d) const
      {
        auto center = nodebox2d.center();
        // West
        if (valuebox2d.right() < center.x)
        {
          // North West
          if (valuebox2d.bottom() < center.y)
            return 0;
            // South West
          else if (valuebox2d.top() >= center.y)
            return 2;
            // Not contained in any quadrant
          else
            return -1;
        }
          // East
        else if (valuebox2d.left() >= center.x)
        {
          // North East
          if (valuebox2d.bottom() < center.y)
            return 1;
            // South East
          else if (valuebox2d.top() >= center.y)
            return 3;
            // Not contained in any quadrant
          else
            return -1;
        }
          // Not contained in any quadrant
        else
          return -1;
      }

      void add(Node* node, std::size_t depth, const box_t& box, const T& value)
      {
        assert(node != nullptr);
        assert(box.contains (mGetbox2d (value)));

        if (is_leaf (node))
        {
          // Insert the value in this node if possible
          if (depth >= MaxDepth || node->values.size() < Threshold)
            node->values.push_back(value);
            // Otherwise, we split and we try again
          else
          {
            split(node, box);
            add(node, depth, box, value);
          }
        }
        else
        {
          auto i = get_quadrant (box, mGetbox2d (value));
          // Add the value in a child if the value is entirely contained in it
          if (i != -1)
            add(node->children[static_cast<std::size_t>(i)].get(), depth + 1, computebox2d(box, i), value);
            // Otherwise, we add the value in the current node
          else
            node->values.push_back(value);
        }
      }

      void split(Node* node, const box_t& box)
      {
        assert(node != nullptr);
        assert(is_leaf (node) && "Only leaves can be split");
        // Create children
        for (auto& child : node->children)
          child = std::make_unique<Node>();
        // Assign values to children
        auto newValues = std::vector<T>(); // New values for this node
        for (const auto& value : node->values)
        {
          auto i = get_quadrant (box, mGetbox2d (value));
          if (i != -1)
            node->children[static_cast<std::size_t>(i)]->values.push_back(value);
          else
            newValues.push_back(value);
        }
        node->values = std::move(newValues);
      }

      bool remove(Node* node, const box_t& box, const T& value)
      {
        assert(node != nullptr);
        assert(box.contains(mGetbox2d(value)));
        if (is_leaf (node))
        {
          // Remove the value from node
          remove_value (node, value);
          return true;
        }
        else
        {
          // Remove the value in a child if the value is entirely contained in it
          auto i = get_quadrant (box, mGetbox2d (value));
          if (i != -1)
          {
            if (remove(node->children[static_cast<std::size_t>(i)].get(), computebox2d(box, i), value))
              return try_merge (node);
          }
            // Otherwise, we remove the value from the current node
          else
            remove_value (node, value);
          return false;
        }
      }

      void remove_value(Node* node, const T& value)
      {
        // Find the value in node->values
        auto it = std::find_if(std::begin(node->values), std::end(node->values),
                               [this, &value](const auto& rhs){ return mEqual(value, rhs); });
        assert(it != std::end(node->values) && "Trying to remove a value that is not present in the node");
        // Swap with the last element and pop back
        *it = std::move(node->values.back());
        node->values.pop_back();
      }

      bool try_merge(Node* node)
      {
        assert(node != nullptr);
        assert(!is_leaf (node) && "Only interior nodes can be merged");
        auto nbValues = node->values.size();
        for (const auto& child : node->children)
        {
          if (!is_leaf (child.get ()))
            return false;
          nbValues += child->values.size();
        }
        if (nbValues <= Threshold)
        {
          node->values.reserve(nbValues);
          // Merge the values of all the children
          for (const auto& child : node->children)
          {
            for (const auto& value : child->values)
              node->values.push_back(value);
          }
          // Remove the children
          for (auto& child : node->children)
            child.reset();
          return true;
        }
        else
          return false;
      }

      void query(Node* node, const box_t& box, const box_t& querybox2d, std::vector<T>& values) const
      {
        assert(node != nullptr);
        assert(querybox2d.intersects(box));
        for (const auto& value : node->values)
        {
          if (querybox2d.intersects(mGetbox2d(value)))
            values.push_back(value);
        }
        if (!is_leaf (node))
        {
          for (auto i = std::size_t(0); i < node->children.size(); ++i)
          {
            auto childbox2d = computebox2d(box, static_cast<int>(i));
            if (querybox2d.intersects(childbox2d))
              query(node->children[i].get(), childbox2d, querybox2d, values);
          }
        }
      }

      void all_intersections(Node* node, std::vector<std::pair<T, T>>& intersections) const
      {
        // Find intersections between values stored in this node
        // Make sure to not report the same intersection twice
        for (auto i = std::size_t(0); i < node->values.size(); ++i)
        {
          for (auto j = std::size_t(0); j < i; ++j)
          {
            if (mGetbox2d(node->values[i]).intersects(mGetbox2d(node->values[j])))
              intersections.emplace_back(node->values[i], node->values[j]);
          }
        }
        if (!is_leaf (node))
        {
          // Values in this node can intersect values in descendants
          for (const auto& child : node->children)
          {
            for (const auto& value : node->values)
              find_intersections_in_descendants (child.get (), value, intersections);
          }
          // Find intersections in children
          for (const auto& child : node->children)
            all_intersections (child.get (), intersections);
        }
      }

      void find_intersections_in_descendants(Node* node, const T& value, std::vector<std::pair<T, T>>& intersections) const
      {
        // Test against the values stored in this node
        for (const auto& other : node->values)
        {
          if (mGetbox2d(value).intersects(mGetbox2d(other)))
            intersections.emplace_back(value, other);
        }
        // Test against values stored into descendants of this node
        if (!is_leaf (node))
        {
          for (const auto& child : node->children)
            find_intersections_in_descendants (child.get (), value, intersections);
        }
      }
  };
}

#endif //INCLUDE_NEUTRINO_MATH_QUAD_TREE_HH
