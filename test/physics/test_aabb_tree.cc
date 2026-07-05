//
// Created by igor on 19/06/2026.
//
// Ground-truth correctness suite for the index/storage dynamic AABB tree in
// <neutrino/physics/collide/dynamic_aabb_tree.hh>.
//
// The suite is split into two kinds of checks:
//
//   * STRUCTURAL invariants (validate_node / validate_tree) -- these must hold for
//     *any* correct binary AABB tree regardless of the balancing strategy:
//       - parent <-> child links are mutually consistent;
//       - the root has no parent (a null node_ptr);
//       - every internal node has exactly two children, every leaf has none;
//       - a leaf has height 0; an internal node has height 1 + max(child heights);
//       - a node's stored box is exactly the union of its two children's boxes
//         (and therefore exactly bounds every descendant leaf);
//       - the set of leaf boxes / entity ids equals exactly what was inserted.
//
//   * BALANCE invariants (max_imbalance / tree height) -- specific to the current
//     AVL-style strategy (rebalance whenever |h(L) - h(R)| > 1). If the balancing
//     strategy is ever changed, only these checks need revisiting.
//
// Box equality is tested with exact float comparison on purpose: aabb::combine only
// ever selects existing min/max coordinates (no arithmetic), so an exactly-correct
// tree reproduces the inserted coordinates bit-for-bit.
//
// Nodes are referenced by a strongly-typed index (node_ptr); the tree owns its
// storage pool, so there is nothing to free -- a tree cleans up when it goes out of
// scope. Inside doctest CHECK/REQUIRE we compare the raw indices (*ptr) so failures
// print readable integers; in plain control flow we use node_ptr's explicit bool.
//
#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include <neutrino/physics/collide/dynamic_aabb_tree.hh>

using namespace neutrino::physics;

namespace {
    constexpr auto NIL = node_ptr::INVALID_NODE; // raw sentinel index for CHECK comparisons

    // ---- small builders ----------------------------------------------------

    aabb box_at(float x, float y, float w = 1.0f, float h = 1.0f) {
        return aabb{{x, y}, {x + w, y + h}};
    }

    // The "fat AABB" margin is a caller-side policy (the tree stores boxes verbatim), so
    // the update tests fatten boxes themselves, playing the role of a broadphase layer.
    aabb fatten(const aabb& b, float m) {
        return aabb{{b.min.x() - m, b.min.y() - m}, {b.max.x() + m, b.max.y() + m}};
    }

    bool box_eq(const aabb& a, const aabb& b) {
        return a.min.x() == b.min.x() && a.min.y() == b.min.y()
            && a.max.x() == b.max.x() && a.max.y() == b.max.y();
    }

    // Height of a node by index, with the null convention (leaf == 0, absent == -1).
    int height_of(const dynamic_aabb_tree& t, node_ptr i) {
        return i ? t[i].height : -1;
    }

    // Allocate a leaf in the tree's pool and return its index.
    node_ptr mk_leaf(dynamic_aabb_tree& t, entity_id_t eid, const aabb& box) {
        return t.m_storage.allocate(eid, box);
    }

    // Allocate an internal node over two existing children, wiring links and height.
    // Careful: aabb::combine is evaluated before allocate() (which may reallocate the
    // pool); afterwards we only touch nodes by index.
    node_ptr mk_internal(dynamic_aabb_tree& t, node_ptr l, node_ptr r) {
        const aabb merged = aabb::combine(t[l].box, t[r].box);
        const node_ptr p = t.m_storage.allocate(0, merged);
        t[p].left = l;
        t[p].right = r;
        t[p].height = static_cast<int16_t>(1 + std::max(t[l].height, t[r].height));
        t[l].parent = p;
        t[r].parent = p;
        return p;
    }

    // ---- structural validation --------------------------------------------

    struct tree_stats {
        int leaf_count = 0;
        int internal_count = 0;
        int max_leaf_depth = 0;
        std::vector<node_ptr> leaves;
    };

    // Validates every structural invariant of the subtree rooted at index `i` and
    // returns the exact bounding box of all leaves beneath it. `expected_parent` is
    // what `t[i].parent` must equal.
    aabb validate_node(const dynamic_aabb_tree& t, node_ptr i, node_ptr expected_parent, int depth, tree_stats& st) {
        REQUIRE(*i != NIL);
        const dynamic_aabb_node& n = t[i];
        CHECK_MESSAGE(*n.parent == *expected_parent, "parent link is inconsistent with child link");

        if (is_leaf(n)) {
            st.leaf_count++;
            st.leaves.push_back(i);
            st.max_leaf_depth = std::max(st.max_leaf_depth, depth);
            CHECK_MESSAGE(*n.left == NIL, "leaf must have no left child");
            CHECK_MESSAGE(*n.right == NIL, "leaf must have no right child");
            CHECK_MESSAGE(n.height == 0, "leaf height must be 0");
            return n.box;
        }

        st.internal_count++;
        // An internal node (one with any child) must have BOTH children.
        REQUIRE_MESSAGE(*n.left != NIL, "internal node missing left child");
        REQUIRE_MESSAGE(*n.right != NIL, "internal node missing right child");

        const aabb lb = validate_node(t, n.left, i, depth + 1, st);
        const aabb rb = validate_node(t, n.right, i, depth + 1, st);

        // Height bookkeeping must match the children.
        const int expected_height = 1 + std::max(t[n.left].height, t[n.right].height);
        CHECK_MESSAGE(n.height == expected_height, "stored height inconsistent with children");

        // The box must be exactly the union of the two children's stored boxes ...
        CHECK_MESSAGE(box_eq(n.box, aabb::combine(t[n.left].box, t[n.right].box)),
                      "node box != union of child boxes");
        // ... which (by induction) is exactly the bound of all descendant leaves.
        const aabb leaf_union = aabb::combine(lb, rb);
        CHECK_MESSAGE(box_eq(n.box, leaf_union), "node box != exact bound of descendant leaves");
        return leaf_union;
    }

    // Whole-tree structural check. `expected` maps entity id -> inserted box.
    void validate_tree(const dynamic_aabb_tree& t, const std::map<entity_id_t, aabb>& expected) {
        if (expected.empty()) {
            CHECK(*t.m_root == NIL);
            return;
        }
        REQUIRE(*t.m_root != NIL);
        CHECK_MESSAGE(*t[t.m_root].parent == NIL, "root must have no parent");

        tree_stats st;
        validate_node(t, t.m_root, node_ptr{}, 0, st);

        // Exactly the inserted leaves are present, once each, with the right boxes.
        CHECK(st.leaf_count == static_cast<int>(expected.size()));
        CHECK(st.internal_count == st.leaf_count - 1); // a full binary tree

        std::set<entity_id_t> seen;
        for (const node_ptr li : st.leaves) {
            const dynamic_aabb_node& leaf = t[li];
            auto it = expected.find(leaf.entity_id);
            REQUIRE_MESSAGE(it != expected.end(), "leaf has an entity id that was never inserted");
            CHECK_MESSAGE(box_eq(leaf.box, it->second), "leaf box differs from inserted box");
            const bool fresh = seen.insert(leaf.entity_id).second;
            CHECK_MESSAGE(fresh, "duplicate entity id among leaves");
        }
        CHECK(seen.size() == expected.size());
    }

    // ---- balance metrics (AVL-strategy specific) ---------------------------

