#pragma once

#include <string>
#include <json.hpp>
#include <unordered_map>
#include <vector>
#include <UUID.h>

using json = nlohmann::json;

class Photomath {
public:
    Photomath();
    std::string parseEquation();
    std::string parseSolution();
    std::string parseChildren(const std::vector<json>& children, const std::string& type);
    void createRequest();

private:
    std::unordered_map<std::string, std::string> m_op_map{
        {"add", "+"},
        {"sub", "-"},
        {"frac", "/"},
        {"muli", "*"},
        {"mul", "*"},
        {"pow", "^"},
        // Bracket is (), we only have closing bracket as we write the opening beforehand.
        {"bracket", ")"},
        // Seen in integrals, C is an element of ℝ
        {"list", ","},
        {"elem_of", "E"}
    };

    UUID m_uuid{};
    String m_body{};
};
