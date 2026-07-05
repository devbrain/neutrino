//
// Created by igor on 20/06/2026.
//

/**
 * @file dynamic_aabb_tree.hh
 * @brief A self-balancing dynamic AABB tree (bounding-volume hierarchy) -- the dynamic-body
 *        broadphase behind @ref neutrino::physics::world.
 *
 * Stores leaf AABBs in a binary BVH that is kept strictly AVL-balanced (heights of sibling
 * subtrees differ by at most 1) under incremental insert/remove/update, choosing each insertion
 * point by a surface-area-heuristic (SAH) cost. The tree is a free-function API over the @ref
 * neutrino::physics::dynamic_aabb_tree state struct:
 *   - @ref neutrino::physics::insert_leaf -- add an entity's box, returns a stable leaf handle.
 *   - @ref neutrino::physics::remove_leaf -- drop a leaf.
 *   - @ref neutrino::physics::update_leaf -- move a leaf's box, keeping the same handle (with a
 *     fat-AABB containment short-circuit).
 *   - @ref neutrino::physics::query -- enumerate leaves whose stored box overlaps a region.
 *   - @ref neutrino::physics::raycast -- enumerate leaves whose stored box a segment crosses,
 *     with optional closest-hit ray-clipping.
 *
 * @note Reported hits are broadphase CANDIDATES against each leaf's STORED box (which may be a
 *       fat-AABB margin larger than the true bounds); the caller narrow-phases against real
 *       geometry. The tree stores boxes verbatim -- any margin policy is applied by the caller
 *       (see @ref neutrino::physics::world::fatten).
 * @see aabb_tree_node.hh for the node/handle types, aabb_tree_storage.hh for the node arena.
 */

#pragma once

#include <algorithm>
#include <functional>
#include <type_traits>
#include <neutrino/physics/collide/aabb_tree_node.hh>
#include <neutrino/physics/collide/aabb_tree_storage.hh>
#include <neutrino/physics/geometry/overlap.hh>
#include <neutrino/physics/geometry/sweep.hh>

namespace neutrino::physics {
    /**
     * @brief The dynamic AABB tree state: the node arena plus the root handle.
     *
     * A plain aggregate operated on by the free functions in this header (@ref insert_leaf,
     * @ref remove_leaf, @ref update_leaf, @ref query, @ref raycast). Dereference node handles
     * through @c operator[].
     */
    struct dynamic_aabb_tree {
        aabb_tree_storage m_storage; ///< The pooled node arena.
        node_ptr m_root;             ///< Handle of the root node (null when the tree is empty).

        /// @brief Construct an empty tree.
        dynamic_aabb_tree() = default;

        /// @brief Dereference a node handle (const).
        const dynamic_aabb_node& operator [](node_ptr idx) const {
            return m_storage[idx];
        }

        /// @brief Dereference a node handle.
        dynamic_aabb_node& operator [](node_ptr idx) {
            return m_storage[idx];
        }

        /**
         * @brief Empty the tree: drop all leaves, clear the root, reset storage to its
         *        just-constructed state (capacity retained).
         *
         * Mirrors @ref grid::reset -- a cheap "rebuild from scratch" without reallocating a tree.
         * @note Any leaf @ref node_ptr handed out before the reset is invalidated.
         */
        void reset() {
            m_storage.clear();
            m_root = node_ptr{};
        }
    };

    namespace detail {
        /// @brief The union of node @p n's box with @p box.
        inline
        aabb combine(const dynamic_aabb_node& n, const aabb& box) {
            return aabb::combine(n.box, box);
        }

        /// @brief Surface area (SAH cost metric) of node @p n's box.
        inline
        float area(const dynamic_aabb_node& n) {
            return n.box.area();
        }