    int max_imbalance(const dynamic_aabb_tree& t, node_ptr i) {
        if (!i || is_leaf(t[i])) {
            return 0;
        }
        const dynamic_aabb_node& n = t[i];
        return std::max({std::abs(height_of(t, n.left) - height_of(t, n.right)),
                         max_imbalance(t, n.left), max_imbalance(t, n.right)});
    }

    // Locate the leaf carrying `eid` by traversing from the root (insert_leaf returns
    // void, so tests recover a leaf handle this way). Returns a null node_ptr if absent.
    node_ptr find_leaf(const dynamic_aabb_tree& t, entity_id_t eid) {
        std::vector<node_ptr> stack;
        if (t.m_root) {
            stack.push_back(t.m_root);
        }
        while (!stack.empty()) {
            const node_ptr cur = stack.back();
            stack.pop_back();
            const dynamic_aabb_node& n = t[cur];
            if (is_leaf(n)) {
                if (n.entity_id == eid) {
                    return cur;
                }
            } else {
                stack.push_back(n.left);
                stack.push_back(n.right);
            }
        }
        return node_ptr{};
    }

    // Entity ids reported by query() for a region (collected into a set for comparison).
    std::set<entity_id_t> query_ids(const dynamic_aabb_tree& t, const aabb& region) {
        std::set<entity_id_t> hits;
        query(t, region, [&hits](entity_id_t id, const aabb&) { hits.insert(id); });
        return hits;
    }

    // The ground-truth answer: every live box that actually overlaps the region.
    std::set<entity_id_t> brute_ids(const std::map<entity_id_t, aabb>& live, const aabb& region) {
        std::set<entity_id_t> hits;
        for (const auto& kv : live) {
            if (intersects(kv.second, region)) {
                hits.insert(kv.first);
            }
        }
        return hits;
    }

    segment seg(float x0, float y0, float x1, float y1) {
        return segment{{x0, y0}, {x1, y1}};
    }

    // Entity ids reported by raycast() for a ray (collected for comparison).
    std::set<entity_id_t> raycast_ids(const dynamic_aabb_tree& t, const segment& ray) {
        std::set<entity_id_t> hits;
        raycast(t, ray, [&hits](entity_id_t id, const aabb&, const line_hit&) { hits.insert(id); });
        return hits;
    }

    // Ground truth: every live box the segment crosses, using the same primitive the
    // tree prunes with so the two agree exactly at boundaries.
    std::set<entity_id_t> brute_ray(const std::map<entity_id_t, aabb>& live, const segment& ray) {
        std::set<entity_id_t> hits;
        for (const auto& kv : live) {
            const auto h = intersect_param(kv.second, ray);
            if (h && h->segment_overlaps()) {
                hits.insert(kv.first);
            }
        }
        return hits;
    }

    // Build a tree from a list of boxes (entity id = index), validate all structural
    // invariants, and assert the strict AVL balance invariant (|hL - hR| <= 1 at
    // every node). The recursive rebalance maintains this for any insertion order.
    void build_and_validate(const std::vector<aabb>& boxes) {
        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> expected;
        for (std::size_t i = 0; i < boxes.size(); ++i) {
            insert_leaf(t, static_cast<entity_id_t>(i), boxes[i]);
            expected[static_cast<entity_id_t>(i)] = boxes[i];
        }
        validate_tree(t, expected);
        CHECK_MESSAGE(max_imbalance(t, t.m_root) <= 1, "AVL balance invariant violated (|hL - hR| > 1)");

        // Non-degeneracy guard: an AVL tree's height is < 1.4405*log2(n+2). A generous
        // 2x*log2(n) ceiling still catches a regression to a linked list.
        if (boxes.size() >= 2) {
            const int ideal = static_cast<int>(std::ceil(std::log2(static_cast<double>(boxes.size()))));
            CHECK_MESSAGE(t[t.m_root].height <= 2 * ideal + 1,
                          "tree height grew far beyond logarithmic -> balancing broke");
        }
    }
} // namespace

TEST_SUITE("aabb_tree: helpers") {
    TEST_CASE("aabb::combine produces the exact union") {
        const aabb a{{0.0f, 0.0f}, {2.0f, 2.0f}};
        const aabb b{{1.0f, -1.0f}, {5.0f, 1.0f}};
        const aabb c = aabb::combine(a, b);
        CHECK(c.min.x() == 0.0f);
        CHECK(c.min.y() == -1.0f);
        CHECK(c.max.x() == 5.0f);
        CHECK(c.max.y() == 2.0f);

        // commutative and idempotent
        CHECK(box_eq(aabb::combine(a, b), aabb::combine(b, a)));
        CHECK(box_eq(aabb::combine(a, a), a));
        // absorbing: union with an enclosed box is the enclosing box
        const aabb inner{{0.5f, 0.5f}, {1.0f, 1.0f}};
        CHECK(box_eq(aabb::combine(a, inner), a));
    }

    TEST_CASE("aabb::area") {
        CHECK(aabb{{0.0f, 0.0f}, {2.0f, 3.0f}}.area() == 6.0f);
        CHECK(aabb{{-1.0f, -1.0f}, {1.0f, 1.0f}}.area() == 4.0f);
        CHECK(aabb{{0.0f, 0.0f}, {0.0f, 5.0f}}.area() == 0.0f); // degenerate (line)
        CHECK(aabb{{3.0f, 3.0f}, {3.0f, 3.0f}}.area() == 0.0f); // degenerate (point)
        static_assert(aabb{{0.0f, 0.0f}, {2.0f, 3.0f}}.area() == 6.0f, "area is constexpr");
    }

    TEST_CASE("node_ptr: null default and explicit value") {
        const node_ptr nul;          // default ctor -> null
        const node_ptr p{7};         // explicit value
        CHECK(*nul == NIL);
        CHECK_FALSE(static_cast<bool>(nul));
        CHECK(*p == 7);
        CHECK(static_cast<bool>(p));
        CHECK(p != nul);
        CHECK(node_ptr{7} == p);
    }

    TEST_CASE("is_leaf") {
        // A node{} default-constructs its child node_ptrs to null, i.e. a leaf.
        dynamic_aabb_node leaf{};
        CHECK(is_leaf(leaf));

        dynamic_aabb_node internal{};
        internal.left = node_ptr{0};
        internal.right = node_ptr{1};
        CHECK_FALSE(is_leaf(internal));
    }

    TEST_CASE("get_height: absent is -1, leaf is 0") {
        dynamic_aabb_tree t;
        CHECK(detail::get_height(t, node_ptr{}) == -1);
        const node_ptr leaf = mk_leaf(t, 1, box_at(0, 0));
        CHECK(detail::get_height(t, leaf) == 0);
    }
}

