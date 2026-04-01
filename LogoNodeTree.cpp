#include "LogoNodeTree.h"

#include <algorithm>
#include <limits>
#include <utility>

void LogoNodeTree::Clear()
{
    nodes_.clear();
    rootIndex_ = -1;
}

void LogoNodeTree::Rebuild(const std::vector<LogoNodeInput>& items)
{
    Clear();
    if (items.empty())
    {
        return;
    }

    std::vector<LogoNodeInput> copy = items;
    nodes_.reserve(copy.size());
    rootIndex_ = BuildRecursive(std::move(copy), 0);
}

int LogoNodeTree::BuildRecursive(std::vector<LogoNodeInput> items, int depth)
{
    if (items.empty())
    {
        return -1;
    }

    const bool splitOnX = (depth % 2) == 0;
    auto middle = items.begin() + static_cast<std::ptrdiff_t>(items.size() / 2);
    std::nth_element(
        items.begin(),
        middle,
        items.end(),
        [splitOnX](const LogoNodeInput& a, const LogoNodeInput& b)
        {
            return splitOnX ? a.x < b.x : a.y < b.y;
        });

    const int nodeIndex = static_cast<int>(nodes_.size());
    nodes_.push_back(Node{});
    nodes_[static_cast<std::size_t>(nodeIndex)].item = *middle;

    std::vector<LogoNodeInput> left(items.begin(), middle);
    std::vector<LogoNodeInput> right(middle + 1, items.end());
    const int leftIndex = BuildRecursive(std::move(left), depth + 1);
    const int rightIndex = BuildRecursive(std::move(right), depth + 1);

    Node& node = nodes_[static_cast<std::size_t>(nodeIndex)];
    node.left = leftIndex;
    node.right = rightIndex;
    node.minX = node.maxX = node.item.x;
    node.minY = node.maxY = node.item.y;

    if (leftIndex >= 0)
    {
        const Node& leftNode = nodes_[static_cast<std::size_t>(leftIndex)];
        node.minX = std::min(node.minX, leftNode.minX);
        node.maxX = std::max(node.maxX, leftNode.maxX);
        node.minY = std::min(node.minY, leftNode.minY);
        node.maxY = std::max(node.maxY, leftNode.maxY);
    }

    if (rightIndex >= 0)
    {
        const Node& rightNode = nodes_[static_cast<std::size_t>(rightIndex)];
        node.minX = std::min(node.minX, rightNode.minX);
        node.maxX = std::max(node.maxX, rightNode.maxX);
        node.minY = std::min(node.minY, rightNode.minY);
        node.maxY = std::max(node.maxY, rightNode.maxY);
    }

    return nodeIndex;
}

int LogoNodeTree::FindNearest(float x, float y) const
{
    if (rootIndex_ < 0)
    {
        return -1;
    }

    float bestDistanceSquared = std::numeric_limits<float>::max();
    return FindNearestImpl(rootIndex_, x, y, -1, bestDistanceSquared);
}

void LogoNodeTree::GatherWithinRadius(float x, float y, float radius, std::vector<std::size_t>& outIndices) const
{
    if (rootIndex_ < 0)
    {
        return;
    }

    GatherWithinRadiusImpl(rootIndex_, x, y, radius * radius, outIndices);
}

int LogoNodeTree::FindNearestImpl(int index, float x, float y, int bestIndex, float& bestDistanceSquared) const
{
    if (index < 0)
    {
        return bestIndex;
    }

    const Node& node = nodes_[static_cast<std::size_t>(index)];
    if (DistanceSquaredToBounds(x, y, node) > bestDistanceSquared)
    {
        return bestIndex;
    }

    const float nodeDistance = DistanceSquared(x, y, node.item.x, node.item.y);
    if (nodeDistance < bestDistanceSquared)
    {
        bestDistanceSquared = nodeDistance;
        bestIndex = static_cast<int>(node.item.index);
    }

    const int firstChild = (node.left >= 0) ? node.left : node.right;
    const int secondChild = (node.left >= 0) ? node.right : -1;

    bestIndex = FindNearestImpl(firstChild, x, y, bestIndex, bestDistanceSquared);
    if (secondChild >= 0)
    {
        bestIndex = FindNearestImpl(secondChild, x, y, bestIndex, bestDistanceSquared);
    }

    return bestIndex;
}

void LogoNodeTree::GatherWithinRadiusImpl(int index, float x, float y, float radiusSquared, std::vector<std::size_t>& outIndices) const
{
    if (index < 0)
    {
        return;
    }

    const Node& node = nodes_[static_cast<std::size_t>(index)];
    if (DistanceSquaredToBounds(x, y, node) > radiusSquared)
    {
        return;
    }

    if (DistanceSquared(x, y, node.item.x, node.item.y) <= radiusSquared)
    {
        outIndices.push_back(node.item.index);
    }

    GatherWithinRadiusImpl(node.left, x, y, radiusSquared, outIndices);
    GatherWithinRadiusImpl(node.right, x, y, radiusSquared, outIndices);
}

float LogoNodeTree::DistanceSquared(float ax, float ay, float bx, float by)
{
    const float dx = ax - bx;
    const float dy = ay - by;
    return (dx * dx) + (dy * dy);
}

float LogoNodeTree::DistanceSquaredToBounds(float x, float y, const Node& node)
{
    float dx = 0.0f;
    if (x < node.minX)
    {
        dx = node.minX - x;
    }
    else if (x > node.maxX)
    {
        dx = x - node.maxX;
    }

    float dy = 0.0f;
    if (y < node.minY)
    {
        dy = node.minY - y;
    }
    else if (y > node.maxY)
    {
        dy = y - node.maxY;
    }

    return (dx * dx) + (dy * dy);
}