        /**
         * @brief SAH cost of routing a new box through @p child.
         * @param child   Candidate descent node.
         * @param new_box The box being inserted.
         * @return For a leaf, the area of the enlarged box; for an internal node, the area increase
         *         (enlarged minus current) -- the marginal cost of descending into it.
         */
        inline
        float get_cost_of_child(const dynamic_aabb_node& child, const aabb& new_box) {
            auto new_left_aabb = combine(child, new_box);

            if (is_leaf(child)) {
                return new_left_aabb.area();
            }
            auto old_area = area(child);
            return (new_left_aabb.area() - old_area);
        }

        /**
         * @brief Height of the subtree rooted at @p idx.
         * @param t   The tree.
         * @param idx Node handle; a null handle yields @c -1 (so a leaf computes height 0).
         */
        inline
        int16_t get_height(const dynamic_aabb_tree& t, node_ptr idx) {
            if (!idx) {
                return -1;
            }
            return t[idx].height;
        }

        /**
         * @brief Recompute internal node @p n's cached height and box from its two children.
         * @param t The tree.
         * @param n The internal node to refit (its children must be current).
         */
        inline void refit(const dynamic_aabb_tree& t, dynamic_aabb_node& n) {
            n.height = 1 + std::max(get_height(t, n.left), get_height(t, n.right));
            n.box = aabb::combine(t[n.left].box, t[n.right].box);
        }

        /**
         * @brief Is @p inner fully contained in @p outer (closed bounds)?
         * @param outer The enclosing box.
         * @param inner The candidate inner box.
         * @return @c true iff every edge of @p inner is within @p outer. Drives @ref update_leaf 's
         *         fat-AABB short-circuit.
         */
        inline bool contains(const aabb& outer, const aabb& inner) {
            return outer.min.x() <= inner.min.x() && outer.min.y() <= inner.min.y()
                   && inner.max.x() <= outer.max.x() && inner.max.y() <= outer.max.y();
        }
    }

    /**
     * @brief Find the SAH-best sibling to pair a new box with -- the leaf/subtree under which
     *        inserting @p new_box adds the least surface area.
     *
     * Greedily descends from the root, at each node comparing the cost of splitting here against the
     * inherited-cost-adjusted cost of descending into either child, and stops when splitting here
     * wins.
     *
     * @param t       The tree (non-empty).
     * @param new_box The box about to be inserted.
     * @return The chosen sibling node's handle.
     */
    inline
    node_ptr find_best_sibling(const dynamic_aabb_tree& t, const aabb& new_box) {
        auto itr = t.m_root;
        while (!is_leaf(t[itr])) {
            // 1. Cost of creating a new parent right here, merging with 'index'
            auto combined_aabb = detail::combine(t[itr], new_box);
            auto cost = combined_aabb.area();
            const auto& n = t[itr];
            //  Inherited cost multiplier used as you descend deeper into the hierarchy
            // (This discourages making deeply nested, overlapping strands)
            auto inheritance_cost = cost - detail::area(n);

            // Compute cost of descending into the Left Child.
            // Both children are dereferenced unconditionally: this relies on the
            // full-binary-tree invariant -- every node has either 0 children (a leaf)
            // or exactly 2, never 1. is_leaf() above only guarantees "at least one
            // child is set", so the unchecked t[n.left]/t[n.right] are safe ONLY while
            // the mutators (insert_leaf, the rotations) preserve that invariant. A
            // one-child node would make t[INVALID_NODE] index out of bounds.
            float cost_left = detail::get_cost_of_child(t[n.left], new_box) + inheritance_cost;
            float cost_right = detail::get_cost_of_child(t[n.right], new_box) + inheritance_cost;

            // If the cost of splitting right here is cheaper than descending further into
            // either child, we BREAK the loop. 'index' is our chosen sibling!
            if ((cost < cost_left) && (cost < cost_right)) {
                break;
            }
            if (cost_left < cost_right) {
                itr = t[itr].left;
            } else {
                itr = t[itr].right;
            }
        }
        return itr;
    }