TEST_SUITE("aabb_tree: insertion structure") {
    TEST_CASE("empty tree has a null root") {
        dynamic_aabb_tree t;
        validate_tree(t, {});
    }

    TEST_CASE("reset() empties the tree and leaves it reusable") {
        dynamic_aabb_tree t;
        for (int i = 0; i < 8; ++i) {
            insert_leaf(t, static_cast<entity_id_t>(i), box_at(i * 2, 0));
        }
        REQUIRE(static_cast<bool>(t.m_root));

        t.reset();
        CHECK_FALSE(static_cast<bool>(t.m_root));            // root cleared
        CHECK(query_ids(t, box_at(0, 0, 100, 100)).empty()); // nothing reported
        validate_tree(t, {});                                // structurally an empty tree

        // reusable after reset: re-insert and every invariant holds again
        std::map<entity_id_t, aabb> expected;
        for (int i = 0; i < 5; ++i) {
            insert_leaf(t, static_cast<entity_id_t>(i), box_at(i, i));
            expected[static_cast<entity_id_t>(i)] = box_at(i, i);
        }
        validate_tree(t, expected);
        CHECK(query_ids(t, box_at(0, 0, 100, 100)).size() == 5u);
    }

    TEST_CASE("single insert: root is the leaf itself") {
        dynamic_aabb_tree t;
        insert_leaf(t, 42, box_at(1, 1, 3, 4));

        REQUIRE(*t.m_root != NIL);
        CHECK(is_leaf(t[t.m_root]));
        CHECK(*t[t.m_root].parent == NIL);
        CHECK(t[t.m_root].entity_id == 42);
        CHECK(t[t.m_root].height == 0);
        CHECK(box_eq(t[t.m_root].box, box_at(1, 1, 3, 4)));

        validate_tree(t, {{42, box_at(1, 1, 3, 4)}});
    }

    TEST_CASE("two inserts: root becomes an internal node over two leaves") {
        dynamic_aabb_tree t;
        insert_leaf(t, 0, box_at(0, 0));
        insert_leaf(t, 1, box_at(10, 0));

        REQUIRE(*t.m_root != NIL);
        CHECK_FALSE(is_leaf(t[t.m_root]));
        CHECK(*t[t.m_root].parent == NIL);
        CHECK(t[t.m_root].height == 1);
        CHECK(is_leaf(t[t[t.m_root].left]));
        CHECK(is_leaf(t[t[t.m_root].right]));
        // the internal box bounds both leaves
        CHECK(box_eq(t[t.m_root].box, aabb::combine(box_at(0, 0), box_at(10, 0))));

        validate_tree(t, {{0, box_at(0, 0)}, {1, box_at(10, 0)}});
    }

    TEST_CASE("three inserts keep all invariants") {
        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> exp;
        for (int i = 0; i < 3; ++i) {
            const aabb b = box_at(static_cast<float>(i) * 5.0f, 0.0f);
            insert_leaf(t, i, b);
            exp[i] = b;
        }
        validate_tree(t, exp);
        CHECK(max_imbalance(t, t.m_root) <= 1);
    }
}

TEST_SUITE("aabb_tree: invariants across insertion patterns") {
    TEST_CASE("ascending (worst case for a naive BVH)") {
        std::vector<aabb> boxes;
        for (int i = 0; i < 256; ++i) {
            boxes.push_back(box_at(static_cast<float>(i), 0.0f));
        }
        build_and_validate(boxes);
    }

    TEST_CASE("descending") {
        std::vector<aabb> boxes;
        for (int i = 0; i < 256; ++i) {
            boxes.push_back(box_at(static_cast<float>(1000 - i), 0.0f));
        }
        build_and_validate(boxes);
    }

    TEST_CASE("diagonal spread") {
        std::vector<aabb> boxes;
        for (int i = 0; i < 200; ++i) {
            boxes.push_back(box_at(static_cast<float>(i), static_cast<float>(i)));
        }
        build_and_validate(boxes);
    }

    TEST_CASE("clustered grid") {
        std::vector<aabb> boxes;
        for (int i = 0; i < 150; ++i) {
            boxes.push_back(box_at(static_cast<float>(i % 4), static_cast<float>(i / 4)));
        }
        build_and_validate(boxes);
    }

    TEST_CASE("identical overlapping boxes (zero pairwise distance)") {
        std::vector<aabb> boxes(64, box_at(2.0f, 2.0f, 3.0f, 3.0f));
        build_and_validate(boxes);
    }

    TEST_CASE("pseudo-random scatter") {
        std::vector<aabb> boxes;
        std::uint32_t seed = 0x1234567u;
        auto next = [&seed]() {
            seed = seed * 1664525u + 1013904223u;
            return seed;
        };
        for (int i = 0; i < 512; ++i) {
            const float x = static_cast<float>(next() % 2000u) * 0.1f;
            const float y = static_cast<float>(next() % 2000u) * 0.1f;
            const float w = 1.0f + static_cast<float>(next() % 30u) * 0.1f;
            const float h = 1.0f + static_cast<float>(next() % 30u) * 0.1f;
            boxes.push_back(box_at(x, y, w, h));
        }
        build_and_validate(boxes);
    }

    TEST_CASE("large N stays balanced and correct") {
        std::vector<aabb> boxes;
        for (int i = 0; i < 2000; ++i) {
            boxes.push_back(box_at(static_cast<float>(i), static_cast<float>((i * 37) % 113)));
        }
        build_and_validate(boxes);
    }
}

TEST_SUITE("aabb_tree: balance quality (AVL strategy)") {
    TEST_CASE("height is logarithmic on adversarial monotonic insertion") {
        dynamic_aabb_tree t;
        const int n = 1024;
        std::map<entity_id_t, aabb> exp;
        for (int i = 0; i < n; ++i) {
            const aabb b = box_at(static_cast<float>(i), 0.0f);
            insert_leaf(t, i, b);
            exp[i] = b;
        }
        validate_tree(t, exp);

        // AVL height bound: h <= 1.4405 * log2(n+2) - 0.3277. Use a generous ceiling.
        const int ideal = static_cast<int>(std::ceil(std::log2(static_cast<double>(n))));
        CHECK_MESSAGE(t[t.m_root].height <= 2 * ideal,
                      "tree height grew far beyond logarithmic -> not balanced");
        CHECK(max_imbalance(t, t.m_root) <= 1);
    }

    // The strongest form of the guarantee: the strict AVL invariant must hold after
    // EVERY insert, not just at the end -- and regardless of insertion order. Uses the
    // adversarial strip pattern fed in a scrambled order.
    TEST_CASE("strict AVL is maintained incrementally and order-independently") {
        // Scramble indices with an LCG-driven Fisher-Yates so insertion order is mixed.
        const int n = 1500;
        std::vector<int> order(n);
        for (int i = 0; i < n; ++i) {
            order[i] = i;
        }
        std::uint32_t seed = 0xC0FFEEu;
        auto next = [&seed]() {
            seed = seed * 1664525u + 1013904223u;
            return seed;
        };
        for (int i = n - 1; i > 0; --i) {
            const int j = static_cast<int>(next() % static_cast<std::uint32_t>(i + 1));
            std::swap(order[i], order[j]);
        }

        dynamic_aabb_tree t;
        for (int k = 0; k < n; ++k) {
            const int id = order[k];
            insert_leaf(t, id, box_at(static_cast<float>(id), static_cast<float>((id * 37) % 113)));
            // Invariant holds at every step.
            REQUIRE(max_imbalance(t, t.m_root) <= 1);
        }
        // Final full structural sweep.
        std::map<entity_id_t, aabb> exp;
        for (int id = 0; id < n; ++id) {
            exp[id] = box_at(static_cast<float>(id), static_cast<float>((id * 37) % 113));
        }
        validate_tree(t, exp);
    }

    // Strict AVL invariant at scale on the box pattern that previously defeated the
    // single-rotation strategy (a long horizontal strip that makes find_best_sibling
    // graft next to tall internal nodes). The recursive rebalance holds |hL - hR| <= 1
    // and keeps the height within a small constant of optimal.
    TEST_CASE("strict AVL balance |hL - hR| <= 1 at scale") {
        dynamic_aabb_tree t;
        const int n = 8000;
        for (int i = 0; i < n; ++i) {
            insert_leaf(t, i, box_at(static_cast<float>(i), static_cast<float>((i * 37) % 113)));
        }
        CHECK_MESSAGE(max_imbalance(t, t.m_root) <= 1, "AVL balance invariant violated (|hL - hR| > 1)");

        const int ideal = static_cast<int>(std::ceil(std::log2(static_cast<double>(n))));
        CHECK(t[t.m_root].height <= 2 * ideal);
    }
}

