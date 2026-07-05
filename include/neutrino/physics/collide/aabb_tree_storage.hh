//
// Created by igor on 20/06/2026.
//

/**
 * @file aabb_tree_storage.hh
 * @brief The pooled node arena (@ref neutrino::physics::aabb_tree_storage) backing the dynamic AABB
 *        tree.
 *
 * A slot-recycling arena of @ref neutrino::physics::dynamic_aabb_node, addressed by @ref
 * neutrino::physics::node_ptr. Freed slots are reused via an intrusive free list, and a freed slot
 * is stamped @ref neutrino::physics::FREE_NODE_HEIGHT to catch double frees.
 */

#pragma once

#include <cstddef>
#include <vector>

#include <neutrino/physics/collide/aabb_tree_node.hh>
#include <failsafe/enforce.hh>

namespace neutrino::physics {
    /**
     * @brief A pooled, index-addressed node arena for the dynamic AABB tree.
     *
     * Nodes live in one contiguous vector and are referenced by @ref node_ptr (a strongly-typed
     * index), never by raw pointer: the vector can reallocate on growth, so any reference obtained
     * from @c operator[] is valid only until the next @ref allocate. Callers hold @c node_ptr values
     * and re-dereference through @c operator[] freshly each time.
     *
     * Freed slots are recycled via an intrusive singly-linked free list threaded through the node's
     * @c next field (which aliases @c entity_id, unused while free); a freed slot is marked
     * @ref FREE_NODE_HEIGHT to catch double frees.
     */
    class aabb_tree_storage {
        public:
            /// @brief Construct an empty arena.
            aabb_tree_storage() = default;
            /// @brief Construct an empty arena, pre-reserving @p to_reserve slots.
            explicit aabb_tree_storage(std::size_t to_reserve);

            /**
             * @brief Allocate a node initialised as a leaf (no children, height 0).
             * @param eid The leaf's entity payload.
             * @param box The leaf's bounding box.
             * @return The new node's handle. The handle is stable for the node's lifetime; a
             *         reference from @c operator[] is NOT stable across @ref allocate.
             */
            node_ptr allocate(entity_id_t eid, const aabb& box);

            /**
             * @brief Return a slot to the free list.
             * @param index The node to free; a null handle is a no-op. Double-freeing aborts.
             */
            void deallocate(node_ptr index);

            /**
             * @brief Index -> node access.
             * @param index A live node handle.
             * @return A reference valid only until the next @ref allocate (the backing vector may
             *         reallocate).
             */
            dynamic_aabb_node& operator[](node_ptr index) { return m_pool[static_cast <std::size_t>(*index)]; }
            /// @copydoc operator[](node_ptr)
            const dynamic_aabb_node& operator[](node_ptr index) const { return m_pool[static_cast <std::size_t>(*index)]; }

            /// @brief Total number of slots (live + free) -- useful for reserve/validation.
            [[nodiscard]] std::size_t capacity_used() const { return m_pool.size(); }

            /// @brief Pre-grow the pool to @p n slots to avoid reallocations during a batch of inserts.
            void reserve(std::size_t n) { m_pool.reserve(n); }

            /**
             * @brief Drop every node and empty the free list -- the arena returns to its
             *        just-constructed state (capacity retained).
             * @note Any @ref node_ptr into the old contents is invalidated.
             */
            void clear() {
                m_pool.clear();
                m_free_head = node_ptr{};
            }

        private:
            /// @brief Initialise slot @p n as a fresh leaf with payload @p eid and bounds @p box.
            static void init(dynamic_aabb_node& n, entity_id_t eid, const aabb& box);

        private:
            std::vector <dynamic_aabb_node> m_pool; ///< The contiguous node arena.
            node_ptr m_free_head;                   ///< Head of the intrusive free list (null if none).
    };

    inline aabb_tree_storage::aabb_tree_storage(std::size_t to_reserve) {
        m_pool.reserve(to_reserve);
    }

    inline node_ptr aabb_tree_storage::allocate(entity_id_t eid, const aabb& box) {
        node_ptr index;
        if (m_free_head) {
            // Reuse a recycled slot: pop the head of the free list.
            index = m_free_head;
            m_free_head = m_pool[static_cast <std::size_t>(*index)].next;
        } else {
            // Grow the pool. emplace_back may reallocate; that is fine because we
            // resolve the reference (init below) only after the vector settles.
            index = static_cast <node_ptr>(m_pool.size());
            m_pool.emplace_back();
        }
        init(m_pool[static_cast <std::size_t>(*index)], eid, box);
        return index;
    }

    inline void aabb_tree_storage::deallocate(node_ptr index) {
        if (!index) {
            return;
        }
        ENFORCE(*index >= 0 && static_cast<std::size_t>(*index) < m_pool.size());
        dynamic_aabb_node& n = m_pool[static_cast <std::size_t>(*index)];
        ENFORCE(n.height != FREE_NODE_HEIGHT)("Double free of aabb_node"); // guard against double free
        n.height = FREE_NODE_HEIGHT; // mark slot as free
        n.next = m_free_head; // push onto the free list
        m_free_head = index;
    }

    inline void aabb_tree_storage::init(dynamic_aabb_node& n, entity_id_t eid, const aabb& box) {
        n.parent = node_ptr{};
        n.left = node_ptr{};
        n.right = node_ptr{};

        n.height = 0;
        n.entity_id = eid;
        n.box = box;
    }
}
