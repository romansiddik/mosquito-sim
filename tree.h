#ifndef TREE_H
#define TREE_H

// Enum to represent different types of trees
enum TreeType {
    PINE,    // A coniferous tree with a cone-like shape
    OAK,     // A broadleaf tree with a full, rounded canopy
    WILLOW   // A tree with drooping branches
};

// The updated drawTree function now accepts a TreeType
void drawTree(TreeType type, float x, float y, float z);

#endif // TREE_H