TEST_SUITE("aabb_tree: rotations") {
    /*
     * Builds (right-heavy at A):
     *            A
     *          /   \
     *         X     B
     *              / \
     *          inner  G      (inner = taller grandchild, height 1)
     *          /  \
     *        i0    i1
     * rotate_right_up must promote B, keep the TALLER grandchild (inner) under B,
     * and push the shorter grandchild (G) down to A (the inner / RL case).
     */
    TEST_CASE("rotate_right_up: inner case is grandchild-aware") {
        dynamic_aabb_tree t;
        const node_ptr X = mk_leaf(t, 1, box_at(0, 0));
        const node_ptr i0 = mk_leaf(t, 2, box_at(10, 0));
        const node_ptr i1 = mk_leaf(t, 3, box_at(11, 0));
        const node_ptr G = mk_leaf(t, 4, box_at(20, 0));
        const node_ptr inner = mk_internal(t, i0, i1); // height 1
        const node_ptr B = mk_internal(t, inner, G);   // height 2
        const node_ptr A = mk_internal(t, X, B);       // height 3
        t.m_root = A;

        const node_ptr new_root = rotate_right_up(t, A);

        CHECK(*new_root == *B);
        CHECK(*t.m_root == *B);
        CHECK(*t[B].parent == NIL);
        CHECK(*t[B].left == *A);
        CHECK(*t[B].right == *inner); // taller grandchild stayed high
        CHECK(*t[A].left == *X);
        CHECK(*t[A].right == *G);     // shorter grandchild pushed down
        CHECK(*t[A].parent == *B);
        CHECK(*t[inner].parent == *B);
        CHECK(*t[G].parent == *A);
        CHECK(t[A].height == 1);
        CHECK(t[inner].height == 1);
        CHECK(t[B].height == 2);

        // and the result is a fully valid, balanced tree
        tree_stats st;
        validate_node(t, B, node_ptr{}, 0, st);
        CHECK(max_imbalance(t, B) <= 1);
    }

    /*
     * Mirror of the above; left-heavy outer (LL) case where the taller grandchild
     * is on the outside and stays in place (left-heavy at A):
     *            A
     *          /   \
     *         B     X
     *        / \
     *    outer  g
     *     /  \
     *   o0    o1
     */
    TEST_CASE("rotate_left_up: outer case keeps the outer grandchild high") {
        dynamic_aabb_tree t;
        const node_ptr X = mk_leaf(t, 1, box_at(100, 0));
        const node_ptr o0 = mk_leaf(t, 2, box_at(0, 0));
        const node_ptr o1 = mk_leaf(t, 3, box_at(1, 0));
        const node_ptr g = mk_leaf(t, 4, box_at(5, 0));
        const node_ptr outer = mk_internal(t, o0, o1); // height 1
        const node_ptr B = mk_internal(t, outer, g);   // height 2
        const node_ptr A = mk_internal(t, B, X);       // height 3
        t.m_root = A;

        const node_ptr new_root = rotate_left_up(t, A);

        CHECK(*new_root == *B);
        CHECK(*t.m_root == *B);
        CHECK(*t[B].parent == NIL);
        CHECK(*t[B].left == *outer); // taller (outer) grandchild stays high
        CHECK(*t[B].right == *A);
        CHECK(*t[A].left == *g);     // shorter grandchild pushed down
        CHECK(*t[A].right == *X);
        CHECK(*t[A].parent == *B);
        CHECK(*t[outer].parent == *B);
        CHECK(*t[g].parent == *A);
        CHECK(t[A].height == 1);
        CHECK(t[B].height == 2);

        tree_stats st;
        validate_node(t, B, node_ptr{}, 0, st);
        CHECK(max_imbalance(t, B) <= 1);
    }

    TEST_CASE("rotation reseats a non-root subtree into its parent") {
        // root.left = A (right-heavy), root.right = sib.
        // After balancing A, root.left must point to the promoted node with correct links.
        dynamic_aabb_tree t;
        const node_ptr X = mk_leaf(t, 1, box_at(0, 0));
        const node_ptr g0 = mk_leaf(t, 2, box_at(20, 0));
        const node_ptr g1 = mk_leaf(t, 3, box_at(21, 0));
        const node_ptr G = mk_leaf(t, 4, box_at(30, 0));
        const node_ptr inner = mk_internal(t, g0, g1); // height 1
        const node_ptr B = mk_internal(t, inner, G);   // height 2
        const node_ptr A = mk_internal(t, X, B);       // height 3
        const node_ptr sib = mk_leaf(t, 5, box_at(200, 0));
        const node_ptr root = mk_internal(t, A, sib);  // height 4
        t.m_root = root;

        const node_ptr promoted = balance_tree_at_node(t, A);

        CHECK(*promoted == *B);     // B was promoted in A's place
        CHECK(*t.m_root == *root);  // root unchanged
        CHECK(*t[root].left == *B); // re-seated under the original parent
        CHECK(*t[B].parent == *root);
        CHECK(*t[root].right == *sib);

        // The rotated subtree is valid (note: root's own height/box are refit by the
        // insert loop in production; here we only assert the rotated subtree).
        tree_stats st;
        validate_node(t, B, root, 1, st);
        CHECK(max_imbalance(t, B) <= 1);
    }

    TEST_CASE("balance_tree_at_node is a no-op on a balanced node and on leaves") {
        // balanced internal node
        dynamic_aabb_tree t;
        const node_ptr l = mk_leaf(t, 1, box_at(0, 0));
        const node_ptr r = mk_leaf(t, 2, box_at(10, 0));
        const node_ptr p = mk_internal(t, l, r); // height 1, balanced
        t.m_root = p;
        CHECK(*balance_tree_at_node(t, p) == *p);
        CHECK(*t.m_root == *p);

        // leaf
        dynamic_aabb_tree t2;
        const node_ptr leaf = mk_leaf(t2, 3, box_at(0, 0));
        t2.m_root = leaf;
        CHECK(*balance_tree_at_node(t2, leaf) == *leaf);
    }
}

TEST_SUITE("aabb_tree: sibling selection") {
    // find_best_sibling must return a node that actually belongs to the tree.
    TEST_CASE("find_best_sibling returns a node reachable from the root") {
        dynamic_aabb_tree t;
        for (int i = 0; i < 32; ++i) {
            insert_leaf(t, i, box_at(static_cast<float>(i), static_cast<float>((i * 13) % 20)));
        }

        const aabb probe = box_at(7.0f, 3.0f);
        const node_ptr sib = find_best_sibling(t, probe);
        REQUIRE(*sib != NIL);

        // walk down from the root following children until we either reach `sib`
        // or fall off -- `sib` must be encountered.
        bool found = false;
        std::vector<node_ptr> stack{t.m_root};
        while (!stack.empty()) {
            const node_ptr cur = stack.back();
            stack.pop_back();
            if (*cur == *sib) {
                found = true;
                break;
            }
            if (t[cur].left) {
                stack.push_back(t[cur].left);
            }
            if (t[cur].right) {
                stack.push_back(t[cur].right);
            }
        }
        CHECK_MESSAGE(found, "find_best_sibling returned a node not present in the tree");
    }

    TEST_CASE("inserting into a tight cluster makes a nearby leaf the sibling") {
        // Two far-apart clusters; a probe inside cluster A should be paired with a
        // box from cluster A, so the new internal parent stays small.
        dynamic_aabb_tree t;
        // cluster A around the origin
        for (int i = 0; i < 8; ++i) {
            insert_leaf(t, i, box_at(static_cast<float>(i) * 0.1f, 0.0f, 0.5f, 0.5f));
        }
        // cluster B far away
        for (int i = 0; i < 8; ++i) {
            insert_leaf(t, 100 + i, box_at(1000.0f + static_cast<float>(i) * 0.1f, 0.0f, 0.5f, 0.5f));
        }

        const aabb probe = box_at(0.25f, 0.0f, 0.5f, 0.5f); // squarely inside cluster A
        const node_ptr sib = find_best_sibling(t, probe);
        REQUIRE(*sib != NIL);

        // the new parent (sibling box U probe) should be far cheaper than pairing
        // with anything in cluster B.
        const float chosen_area = aabb::combine(t[sib].box, probe).area();
        CHECK_MESSAGE(chosen_area < 100.0f,
                      "sibling selection paired the probe with a distant cluster");
    }
}

