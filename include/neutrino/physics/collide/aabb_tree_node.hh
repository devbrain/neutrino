//
// Created by igor on 20/06/2026.
//

/**
 * @file aabb_tree_node.hh
 * @brief The node, node handle, and entity-id types of the dynamic AABB tree (@ref
 *        neutrino::physics::dynamic_aabb_tree).
 *
 * Defines @ref neutrino::physics::node_ptr (a strongly-typed index into the node arena), @ref
 * neutrino::physics::dynamic_aabb_node (one tree node -- leaf or internal), and the @ref
 * neutrino::physics::FREE_NODE_HEIGHT sentinel. Nodes are addressed by @c node_ptr rather than raw
 * pointer because the backing arena (@ref aabb_tree_storage.hh) reallocates on growth.
 */

#pragma once

#include <cstdint>
#include <neutrino/physics/geometry/types.hh>

namespace neutrino::physics {
    /// @brief The game entity id carried by a tree leaf (an opaque payload, echoed back by queries).
    using entity_id_t = uint32_t;

    /// @brief Height sentinel stamped on a slot while it sits on the storage free list.
    /// @note A live leaf is height 0 and a live internal node is >= 1, so @c -1 is unambiguous --
    ///       letting the storage detect a double free and queries reject stale handles.
    inline constexpr int16_t FREE_NODE_HEIGHT = -1;

    /**
     * @brief A strongly-typed index ("handle") into the AABB tree's node arena.
     *
     * Used instead of a raw @c dynamic_aabb_node* because the arena's backing vector reallocates on
     * growth -- a @c node_ptr stays valid across that, a pointer would dangle. A default-constructed
     * handle is the @ref INVALID_NODE null sentinel and tests @c false via @c operator bool.
     */
    struct node_ptr {
        using value_t = int32_t;                        ///< Underlying index type.
        static constexpr value_t INVALID_NODE = -1;     ///< Null-handle sentinel.
        value_t value;

        /// @brief Construct the null handle (@ref INVALID_NODE).
        constexpr node_ptr()
            : value(INVALID_NODE) {
        }

        /// @brief Construct a handle wrapping raw arena index @p x.
        constexpr explicit node_ptr(int32_t x)
            : value(x) {
        }

        node_ptr& operator =(const node_ptr& x) = default;
        node_ptr(const node_ptr& x) = default;

        /// @brief @c true iff this is not the null handle.
        constexpr explicit operator bool() const noexcept {
            return value != INVALID_NODE;
        }

        /// @brief Handle equality (same underlying index).
        constexpr bool operator==(const node_ptr& x) const noexcept {
            return value == x.value;
        }

        /// @brief Handle inequality.
        constexpr bool operator!=(const node_ptr& x) const noexcept {
            return !(*this == x);
        }

        /// @brief Unwrap to the raw arena index (for @c aabb_tree_storage::operator[]).
        constexpr value_t operator*() const noexcept {
            return value;
        }
    };

    /**
     * @brief One node of the dynamic AABB tree -- a leaf (holding an entity) or an internal node
     *        (holding two children whose boxes it encloses).
     *
     * The tree is a full binary tree: every node has either 0 children (a leaf) or exactly 2. The
     * @c entity_id / @c next union saves space -- @c entity_id is meaningful only for a live leaf,
     * while @c next threads the free list only for a slot on the arena's free list.
     */
    struct dynamic_aabb_node {
        aabb box;             ///< Bounds: a leaf's stored (possibly fat) box, or the union of its children.
        node_ptr parent;      ///< Parent handle (null for the root).
        node_ptr left, right; ///< Child handles; both null for a leaf.
        union {
            entity_id_t  entity_id; ///< Payload when a live leaf.
            node_ptr next;          ///< Free-list link when the slot is free.
        };
        int16_t height;       ///< 0 for a leaf, 1+max(children) internally, @ref FREE_NODE_HEIGHT when free.

        /// @brief Construct a zero-initialised leaf node (height 0, entity 0).
        dynamic_aabb_node ()
            : entity_id(0), height(0) {
        }
    };

    /**
     * @brief Is @p n a leaf (no children)?
     * @param n The node to test.
     * @return @c true iff @p n has no children. Relies on the full-binary invariant: a node has
     *         either both children set or neither.
     */
    constexpr bool is_leaf(const dynamic_aabb_node& n) {
        return !(n.right && n.left);
    }
}
