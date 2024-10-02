#include <random>
#include <array>
#include <doctest/doctest.h>
#include "neutrino/modules/physics/detail/quad_tree/quad_tree.hh"

using namespace neutrino::ecs::physics;



struct Node
{
    Box<float> box;
    std::size_t id;
};

std::vector<Node> generateRandomNodes(std::size_t n)
{
    auto generator = std::default_random_engine();
    auto originDistribution = std::uniform_real_distribution(0.0f, 1.0f);
    auto sizeDistribution = std::uniform_real_distribution(0.0f, 0.01f);
    auto nodes = std::vector<Node>(n);
    for (auto i = std::size_t(0); i < n; ++i)
    {
        nodes[i].box.x = originDistribution(generator);
        nodes[i].box.y = originDistribution(generator);
        nodes[i].box.w = std::min(1.0f - nodes[i].box.left(), sizeDistribution(generator));
        nodes[i].box.h = std::min(1.0f - nodes[i].box.top(), sizeDistribution(generator));
        nodes[i].id = i;
    }
    return nodes;
}

std::vector<Node*> query(const Box<float>& box, std::vector<Node>& nodes, const std::vector<bool>& removed)
{
    auto intersections = std::vector<Node*>();
    for (auto& n : nodes)
    {
        if (removed.size() == 0 || !removed[n.id])
        {
            if (box.intersects(n.box))
                intersections.push_back(&n);
        }
    }
    return intersections;
}

std::vector<std::pair<Node*, Node*>> findAllIntersections(std::vector<Node>& nodes, const std::vector<bool>& removed)
{
    auto intersections = std::vector<std::pair<Node*, Node*>>();
    for (auto i = std::size_t(0); i < nodes.size(); ++i)
    {
        if (removed.size() == 0 || !removed[i])
        {
            for (auto j = std::size_t(0); j < i; ++j)
            {
                if (removed.size() == 0 || !removed[j])
                {
                    if (nodes[i].box.intersects(nodes[j].box))
                        intersections.emplace_back(&nodes[i], &nodes[j]);
                }
            }
        }
    }
    return intersections;
}

bool checkIntersections(std::vector<Node*> nodes1, std::vector<Node*> nodes2)
{
    if (nodes1.size() != nodes2.size())
        return false;
    std::sort(std::begin(nodes1), std::end(nodes1));
    std::sort(std::begin(nodes2), std::end(nodes2));
    return nodes1 == nodes2;
}

bool checkIntersections(std::vector<std::pair<Node*, Node*>> intersections1,
    std::vector<std::pair<Node*, Node*>> intersections2)
{
    if (intersections1.size() != intersections2.size())
        return false;
    for (auto& intersection : intersections1)
    {
        if (intersection.first >= intersection.second)
            std::swap(intersection.first, intersection.second);
    }
    for (auto& intersection : intersections2)
    {
        if (intersection.first >= intersection.second)
            std::swap(intersection.first, intersection.second);
    }
    std::sort(std::begin(intersections1), std::end(intersections1));
    std::sort(std::begin(intersections2), std::end(intersections2));
    return intersections1 == intersections2;
}

std::vector<std::size_t> get_param() {
	std::vector<std::size_t> v;
	for (std::size_t i=1; i<=200; i++) {
		v.push_back(i);
	}
	v.push_back(1000);
	v.push_back(10000);
	return v;
}

TEST_SUITE("quadtree test") {
	TEST_CASE("add_and_query_test")
	{
		for (auto n : get_param()) {

			auto getBox = [](Node* node) {
			  return node->box;
			};
			auto box = Box<float>(0.0f, 0.0f, 1.0f, 1.0f);
			auto nodes = generateRandomNodes(n);
			// Add nodes to quadtree
			auto quadtree = quad_tree<Node*, decltype(getBox)>(box, getBox);
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
				REQUIRE(checkIntersections(intersections1[node.id], intersections2[node.id]));
		}
	}

	TEST_CASE("AddAndFindAllIntersectionsTest")
	{
		for (auto n : get_param()) {
			auto getBox = [](Node* node) {
			  return node->box;
			};
			auto box = Box<float>(0.0f, 0.0f, 1.0f, 1.0f);
			auto nodes = generateRandomNodes(n);
			// Add nodes to quadtree
			auto quadtree = quad_tree<Node*, decltype(getBox)>(box, getBox);
			for (auto& node : nodes)
				quadtree.add(&node);
			// Quadtree
			auto intersections1 = quadtree.find_all_intersections();
			// Brute force
			auto intersections2 = findAllIntersections(nodes, {});
			// Check
			REQUIRE(checkIntersections(intersections1, intersections2));
		}
	}

	TEST_CASE("AddRemoveAndQueryTest")
	{
		for (auto n : get_param()) {
			auto getBox = [](Node* node) {
			  return node->box;
			};
			auto box = Box<float>(0.0f, 0.0f, 1.0f, 1.0f);
			auto nodes = generateRandomNodes(n);
			// Add nodes to quadtree
			auto quadtree = quad_tree<Node*, decltype(getBox)>(box, getBox);
			for (auto& node : nodes)
				quadtree.add(&node);
			// Randomly remove some nodes
			auto generator = std::default_random_engine();
			auto deathDistribution = std::uniform_int_distribution(0, 1);
			auto removed = std::vector<bool>(nodes.size());
			std::generate(std::begin(removed), std::end(removed),
						  [&generator, &deathDistribution]() { return deathDistribution(generator); });
			for (auto& node : nodes) {
				if (removed[node.id])
					quadtree.remove(&node);
			}
			// Quadtree
			auto intersections1 = std::vector<std::vector<Node*>>(n);
			for (const auto& node : nodes) {
				if (!removed[node.id])
					intersections1[node.id] = quadtree.query(node.box);
			}
			// Brute force
			auto intersections2 = std::vector<std::vector<Node*>>(n);
			for (const auto& node : nodes) {
				if (!removed[node.id])
					intersections2[node.id] = query(node.box, nodes, removed);
			}
			// Check
			for (const auto& node : nodes) {
				if (!removed[node.id]) {
					REQUIRE(checkIntersections(intersections1[node.id], intersections2[node.id]));
				}
			}
		}
	}

	TEST_CASE("AddRemoveAndFindAllIntersectionsTest")
	{
		for (auto n : get_param()) {
			auto getBox = [](Node* node) {
			  return node->box;
			};
			auto box = Box<float>(0.0f, 0.0f, 1.0f, 1.0f);
			auto nodes = generateRandomNodes(n);
			// Add nodes to quadtree
			auto quadtree = quad_tree<Node*, decltype(getBox)>(box, getBox);
			for (auto& node : nodes)
				quadtree.add(&node);
			// Randomly remove some nodes
			auto generator = std::default_random_engine();
			auto deathDistribution = std::uniform_int_distribution(0, 1);
			auto removed = std::vector<bool>(nodes.size());
			std::generate(std::begin(removed), std::end(removed),
						  [&generator, &deathDistribution]() { return deathDistribution(generator); });
			for (auto& node : nodes) {
				if (removed[node.id])
					quadtree.remove(&node);
			}
			// Quadtree
			auto intersections1 = quadtree.find_all_intersections();
			// Brute force
			auto intersections2 = findAllIntersections(nodes, removed);
			// Check
			REQUIRE(checkIntersections(intersections1, intersections2));
		}
	}
}