TEST_SUITE("aabb_tree: removal structure") {
    TEST_CASE("remove the only leaf empties the tree and recycles its slot") {
        dynamic_aabb_tree t;
        insert_leaf(t, 7, box_at(1, 1));
        const std::size_t cap = t.m_storage.capacity_used();
        remove_leaf(t, t.m_root);

        CHECK(*t.m_root == NIL);
        validate_tree(t, {});
        // a subsequent insert reuses the freed slot rather than growing the pool
        insert_leaf(t, 8, box_at(2, 2));
        CHECK(t.m_storage.capacity_used() == cap);
        validate_tree(t, {{8, box_at(2, 2)}});
    }

    TEST_CASE("remove one of two: the sibling becomes the root leaf") {
        dynamic_aabb_tree t;
        insert_leaf(t, 0, box_at(0, 0));
        insert_leaf(t, 1, box_at(10, 0));
        const node_ptr l0 = find_leaf(t, 0);
        REQUIRE(*l0 != NIL);

        remove_leaf(t, l0);

        REQUIRE(*t.m_root != NIL);
        CHECK(is_leaf(t[t.m_root]));
        CHECK(t[t.m_root].entity_id == 1);
        CHECK(*t[t.m_root].parent == NIL);
        CHECK(t[t.m_root].height == 0);
        validate_tree(t, {{1, box_at(10, 0)}});
    }

    TEST_CASE("remove(null) and remove from empty are no-ops") {
        dynamic_aabb_tree t;
        remove_leaf(t, node_ptr{});      // null on empty tree
        CHECK(*t.m_root == NIL);

        insert_leaf(t, 1, box_at(0, 0));
        remove_leaf(t, node_ptr{});      // null on non-empty tree
        validate_tree(t, {{1, box_at(0, 0)}});
    }

    /*
     * Deterministic rebalance-on-remove. Start from a valid AVL tree where the left
     * subtree is height 2 and the right is height 1; removing a leaf from the right
     * shortens it to height 0, leaving the root off by 2 -> remove must rotate.
     *
     *              R (h3)
     *            /       \
     *         A (h2)      C (h1)
     *         /   \       /   \
     *      A0(h1)  A1   c0     c1
     *      /  \
     *    x0    x1
     */
    TEST_CASE("remove triggers a rebalance and restores strict AVL") {
        dynamic_aabb_tree t;
        const node_ptr x0 = mk_leaf(t, 1, box_at(0, 0));
        const node_ptr x1 = mk_leaf(t, 2, box_at(1, 0));
        const node_ptr A0 = mk_internal(t, x0, x1);     // h1
        const node_ptr A1 = mk_leaf(t, 3, box_at(2, 0));
        const node_ptr A = mk_internal(t, A0, A1);      // h2
        const node_ptr c0 = mk_leaf(t, 4, box_at(10, 0));
        const node_ptr c1 = mk_leaf(t, 5, box_at(11, 0));
        const node_ptr C = mk_internal(t, c0, c1);      // h1
        const node_ptr R = mk_internal(t, A, C);        // h3
        t.m_root = R;

        // sanity: the hand-built starting tree is valid and balanced
        { tree_stats st; validate_node(t, R, node_ptr{}, 0, st); }
        REQUIRE(max_imbalance(t, R) <= 1);

        remove_leaf(t, c0); // C collapses to c1, root becomes left-heavy by 2

        tree_stats st;
        validate_node(t, t.m_root, node_ptr{}, 0, st);
        CHECK(st.leaf_count == 4);                 // x0, x1, A1, c1 remain
        CHECK(max_imbalance(t, t.m_root) <= 1);    // rebalanced
    }

    TEST_CASE("remove every leaf one-by-one (scrambled) keeps invariants and empties") {
        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> live;
        const int n = 64;
        for (int i = 0; i < n; ++i) {
            const aabb b = box_at(static_cast<float>(i), static_cast<float>((i * 37) % 30));
            insert_leaf(t, i, b);
            live[i] = b;
        }

        std::vector<int> ids;
        for (const auto& kv : live) {
            ids.push_back(kv.first);
        }
        std::uint32_t seed = 0xBEEFu;
        auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed; };
        for (int i = n - 1; i > 0; --i) {
            std::swap(ids[i], ids[static_cast<int>(next() % static_cast<std::uint32_t>(i + 1))]);
        }

        for (const int id : ids) {
            const node_ptr leaf = find_leaf(t, id);
            REQUIRE(*leaf != NIL);
            remove_leaf(t, leaf);
            live.erase(id);
            validate_tree(t, live);                  // structure + leaf set correct
            CHECK(max_imbalance(t, t.m_root) <= 1);  // strictly balanced at every step
        }
        CHECK(*t.m_root == NIL);
    }
}

TEST_SUITE("aabb_tree: insert/remove sequences") {
    // Random interleaving of inserts and removes; after EVERY operation the tree must
    // be structurally correct, hold the exact live leaf set, and stay strictly AVL.
    TEST_CASE("interleaved churn keeps all invariants after every op") {
        std::uint32_t seed = 0x5151u;
        auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed; };

        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> live;
        entity_id_t next_id = 0;

        for (int op = 0; op < 800; ++op) {
            const bool do_insert = live.empty() || (next() & 1u);
            if (do_insert) {
                const aabb b = box_at(static_cast<float>(next() % 300u) * 0.25f,
                                      static_cast<float>(next() % 300u) * 0.25f,
                                      1.0f + static_cast<float>(next() % 20u) * 0.1f,
                                      1.0f + static_cast<float>(next() % 20u) * 0.1f);
                insert_leaf(t, next_id, b);
                live[next_id] = b;
                ++next_id;
            } else {
                auto it = live.begin();
                std::advance(it, static_cast<long>(next() % live.size()));
                const node_ptr leaf = find_leaf(t, it->first);
                REQUIRE(*leaf != NIL);
                remove_leaf(t, leaf);
                live.erase(it);
            }
            validate_tree(t, live);
            CHECK(max_imbalance(t, t.m_root) <= 1);
        }
    }

    // The pool must not grow without bound: hold a fixed population and cycle
    // remove+insert many times. capacity_used() should stay at the fill high-water
    // mark (freed slots are recycled), not scale with the number of cycles.
    TEST_CASE("capacity is stable across insert/remove cycles (no slot leak)") {
        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> live;
        entity_id_t next_id = 0;

        const int population = 200;
        for (int i = 0; i < population; ++i) {
            const aabb b = box_at(static_cast<float>(i), 0.0f);
            insert_leaf(t, next_id, b);
            live[next_id] = b;
            ++next_id;
        }
        const std::size_t cap_after_fill = t.m_storage.capacity_used();

        std::uint32_t seed = 0x1357u;
        auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed; };
        for (int cycle = 0; cycle < 3000; ++cycle) {
            // remove a random live leaf ...
            auto it = live.begin();
            std::advance(it, static_cast<long>(next() % live.size()));
            const node_ptr leaf = find_leaf(t, it->first);
            REQUIRE(*leaf != NIL);
            remove_leaf(t, leaf);
            live.erase(it);
            // ... then insert a fresh one, keeping the population constant
            const aabb b = box_at(static_cast<float>(next() % 800u) * 0.5f, 0.0f);
            insert_leaf(t, next_id, b);
            live[next_id] = b;
            ++next_id;
        }

        validate_tree(t, live);
        CHECK(max_imbalance(t, t.m_root) <= 1);
        // remove-before-insert each cycle means the two freed slots are reused, so the
        // pool never grows past the fill mark (small slack for allocation ordering).
        CHECK(t.m_storage.capacity_used() <= cap_after_fill + 4);
    }
}