    /**
     * @brief Rebalance a right-heavy node by promoting its right child (a grandchild-aware AVL
     *        rotation resolving both the RR and RL cases).
     * @param t    The tree.
     * @param aptr The right-heavy node A; its right child B (internal) is promoted, A descends to
     *             B's left, and B's taller grandchild stays high while the shorter moves to A.
     * @return The new subtree root (B).
     */
    inline
    node_ptr rotate_right_up(dynamic_aabb_tree& t, node_ptr aptr) {
        // A is right-heavy. Promote B = A->right; A descends to become B's left child.
        // B's two grandchildren (F, G) are redistributed so the TALLER one stays high
        // under B and the shorter one moves down to A. This single grandchild-aware
        // step resolves both the outer (RR) and inner (RL) imbalance cases.
        auto& a = t[aptr];
        const auto bptr = a.right;
        ENFORCE(bptr && !is_leaf(t[bptr])); // right-heavy ⇒ B is internal
        auto& b = t[bptr];
        const auto fptr = b.left;
        const auto gptr = b.right;

        // --- Link B to A's old parent ---
        b.parent = a.parent;
        if (a.parent) {
            if (auto& p = t[a.parent]; p.left == aptr) {
                p.left = bptr;
            } else {
                p.right = bptr;
            }
        } else {
            t.m_root = bptr; // If A was the root of the whole tree, B is now the root
        }
        // --- A becomes B's left child ---
        b.left = aptr;
        a.parent = bptr;
        // --- Redistribute grandchildren: taller stays under B, shorter moves to A ---
        if (detail::get_height(t, fptr) > detail::get_height(t, gptr)) {
            b.right = fptr;
            a.right = gptr;
        } else {
            b.right = gptr;
            a.right = fptr;
        }
        t[a.right].parent = aptr;
        // --- Recalculate metrics (Bottom-Up order!): A is now lower, update it first ---
        detail::refit(t, a);
        detail::refit(t, b);
        return bptr;
    }

    /**
     * @brief Rebalance a left-heavy node by promoting its left child -- the mirror of
     *        @ref rotate_right_up (resolving both the LL and LR cases).
     * @param t    The tree.
     * @param aptr The left-heavy node A; its left child B (internal) is promoted, A descends to B's
     *             right, and B's taller grandchild stays high while the shorter moves to A.
     * @return The new subtree root (B).
     */
    inline
    node_ptr rotate_left_up(dynamic_aabb_tree& t, node_ptr aptr) {
        // A is left-heavy. Promote B = A->left; A descends to become B's right child.
        // Mirror of rotate_right_up: the taller of B's grandchildren stays high under B,
        // the shorter moves down to A, resolving both the outer (LL) and inner (LR) cases.
        auto& a = t[aptr];
        const auto bptr = a.left;
        ENFORCE(bptr && !is_leaf(t[bptr])); // right-heavy ⇒ B is internal
        auto& b = t[bptr];
        const auto fptr = b.left;
        const auto gptr = b.right;

        // --- Link B to A's old parent ---
        b.parent = a.parent;
        if (a.parent) {
            if (auto& p = t[a.parent]; p.left == aptr) {
                p.left = bptr;
            } else {
                p.right = bptr;
            }
        } else {
            t.m_root = bptr; // If A was the root of the whole tree, B is now the root
        }
        // --- A becomes B's right child ---
        b.right = aptr;
        a.parent = bptr;
        // --- Redistribute grandchildren: taller stays under B, shorter moves to A ---
        if (detail::get_height(t, fptr) > detail::get_height(t, gptr)) {
            b.left = fptr;
            a.left = gptr;
        } else {
            b.left = gptr;
            a.left = fptr;
        }
        t[a.left].parent = aptr;
        // --- Recalculate metrics (Bottom-Up order!): A is now lower, update it first ---
        detail::refit(t, a);
        detail::refit(t, b);
        return bptr;
    }

