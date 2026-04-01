#pragma once

#include <cstddef>
#include <vector>

// A lightweight binary-tree view over the active logo set.
// It keeps the scene loosely ordered while still allowing spatial queries.
struct LogoNodeInput
{
    std::size_t index = 0;
    float x = 0.0f;
    float y = 0.0f;
    float scale = 1.0f;
};

class LogoNodeTree
{
public:
    void Clear();
    void Rebuild(const std::vector<LogoNodeInput>& items);
    int FindNearest(float x, float y) const;
    void GatherWithinRadius(float x, float y, float radius, std::vector<std::size_t>& outIndices) const;

    template <typename Visitor>
    void TraverseInOrder(Visitor&& visitor) const
    {
        TraverseInOrderImpl(rootIndex_, visitor);
    }

    bool empty() const { return nodes_.empty(); }

private:
    struct Node
    {
        LogoNodeInput item{};
        int left = -1;
        int right = -1;
        float minX = 0.0f;
        float maxX = 0.0f;
        float minY = 0.0f;
        float maxY = 0.0f;
    };

    int BuildRecursive(std::vector<LogoNodeInput> items, int depth);
    void GatherWithinRadiusImpl(int index, float x, float y, float radiusSquared, std::vector<std::size_t>& outIndices) const;
    int FindNearestImpl(int index, float x, float y, int bestIndex, float& bestDistanceSquared) const;
    static float DistanceSquared(float ax, float ay, float bx, float by);
    static float DistanceSquaredToBounds(float x, float y, const Node& node);

    template <typename Visitor>
    void TraverseInOrderImpl(int index, Visitor&& visitor) const
    {
        if (index < 0)
        {
            return;
        }

        const Node& node = nodes_[static_cast<std::size_t>(index)];
        TraverseInOrderImpl(node.left, visitor);
        visitor(node.item);
        TraverseInOrderImpl(node.right, visitor);
    }

    std::vector<Node> nodes_;
    int rootIndex_ = -1;
};