TEST_SUITE("aabb_tree: update and handles") {
    TEST_CASE("insert_leaf returns a stable handle to the new leaf") {
        dynamic_aabb_tree t;
        const node_ptr h0 = insert_leaf(t, 10, box_at(0, 0));
        const node_ptr h1 = insert_leaf(t, 20, box_at(5, 5));
        REQUIRE(*h0 != NIL);
        REQUIRE(*h1 != NIL);
        CHECK(is_leaf(t[h0]));
        CHECK(is_leaf(t[h1]));
        CHECK(t[h0].entity_id == 10);
        CHECK(t[h1].entity_id == 20);
        CHECK(*find_leaf(t, 10) == *h0); // handle agrees with a tree search

        remove_leaf(t, h0); // remove via the returned handle
        validate_tree(t, {{20, box_at(5, 5)}});
    }

    TEST_CASE("update reinserts when the new box is not enclosed; handle stays valid") {
        dynamic_aabb_tree t;
        const node_ptr a = insert_leaf(t, 1, box_at(0, 0));
        insert_leaf(t, 2, box_at(100, 0));

        const bool moved = update_leaf(t, a, box_at(200, 0));
        CHECK(moved);                       // new box escapes the stored box -> reinsert
        CHECK(*a != NIL);                   // same handle still valid
        CHECK(is_leaf(t[a]));
        CHECK(t[a].entity_id == 1);
        CHECK(box_eq(t[a].box, box_at(200, 0)));
        validate_tree(t, {{1, box_at(200, 0)}, {2, box_at(100, 0)}});
        CHECK(max_imbalance(t, t.m_root) <= 1);
    }

    // The spatial short-circuit, exercised the way a fat-AABB broadphase would: store an
    // enlarged box, then update with the object's tight box -- if it's still enclosed,
    // the tree does nothing. (The margin lives here in the test, not in the tree.)
    TEST_CASE("update is a no-op when the new box is already enclosed") {
        dynamic_aabb_tree t;
        const node_ptr a = insert_leaf(t, 1, fatten(box_at(0, 0, 1, 1), 1.0f)); // stored [-1,-1]..[2,2]
        insert_leaf(t, 2, box_at(100, 0));

        const node_ptr root_before = t.m_root;
        const std::size_t cap_before = t.m_storage.capacity_used();
        const aabb stored_before = t[a].box;

        const bool moved = update_leaf(t, a, box_at(0.2f, 0.2f, 1, 1)); // [0.2,0.2]..[1.2,1.2] ⊆ stored
        CHECK_FALSE(moved);
        CHECK(*t.m_root == *root_before);                  // structure untouched
        CHECK(t.m_storage.capacity_used() == cap_before);  // no alloc/free
        CHECK(box_eq(t[a].box, stored_before));            // stored box unchanged
    }

    TEST_CASE("update re-grafts and keeps the handle when the box escapes") {
        dynamic_aabb_tree t;
        const float margin = 0.5f;
        std::map<entity_id_t, aabb> expected;
        std::vector<node_ptr> h;
        for (int i = 0; i < 16; ++i) {
            const aabb fat = fatten(box_at(static_cast<float>(i), 0.0f), margin);
            h.push_back(insert_leaf(t, i, fat));
            expected[i] = fat;
        }
        validate_tree(t, expected);

        const aabb new_fat = fatten(box_at(1000, 1000), margin);
        const bool moved = update_leaf(t, h[0], new_fat);
        CHECK(moved);
        CHECK(*h[0] != NIL);                 // handle preserved
        CHECK(is_leaf(t[h[0]]));
        CHECK(t[h[0]].entity_id == 0);
        CHECK(box_eq(t[h[0]].box, new_fat)); // tree stored exactly what we gave it

        expected[0] = new_fat;
        validate_tree(t, expected);
        CHECK(max_imbalance(t, t.m_root) <= 1);
    }

    TEST_CASE("update the only leaf keeps it as the root") {
        dynamic_aabb_tree t;
        const node_ptr a = insert_leaf(t, 7, box_at(0, 0));
        const bool moved = update_leaf(t, a, box_at(50, 50));
        CHECK(moved);
        CHECK(*t.m_root == *a);
        CHECK(box_eq(t[a].box, box_at(50, 50)));
        validate_tree(t, {{7, box_at(50, 50)}});
    }

    // Mix insert/remove/update, with the test acting as a fat-AABB broadphase: it stores
    // enlarged boxes and, on move, decides whether the object's tight box is still
    // enclosed (absorb -> pass tight, tree skips) or has escaped (recenter -> pass a
    // fresh fat box, tree reinserts). After every op the tree must be structurally
    // correct, hold the exact live (fat) box set, and stay strictly AVL, and every
    // handle must survive its updates.
    TEST_CASE("interleaved insert/remove/update churn keeps invariants and handles") {
        dynamic_aabb_tree t;
        const float margin = 0.5f;

        std::uint32_t seed = 0x99u;
        auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed; };
        auto rand_box = [&]() {
            return box_at(static_cast<float>(next() % 200u) * 0.25f,
                          static_cast<float>(next() % 200u) * 0.25f);
        };

        struct live_entry { node_ptr h; aabb fat; };
        std::map<entity_id_t, live_entry> live;
        entity_id_t next_id = 0;

        for (int op = 0; op < 800; ++op) {
            const unsigned choice = next() % 3u;
            if (live.empty() || choice == 0u) { // insert
                const aabb fat = fatten(rand_box(), margin);
                const node_ptr handle = insert_leaf(t, next_id, fat);
                live[next_id] = {handle, fat};
                ++next_id;
            } else if (choice == 1u) {          // remove
                auto it = live.begin();
                std::advance(it, static_cast<long>(next() % live.size()));
                remove_leaf(t, it->second.h);
                live.erase(it);
            } else {                            // update (broadphase policy lives here)
                auto it = live.begin();
                std::advance(it, static_cast<long>(next() % live.size()));
                const aabb tight = rand_box();
                if (detail::contains(it->second.fat, tight)) {
                    // still enclosed: hand the tight box down; the tree must absorb it
                    CHECK_FALSE(update_leaf(t, it->second.h, tight));
                } else {
                    // escaped: recenter with a fresh fat box; the tree must reinsert
                    const aabb new_fat = fatten(tight, margin);
                    CHECK(update_leaf(t, it->second.h, new_fat));
                    it->second.fat = new_fat;
                }
                REQUIRE(is_leaf(t[it->second.h]));                   // handle still a leaf ...
                CHECK(t[it->second.h].entity_id == it->first);       // ... for the same entity
                CHECK(detail::contains(t[it->second.h].box, tight)); // stored box bounds the object
            }

            std::map<entity_id_t, aabb> expected;
            for (const auto& kv : live) {
                expected[kv.first] = kv.second.fat;
            }
            validate_tree(t, expected);
            CHECK(max_imbalance(t, t.m_root) <= 1);
        }
    }
}

