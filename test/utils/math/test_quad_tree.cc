//
// Created by igor on 28/04/2022.
//
#include <random>
#include <doctest/doctest.h>
#include <neutrino/math/quad_tree.hh>

using namespace neutrino::math;

namespace {
  struct Node {
    box2d<float> box;
    std::size_t id;
  };

  std::vector<Node> generate_random_nodes (std::size_t n) {
    auto generator = std::default_random_engine ();
    auto originDistribution = std::uniform_real_distribution (0.0f, 1.0f);
    auto sizeDistribution = std::uniform_real_distribution (0.0f, 0.01f);
    auto nodes = std::vector<Node> (n);
    for (auto i = std::size_t (0); i < n; ++i) {
      float left = originDistribution (generator);
      float top = originDistribution (generator);
      float w = std::min (1.0f - left, sizeDistribution (generator));
      float h = std::min (1.0f - top, sizeDistribution (generator));
      nodes[i].box = box2d<float> (left, top, w, h);
      nodes[i].id = i;
    }
    return nodes;
  }

  std::vector<Node*> query (const box2d<float>& box, std::vector<Node>& nodes, const std::vector<bool>& removed) {
    auto intersections = std::vector<Node*> ();
    for (auto& n: nodes) {
      if (removed.size () == 0 || !removed[n.id]) {
        if (box.intersects (n.box))
          intersections.push_back (&n);
      }
    }
    return intersections;
  }

  std::vector<std::pair<Node*, Node*>>
  find_all_intersections (std::vector<Node>& nodes, const std::vector<bool>& removed) {
    auto intersections = std::vector<std::pair<Node*, Node*>> ();
    for (auto i = std::size_t (0); i < nodes.size (); ++i) {
      if (removed.size () == 0 || !removed[i]) {
        for (auto j = std::size_t (0); j < i; ++j) {
          if (removed.size () == 0 || !removed[j]) {
            if (nodes[i].box.intersects (nodes[j].box))
              intersections.emplace_back (&nodes[i], &nodes[j]);
          }
        }
      }
    }
    return intersections;
  }

  bool check_intersections (std::vector<Node*> nodes1, std::vector<Node*> nodes2) {
    if (nodes1.size () != nodes2.size ())
      return false;
    std::sort (std::begin (nodes1), std::end (nodes1));
    std::sort (std::begin (nodes2), std::end (nodes2));
    return nodes1 == nodes2;
  }

  bool check_intersections (std::vector<std::pair<Node*, Node*>> intersections1,
                            std::vector<std::pair<Node*, Node*>> intersections2) {
    if (intersections1.size () != intersections2.size ())
      return false;
    for (auto& intersection: intersections1) {
      if (intersection.first >= intersection.second)
        std::swap (intersection.first, intersection.second);
    }
    for (auto& intersection: intersections2) {
      if (intersection.first >= intersection.second)
        std::swap (intersection.first, intersection.second);
    }
    std::sort (std::begin (intersections1), std::end (intersections1));
    std::sort (std::begin (intersections2), std::end (intersections2));
    return intersections1 == intersections2;
  }
}

template <std::size_t n>
struct Holder {
  static constexpr auto value = n;
};

TEST_CASE_TEMPLATE_DEFINE("quadtree - add and query", T, AddAndQuery) {
  auto n = T::value;
  auto getbox2d = [](Node* node)
  {
    return node->box;
  };
  auto box = box2d<float>(0.0f, 0.0f, 1.0f, 1.0f);
  auto nodes = generate_random_nodes (n);
  // Add nodes to quadtree
  auto quadtree = quad_tree<Node*, decltype(getbox2d)>(box, getbox2d);
  for (auto& node : nodes)
    quadtree.add(&node);
  // Quadtree
  auto intersections1 = std::vector<std::vector<Node*>>(nodes.size());
  for (const auto& node : nodes)
    intersections1[node.id] = quadtree.query(node.box);
  // Brute force
  auto intersections2 = std::vector<std::vector<Node*>>(nodes.size());
  for (const auto& node : nodes)
    intersections2[node.id] = query(node.box, nodes, {});
  // Check
  for (const auto& node : nodes)
    REQUIRE(check_intersections (intersections1[node.id], intersections2[node.id]));
}

TEST_CASE_TEMPLATE_DEFINE("quadtree - add and remove and find all intersections", T, AddRemoveAndFindAllIntersections) {
  auto n = T::value;
  auto getBox = [](Node* node)
  {
    return node->box;
  };
  auto box = box2d<float>(0.0f, 0.0f, 1.0f, 1.0f);
    auto nodes = generate_random_nodes(n);
    // Add nodes to quadtree
    auto quadtree = quad_tree<Node*, decltype(getBox)>(box, getBox);
    for (auto& node : nodes)
        quadtree.add(&node);
    // Randomly remove some nodes
    auto generator = std::default_random_engine();
    auto deathDistribution = std::uniform_int_distribution(0, 1);
    auto removed = std::vector<bool>(nodes.size());
    std::generate(std::begin(removed), std::end(removed),
        [&generator, &deathDistribution](){ return deathDistribution(generator); });
    for (auto& node : nodes)
    {
        if (removed[node.id])
            quadtree.remove(&node);
    }
    // Quadtree
    auto intersections1 = quadtree.find_all_intersections();
    // Brute force
    auto intersections2 = find_all_intersections(nodes, removed);
    // Check
    REQUIRE(check_intersections(intersections1, intersections2));
}

TEST_SUITE("quadtree test") {
  TEST_CASE_TEMPLATE_INVOKE(AddAndQuery, Holder<50>, Holder<100>, Holder<200>, Holder<1000>);
  TEST_CASE_TEMPLATE_INVOKE(AddRemoveAndFindAllIntersections, Holder<50>, Holder<100>, Holder<200>, Holder<1000>);
}