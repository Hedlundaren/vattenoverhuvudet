#pragma once

#include <string>
#include <sstream>

#include "glm/glm.hpp"
#include "glm/gtx/string_cast.hpp"

inline std::string to_string(const std::vector<float> &vector, const std::string join_string = "") {
    std::stringstream ss;

    ss << "[";

    bool first = true;
    for (const float element : vector) {
        if (first) {
            ss << element;
            first = false;
            continue;
        }

        ss << join_string << element;
    }

    ss << "]";

    return ss.str();
}

inline std::string to_string(const std::vector<glm::vec3> &vector, const std::string join_string = "") {
    std::stringstream ss;

    ss << "[";

    bool first = true;
    for (const glm::vec3 element : vector) {
        if (first) {
            ss << glm::to_string(element);
            first = false;
            continue;
        }

        ss << join_string << glm::to_string(element);
    }

    ss << "]";

    return ss.str();
}