TEST_SUITE("aabb_tree: query") {
    TEST_CASE("query on an empty tree visits nothing") {
        dynamic_aabb_tree t;
        int calls = 0;
        query(t, box_at(0, 0), [&calls](entity_id_t, const aabb&) { ++calls; });
        CHECK(calls == 0);
    }

    TEST_CASE("single leaf: hit and miss") {
        dynamic_aabb_tree t;
        insert_leaf(t, 5, box_at(0, 0, 2, 2)); // [0,0]..[2,2]
        CHECK(query_ids(t, box_at(1, 1)) == std::set<entity_id_t>{5});       // overlapping
        CHECK(query_ids(t, box_at(10, 10)).empty());                        // disjoint
    }

    TEST_CASE("touching boundary counts as a hit (inclusive overlap)") {
        dynamic_aabb_tree t;
        insert_leaf(t, 1, box_at(0, 0, 1, 1)); // [0,0]..[1,1]
        // region shares only the edge x == 1 with the leaf
        const aabb edge_region{{1.0f, 0.0f}, {2.0f, 1.0f}};
        CHECK(query_ids(t, edge_region).count(1) == 1);
    }

    TEST_CASE("callback receives the leaf's stored box") {
        dynamic_aabb_tree t;
        const aabb b = box_at(3, 4, 2, 5);
        insert_leaf(t, 9, b);
        aabb got{};
        query(t, b, [&](entity_id_t id, const aabb& box) {
            if (id == 9) {
                got = box;
            }
        });
        CHECK(box_eq(got, b));
    }

    TEST_CASE("region query returns exactly the overlapping leaves") {
        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> live;
        for (int i = 0; i < 100; ++i) { // 10x10 grid of unit boxes at integer coords
            const aabb b = box_at(static_cast<float>(i % 10), static_cast<float>(i / 10));
            insert_leaf(t, i, b);
            live[i] = b;
        }
        // a few hand-picked regions
        for (const aabb region : {box_at(0, 0, 0.5f, 0.5f),       // corner
                                  box_at(2.5f, 2.5f, 3.0f, 3.0f), // interior block
                                  box_at(-5, -5, 2, 2),           // off-grid edge
                                  box_at(100, 100, 1, 1)}) {      // empty area
            CHECK(query_ids(t, region) == brute_ids(live, region));
        }
    }

    TEST_CASE("query matches brute force over random boxes and regions") {
        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> live;
        std::uint32_t seed = 0xA11CEu;
        auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed; };
        auto rand_box = [&](float maxw) {
            const float x = static_cast<float>(next() % 500u) * 0.2f;
            const float y = static_cast<float>(next() % 500u) * 0.2f;
            const float w = 0.5f + static_cast<float>(next() % 40u) * 0.1f;
            const float h = 0.5f + static_cast<float>(next() % 40u) * 0.1f;
            (void)maxw;
            return box_at(x, y, w, h);
        };
        for (int i = 0; i < 300; ++i) {
            const aabb b = rand_box(4.0f);
            insert_leaf(t, i, b);
            live[i] = b;
        }
        for (int q = 0; q < 200; ++q) {
            const aabb region = rand_box(8.0f);
            CHECK(query_ids(t, region) == brute_ids(live, region));
        }
    }

    TEST_CASE("bool callback stops the traversal early") {
        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> live;
        // a cluster of boxes that all overlap the query region
        const aabb region = box_at(0, 0, 5, 5);
        for (int i = 0; i < 8; ++i) {
            const aabb b = box_at(static_cast<float>(i) * 0.25f, 0.0f, 1.0f, 1.0f);
            insert_leaf(t, i, b);
            live[i] = b;
        }
        const std::size_t total = brute_ids(live, region).size();
        REQUIRE(total > 1); // the region genuinely matches several, so stopping is meaningful

        // visit-all (void) sees every match
        int seen_all = 0;
        query(t, region, [&seen_all](entity_id_t, const aabb&) { ++seen_all; });
        CHECK(seen_all == static_cast<int>(total));

        // returning false stops after the very first hit
        int seen_one = 0;
        query(t, region, [&seen_one](entity_id_t, const aabb&) {
            ++seen_one;
            return false;
        });
        CHECK(seen_one == 1);

        // existence query: any overlap?
        bool any = false;
        query(t, region, [&any](entity_id_t, const aabb&) {
            any = true;
            return false;
        });
        CHECK(any);
        bool any_empty = false;
        query(t, box_at(1000, 1000), [&any_empty](entity_id_t, const aabb&) {
            any_empty = true;
            return false;
        });
        CHECK_FALSE(any_empty);
    }

    TEST_CASE("type-erased query_callback_t overload works") {
        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> live;
        for (int i = 0; i < 16; ++i) {
            const aabb b = box_at(static_cast<float>(i), 0.0f);
            insert_leaf(t, i, b);
            live[i] = b;
        }
        std::set<entity_id_t> hits;
        const query_callback_t cb = [&hits](entity_id_t id, const aabb&) { hits.insert(id); };
        const aabb region = box_at(3, 0, 4, 1); // [3,0]..[7,1]
        query(t, region, cb);
        CHECK(hits == brute_ids(live, region));
    }

    TEST_CASE("query reflects removals and updates") {
        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> live;
        for (int i = 0; i < 20; ++i) {
            const aabb b = box_at(static_cast<float>(i), 0.0f);
            insert_leaf(t, i, b);
            live[i] = b;
        }
        // remove a few and confirm they vanish from query results
        for (const int id : {3, 7, 11}) {
            remove_leaf(t, find_leaf(t, id));
            live.erase(id);
        }
        const aabb wide = box_at(-1, -1, 30, 2);
        CHECK(query_ids(t, wide) == brute_ids(live, wide));

        // move one entity far away; old location no longer hits, new one does
        const node_ptr h = find_leaf(t, 5);
        update_leaf(t, h, box_at(500, 500));
        live[5] = box_at(500, 500);
        CHECK(query_ids(t, box_at(5, 0)).count(5) == 0);   // gone from the old spot
        CHECK(query_ids(t, box_at(500, 500)).count(5) == 1); // present at the new spot
        CHECK(query_ids(t, wide) == brute_ids(live, wide));
    }

    // With fat boxes, query reports candidates that may not truly overlap; the caller is
    // expected to narrow-phase against the tight geometry.
    TEST_CASE("query over-reports with fat boxes (caller narrow-phases)") {
        dynamic_aabb_tree t;
        const float margin = 1.0f;
        const aabb tight = box_at(0, 0, 1, 1);          // [0,0]..[1,1]
        insert_leaf(t, 1, fatten(tight, margin));       // stored [-1,-1]..[2,2]

        const aabb region{{-0.5f, -0.5f}, {-0.1f, -0.1f}}; // overlaps the fat box, not the tight one
        CHECK(query_ids(t, region).count(1) == 1);      // reported as a candidate
        CHECK_FALSE(intersects(tight, region));         // ... but the real box does not overlap
    }
}