    /**
     * @brief Refit @p index and fully rebalance the subtree it roots to the strict AVL invariant,
     *        returning the (possibly new) subtree root.
     *
     * @param t     The tree.
     * @param index Subtree root to rebalance. May be off by MORE than one level (a fresh parent next
     *              to a tall sibling), so a single rotation is not enough -- the demoted node is
     *              rebalanced recursively (over a strictly smaller subtree, guaranteeing termination).
     * @return The rebalanced subtree's root handle.
     * @pre Both child subtrees are already strictly balanced (the insert/remove walks guarantee this
     *      by rebalancing bottom-up).
     */
    inline
    node_ptr balance_tree_at_node(dynamic_aabb_tree& t, node_ptr index) {
        // Refits `index` then fully rebalances the subtree it roots to the strict AVL
        // invariant -- |height(left) - height(right)| <= 1 at every node -- returning
        // the (possibly new) subtree root.
        //
        // PRECONDITION: both child subtrees are already strictly balanced. The insert
        // walk guarantees this by rebalancing bottom-up. `index` itself may be off by
        // MORE than one: a new leaf grafted next to a tall internal sibling produces a
        // fresh parent whose imbalance equals the sibling's height.
        //
        // A single rotation removes only one level of imbalance, so recursion is needed.
        // After promoting the taller child, the demoted node descends carrying one
        // grandchild and may still be unbalanced -- we rebalance it (a STRICTLY SMALLER
        // subtree, which guarantees termination). The grandchild-aware rotation then
        // leaves the promoted node balanced once its demoted child is, so a refit (no
        // further rotation) suffices at this level.
        auto& a = t[index];
        if (is_leaf(a)) {
            return index;
        }
        detail::refit(t, a);
        const auto balance = detail::get_height(t, a.right) - detail::get_height(t, a.left);
        if (balance > 1) {
            auto bptr = rotate_right_up(t, index);
            auto& b = t[bptr];
            balance_tree_at_node(t, b.left); // demoted node descended to b->left
            detail::refit(t, b);
            return bptr;
        }
        if (balance < -1) {
            auto bptr = rotate_left_up(t, index);
            auto& b = t[bptr];
            balance_tree_at_node(t, b.right); // demoted node descended to b->right
            detail::refit(t, b);
            return bptr;
        }
        return index;
    }

    /**
     * @brief Graft an already-allocated, detached leaf at the SAH-best sibling and rebalance.
     *
     * Shared by @ref insert_leaf (a fresh node) and @ref update_leaf (re-grafting the same node), so
     * the caller's handle survives a move.
     *
     * @param t        The tree.
     * @param leaf_ptr A leaf node with its @c box set, children null, height 0, and not currently in
     *                 the tree.
     */
    inline
    void insert_existing_leaf(dynamic_aabb_tree& t, node_ptr leaf_ptr) {
        if (!t.m_root) {
            // Empty tree: the leaf becomes the root.
            t.m_root = leaf_ptr;
            t[leaf_ptr].parent = {};
            return;
        }

        // Capture the leaf box by value before the next allocate(), which may reallocate
        // the pool and invalidate any node& we hold. After this point we touch nodes
        // only through t[...] (fresh each time).
        const aabb leaf_box = t[leaf_ptr].box;
        const auto sibling_ptr = find_best_sibling(t, leaf_box);

        const auto old_parent_ptr = t[sibling_ptr].parent;
        const auto merged = aabb::combine(t[sibling_ptr].box, leaf_box);
        const auto sibling_height = t[sibling_ptr].height;

        const auto new_parent_ptr = t.m_storage.allocate(0, merged); // may reallocate

        t[new_parent_ptr].parent = old_parent_ptr;
        t[new_parent_ptr].left = sibling_ptr;
        t[new_parent_ptr].right = leaf_ptr;
        t[new_parent_ptr].height = static_cast <int16_t>(1 + std::max(sibling_height, t[leaf_ptr].height));
        t[sibling_ptr].parent = new_parent_ptr;
        t[leaf_ptr].parent = new_parent_ptr;

        if (!old_parent_ptr) {
            t.m_root = new_parent_ptr;
        } else if (t[old_parent_ptr].left == sibling_ptr) {
            t[old_parent_ptr].left = new_parent_ptr;
        } else {
            t[old_parent_ptr].right = new_parent_ptr;
        }

        for (auto index = new_parent_ptr; index;) {
            index = balance_tree_at_node(t, index);
            index = t[index].parent;
        }
    }

