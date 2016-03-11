#pragma once

#include <vector>
#include <memory>
#include "glm/glm.hpp"

struct HeightMapData {
    HeightMapData(std::shared_ptr<const std::vector<glm::vec3>> heightmap) : hmap(heightmap) { }

    const std::shared_ptr<const std::vector<glm::vec3>> hmap;

    inline size_t kdtree_get_point_count() const {
        return hmap->size();
    }

    // Returns the distance between the vectors
    inline float kdtree_distance(const float *p1, const size_t idx_p2, size_t /*size*/) const {
        const glm::vec3 p1_vec(p1[0], p1[1], p1[2]);
        return glm::length(p1_vec - hmap->at(idx_p2));
    }

    inline float kdtree_get_pt(const size_t idx, int dim) const {
        return hmap->at(idx)[dim];
    }

    template<class BBOX>
    bool kdtree_get_bbox(BBOX & /*bb*/) const {
        return false;
    }
};