TEST_SUITE("aabb_tree: raycast") {
    TEST_CASE("raycast on an empty tree visits nothing") {
        dynamic_aabb_tree t;
        int calls = 0;
        raycast(t, seg(0, 0, 10, 10), [&calls](entity_id_t, const aabb&, const line_hit&) { ++calls; });
        CHECK(calls == 0);
    }

    TEST_CASE("single box: ray hits and misses") {
        dynamic_aabb_tree t;
        insert_leaf(t, 5, box_at(10, -1, 2, 2)); // [10,-1]..[12,1]
        CHECK(raycast_ids(t, seg(0, 0, 20, 0)) == std::set<entity_id_t>{5}); // straight through
        CHECK(raycast_ids(t, seg(0, 5, 20, 5)).empty());                     // parallel, above
        CHECK(raycast_ids(t, seg(0, 0, 5, 0)).empty());                      // stops short of the box
    }

    TEST_CASE("entry_param is the fraction along the ray at the box face") {
        dynamic_aabb_tree t;
        insert_leaf(t, 1, box_at(10, -1, 2, 2)); // enters at x == 10
        float entry = -1.0f;
        raycast(t, seg(0, 0, 20, 0), [&](entity_id_t, const aabb&, const line_hit& h) {
            entry = h.entry_param;
        });
        CHECK(entry == doctest::Approx(0.5f)); // 10 / 20
    }

    TEST_CASE("ray along a row reports exactly the crossed boxes") {
        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> live;
        for (int i = 0; i < 10; ++i) {
            const aabb b = box_at(static_cast<float>(i), static_cast<float>(i % 2)); // staggered in y
            insert_leaf(t, i, b);
            live[i] = b;
        }
        // a horizontal ray at y in [0,1] only crosses the boxes whose row it passes through
        for (const segment ray : {seg(-1, 0.5f, 20, 0.5f), seg(-1, 1.5f, 20, 1.5f), seg(-1, 9, 20, 9)}) {
            CHECK(raycast_ids(t, ray) == brute_ray(live, ray));
        }
    }

    TEST_CASE("raycast matches brute force over random boxes and rays") {
        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> live;
        std::uint32_t seed = 0x4A11u;
        auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed; };
        auto coord = [&]() { return static_cast<float>(next() % 1000u) * 0.1f; };
        for (int i = 0; i < 300; ++i) {
            const aabb b = box_at(coord(), coord(), 0.5f + static_cast<float>(next() % 30u) * 0.1f,
                                  0.5f + static_cast<float>(next() % 30u) * 0.1f);
            insert_leaf(t, i, b);
            live[i] = b;
        }
        for (int q = 0; q < 200; ++q) {
            const segment ray = seg(coord(), coord(), coord(), coord());
            CHECK(raycast_ids(t, ray) == brute_ray(live, ray));
        }
    }

    TEST_CASE("closest-hit narrowing finds the nearest box") {
        dynamic_aabb_tree t;
        // unit boxes at x = 0..9, all straddling y = 0.5
        for (int i = 0; i < 10; ++i) {
            insert_leaf(t, i, box_at(static_cast<float>(i), 0.0f));
        }
        const segment ray = seg(-1.0f, 0.5f, 20.0f, 0.5f); // crosses every box, left to right

        float best = 2.0f;
        entity_id_t best_id = -1;
        raycast(t, ray, [&](entity_id_t id, const aabb&, const line_hit& h) {
            if (h.entry_param < best) {
                best = h.entry_param;
                best_id = id;
            }
            return h.entry_param; // clip the ray to this candidate's entry -> prune farther boxes
        });
        CHECK(best_id == 0); // box at x=0 is nearest along +x
    }

    TEST_CASE("returning 0 stops the raycast after the first candidate") {
        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> live;
        for (int i = 0; i < 8; ++i) {
            const aabb b = box_at(static_cast<float>(i), 0.0f);
            insert_leaf(t, i, b);
            live[i] = b;
        }
        const segment ray = seg(-1.0f, 0.5f, 20.0f, 0.5f);
        REQUIRE(brute_ray(live, ray).size() > 1); // the ray genuinely crosses several

        int calls = 0;
        raycast(t, ray, [&calls](entity_id_t, const aabb&, const line_hit&) {
            ++calls;
            return 0.0f; // stop immediately
        });
        CHECK(calls == 1);
    }

    TEST_CASE("type-erased raycast_callback_t overload works") {
        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> live;
        for (int i = 0; i < 12; ++i) {
            const aabb b = box_at(static_cast<float>(i), 0.0f);
            insert_leaf(t, i, b);
            live[i] = b;
        }
        std::set<entity_id_t> hits;
        const raycast_callback_t cb = [&hits](entity_id_t id, const aabb&, const line_hit&) {
            hits.insert(id);
        };
        const segment ray = seg(-1, 0.5f, 20, 0.5f);
        raycast(t, ray, cb);
        CHECK(hits == brute_ray(live, ray));
    }

    TEST_CASE("raycast reflects removals and updates") {
        dynamic_aabb_tree t;
        std::map<entity_id_t, aabb> live;
        for (int i = 0; i < 10; ++i) {
            const aabb b = box_at(static_cast<float>(i), 0.0f);
            insert_leaf(t, i, b);
            live[i] = b;
        }
        const segment ray = seg(-1, 0.5f, 20, 0.5f);

        remove_leaf(t, find_leaf(t, 4));
        live.erase(4);
        CHECK(raycast_ids(t, ray) == brute_ray(live, ray));
        CHECK(raycast_ids(t, ray).count(4) == 0);

        update_leaf(t, find_leaf(t, 7), box_at(7, 100)); // move off the ray
        live[7] = box_at(7, 100);
        CHECK(raycast_ids(t, ray) == brute_ray(live, ray));
        CHECK(raycast_ids(t, ray).count(7) == 0);
    }
}

TEST_SUITE("aabb_tree: precondition guards") {
    // A live leaf is the only node with height 0; remove_leaf/update_leaf enforce that,
    // which rejects internal nodes and stale/freed handles (height == FREE_NODE_HEIGHT)
    // before they can corrupt the tree. ENFORCE throws, so misuse is observable here, and
    // because the check runs before any mutation the tree is left intact.

    TEST_CASE("remove_leaf rejects a double remove (stale handle)") {
        dynamic_aabb_tree t;
        const node_ptr a = insert_leaf(t, 1, box_at(0, 0));
        insert_leaf(t, 2, box_at(10, 0));

        remove_leaf(t, a);                  // first remove: fine
        CHECK_THROWS(remove_leaf(t, a));    // freed handle: rejected, not silently corrupting
        validate_tree(t, {{2, box_at(10, 0)}}); // tree untouched by the rejected call
    }

    TEST_CASE("remove_leaf rejects an internal node") {
        dynamic_aabb_tree t;
        insert_leaf(t, 1, box_at(0, 0));
        insert_leaf(t, 2, box_at(10, 0)); // root is now an internal node
        REQUIRE_FALSE(is_leaf(t[t.m_root]));

        CHECK_THROWS(remove_leaf(t, t.m_root));
        validate_tree(t, {{1, box_at(0, 0)}, {2, box_at(10, 0)}});
    }

    TEST_CASE("update_leaf rejects a stale handle") {
        dynamic_aabb_tree t;
        const node_ptr a = insert_leaf(t, 1, box_at(0, 0));
        insert_leaf(t, 2, box_at(10, 0));

        remove_leaf(t, a);
        CHECK_THROWS(update_leaf(t, a, box_at(5, 5)));
        validate_tree(t, {{2, box_at(10, 0)}});
    }
}