    /**
     * @brief Insert an entity with the given box and return its stable leaf handle.
     * @param t   The tree.
     * @param eid The entity payload echoed back by queries.
     * @param box The leaf's box, stored verbatim -- any enlargement (a "fat AABB" margin for moving
     *            objects) is a policy the caller applies before handing the box down.
     * @return The new leaf's handle (stable until removed).
     */
    inline
    node_ptr insert_leaf(dynamic_aabb_tree& t, entity_id_t eid, const aabb& box) {
        const auto leaf_ptr = t.m_storage.allocate(eid, box);
        insert_existing_leaf(t, leaf_ptr);
        return leaf_ptr;
    }

    namespace detail {
        /**
         * @brief Unlink a leaf from the tree and rebalance, leaving it in a clean standalone state.
         *
         * Internal helper shared by @ref remove_leaf / @ref update_leaf (the intermediate "detached"
         * state only those two know how to finish). Frees the now-obsolete parent but NOT the leaf
         * itself, and clears the leaf's parent so it can be freed or re-grafted.
         *
         * @param t    The tree.
         * @param leaf A live leaf node.
         */
        inline
        void detach_leaf(dynamic_aabb_tree& t, node_ptr leaf) {
            if (leaf == t.m_root) {
                // The leaf is the whole tree.
                t.m_root = {};
                return; // root's parent is already null
            }

            // The parent has exactly two children (full-binary invariant): `leaf` and its
            // sibling. Detaching `leaf` makes the parent obsolete, so the sibling takes the
            // parent's place; the parent is freed.
            const auto parent_ptr = t[leaf].parent;
            const dynamic_aabb_node& parent = t[parent_ptr];
            const auto grandparent_ptr = parent.parent;
            const node_ptr sibling_ptr = (parent.left == leaf) ? parent.right : parent.left;

            if (!grandparent_ptr) {
                // Parent was the root: the sibling becomes the new root (already balanced).
                t.m_root = sibling_ptr;
                t[sibling_ptr].parent = {};
            } else {
                // Splice the sibling into the parent's slot under the grandparent.
                t[sibling_ptr].parent = grandparent_ptr;
                auto& grandparent = t[grandparent_ptr];
                if (grandparent.left == parent_ptr) {
                    grandparent.left = sibling_ptr;
                } else {
                    grandparent.right = sibling_ptr;
                }
            }

            t.m_storage.deallocate(parent_ptr);
            t[leaf].parent = {}; // leaf is now fully detached -- no dangling parent

            // Refit + rebalance from the grandparent up (balance_tree_at_node refits each
            // node itself; the loop is a no-op when the parent was the root).
            for (auto index = grandparent_ptr; index;) {
                index = balance_tree_at_node(t, index);
                index = t[index].parent;
            }
        }
    }

    /**
     * @brief Remove a leaf from the tree and free its slot.
     * @param t    The tree.
     * @param leaf The leaf to remove; a null handle is a no-op.
     * @note Asserts @p leaf is a live leaf (height 0), which also rejects internal nodes and
     *       stale/double-removed handles before they can corrupt the tree.
     */
    inline
    void remove_leaf(dynamic_aabb_tree& t, node_ptr leaf) {
        if (!leaf) {
            return;
        }
        // A live leaf is the only node with height 0 (internal >= 1, freed slot ==
        // FREE_NODE_HEIGHT), so this also rejects internal nodes and stale/double-removed
        // handles before they can corrupt the tree.
        ENFORCE(is_leaf(t[leaf]) && t[leaf].height == 0);
        detail::detach_leaf(t, leaf);
        t.m_storage.deallocate(leaf);
    }

    /**
     * @brief Update a leaf's bounds to @p box, keeping the SAME handle valid.
     *
     * Spatial short-circuit: if @p box is already enclosed by the leaf's stored box, nothing changes
     * and @c false is returned. This is what makes "fat AABB" callers cheap -- they store an enlarged
     * box and pass the object's tight box here, so small moves stay enclosed -- yet the tree itself
     * knows nothing about margins; it just skips work when the new box adds no coverage. Otherwise the
     * leaf is detached and re-grafted in place with @p box.
     *
     * @param t    The tree.
     * @param leaf A live leaf (asserted, as in @ref remove_leaf).
     * @param box  The leaf's new box.
     * @return @c true iff the tree was restructured; @c false on the containment short-circuit.
     */
    inline
    bool update_leaf(dynamic_aabb_tree& t, node_ptr leaf, const aabb& box) {
        ENFORCE(leaf && is_leaf(t[leaf]) && t[leaf].height == 0); // live leaf only (see remove_leaf)
        if (detail::contains(t[leaf].box, box)) {
            return false; // already bounded -> no structural change
        }
        detail::detach_leaf(t, leaf); // keeps the leaf node (handle stays valid)
        t[leaf].box = box;
        insert_existing_leaf(t, leaf); // re-graft the same node
        return true;
    }

    /**
     * @brief Type-erased convenience callback for the @ref query overload below.
     * @note Prefer passing a lambda to the templated @ref query on hot paths -- it inlines through
     *       the descent, whereas @c std::function adds an indirect call per hit.
     */
    using query_callback_t = std::function<void(entity_id_t, const aabb& box)>;

    namespace detail {
        /**
         * @brief Recursive overlap descent for @ref query.
         * @tparam Fn The hit callback (void or bool).
         * @param t      The tree.
         * @param index  Current subtree root.
         * @param box    The query box.
         * @param on_hit Per-leaf callback.
         * @return @c false as soon as a bool callback asks to stop, @c true otherwise (so a parent
         *         can short-circuit its second child).
         */
        template <class Fn>
        bool query_helper(const dynamic_aabb_tree& t, node_ptr index, const aabb& box, Fn& on_hit) {
            const dynamic_aabb_node& vertex = t[index];
            if (!intersects(vertex.box, box)) {
                return true; // this subtree can't overlap -> prune, keep searching elsewhere
            }
            if (is_leaf(vertex)) {
                if constexpr (std::is_void_v<std::invoke_result_t<Fn&, entity_id_t, const aabb&>>) {
                    on_hit(vertex.entity_id, vertex.box);
                    return true; // a void callback never stops early
                } else {
                    return on_hit(vertex.entity_id, vertex.box); // bool callback: false -> stop
                }
            }
            // Internal node: both children exist (full-binary invariant). The && short
            // -circuits, so a stop request in the left subtree skips the right one.
            return query_helper(t, vertex.left, box, on_hit)
                && query_helper(t, vertex.right, box, on_hit);
        }
    }

    /**
     * @brief Report every leaf whose STORED box overlaps @p box.
     * @tparam Fn Callback @c on_hit(entity_id_t, const aabb& stored_box); may return @c void (visit
     *            all matches) or @c bool (return @c false to stop the whole traversal early -- e.g.
     *            first-hit or "does anything overlap?").
     * @param t      The tree.
     * @param box    The query box.
     * @param on_hit Per-candidate callback.
     * @note These are broadphase CANDIDATES, not confirmed overlaps: the reported box is the node's
     *       stored (possibly fat) box and @c intersects treats touching edges as a hit. Callers
     *       narrow-phase the candidates against real geometry.
     */
    template <class Fn>
    void query(const dynamic_aabb_tree& t, const aabb& box, Fn&& on_hit) {
        if (!t.m_root) {
            return;
        }
        detail::query_helper(t, t.m_root, box, on_hit);
    }

    /**
     * @brief Type-erased @ref query overload (visits all matches via a @ref query_callback_t).
     * @param t        The tree.
     * @param box      The query box.
     * @param callback The type-erased per-candidate callback.
     */
    inline
    void query(const dynamic_aabb_tree& t, const aabb& box, const query_callback_t& callback) {
        query<const query_callback_t&>(t, box, callback);
    }

    /**
     * @brief Type-erased convenience callback for @ref raycast.
     *
     * Invoked for each leaf whose stored box the ray crosses, where @c box_hit.entry_param is the
     * fraction along the ray (0 = @c ray.from, 1 = @c ray.to) at which it enters the candidate's box.
     */
    using raycast_callback_t = std::function<void(entity_id_t, const aabb& box, const line_hit& box_hit)>;

    namespace detail {
        /**
         * @brief Recursive ray descent for @ref raycast.
         * @tparam Fn The hit callback (void or float).
         * @param t      The tree.
         * @param index  Current subtree root.
         * @param ray    The query segment.
         * @param t_max  The current ray clip, shrunk as a closest-hit callback reports nearer hits.
         * @param on_hit Per-leaf callback.
         * @return @c false once a callback asks to stop, @c true otherwise.
         */
        template <class Fn>
        bool raycast_helper(const dynamic_aabb_tree& t, node_ptr index, const segment& ray, float& t_max, Fn& on_hit) {
            const dynamic_aabb_node& vertex = t[index];
            const auto hit = intersect_param(vertex.box, ray);
            // Prune unless the ray, clipped to [0, t_max], actually crosses this box.
            if (!hit || hit->entry_param > t_max || hit->exit_param < 0.0f) {
                return true;
            }
            if (is_leaf(vertex)) {
                if constexpr (std::is_void_v<
                    std::invoke_result_t<Fn&, entity_id_t, const aabb&, const line_hit&>>) {
                    on_hit(vertex.entity_id, vertex.box, *hit);
                    return true; // void callback: visit every crossed candidate
                } else {
                    // float callback: returns the new max fraction in [0,1] (e.g. the true
                    // hit fraction from narrow-phase) to clip the ray; <= 0 stops.
                    const float new_max = on_hit(vertex.entity_id, vertex.box, *hit);
                    if (new_max <= 0.0f) {
                        return false;
                    }
                    t_max = std::min(t_max, new_max);
                    return true;
                }
            }
            // Descend both children, pruned by the (possibly shrunk) t_max; the && short
            // -circuits an early stop. (A closest-hit walk could descend the nearer child
            // first to prune harder -- omitted for clarity; correctness is order-independent.)
            return raycast_helper(t, vertex.left, ray, t_max, on_hit)
                && raycast_helper(t, vertex.right, ray, t_max, on_hit);
        }
    }

    /**
     * @brief Report every leaf whose STORED box the segment @p ray crosses (@c ray.from -> @c ray.to,
     *        parameter 0..1).
     * @tparam Fn Callback @c on_hit(entity_id_t, const aabb& box, const line_hit& box_hit); may
     *            return @c void (visit every crossed candidate) or @c float (closest-hit: return the
     *            new max ray fraction in [0,1] to clip farther candidates, @c <= 0 to stop).
     * @param t      The tree.
     * @param ray    The query segment.
     * @param on_hit Per-candidate callback.
     * @note As with @ref query, these are CANDIDATES against stored (possibly fat) boxes -- the
     *       caller narrow-phases against real geometry and, for closest-hit, returns the true hit
     *       fraction to drive the pruning.
     */
    template <class Fn>
    void raycast(const dynamic_aabb_tree& t, const segment& ray, Fn&& on_hit) {
        if (!t.m_root) {
            return;
        }
        float t_max = 1.0f;
        detail::raycast_helper(t, t.m_root, ray, t_max, on_hit);
    }

    /**
     * @brief Type-erased @ref raycast overload (visits every crossed candidate via a
     *        @ref raycast_callback_t).
     * @param t        The tree.
     * @param ray      The query segment.
     * @param callback The type-erased per-candidate callback.
     */
    inline
    void raycast(const dynamic_aabb_tree& t, const segment& ray, const raycast_callback_t& callback) {
        raycast<const raycast_callback_t&>(t, ray, callback);
    }
}
