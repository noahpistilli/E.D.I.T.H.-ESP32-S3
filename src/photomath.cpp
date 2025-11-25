#include "photomath.h"
#include <Arduino.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <esp_camera.h>


std::string Photomath::parseEquation() {
    auto j = json::parse(m_body);
    auto inp = j["normalizedInput"];
    auto node = inp["node"];

    /*
     * Just a binary tree.
     * // Operation
     * "type": "add"
     * "children": list of current structure repeating
     */
    std::string type = node["type"].get<std::string>();
    auto children = node["children"].get<std::vector<json>>();
    return parseChildren(children, type);
}

std::string Photomath::parseSolution() {
    auto j = json::parse(m_body);
    auto inp = j["preview"];
    auto solution = inp["solution"];

    // Can be one solution
    std::string type = solution["type"].get<std::string>();
    if (type == "const") {
        return solution["value"].get<std::string>();
    }

    auto children = solution["children"].get<std::vector<json>>();
    return parseChildren(children, type);
}


std::string Photomath::parseChildren(const std::vector<json>& children, const std::string& type) {
    std::string ret{};
    if (type == "bracket") {
        ret += "(";
    } else if (type == "integral") {
        ret += "int(";
    }

    for (int i = 0; i < children.size(); i++) {
        auto child = children[i];
        std::string value{};
        std::string _type = child["type"].get<std::string>();
        if (m_op_map.find(_type) != m_op_map.end()) {
            // Has children.
            auto _children = child["children"].get<std::vector<json>>();
            value = parseChildren(_children, _type);
        } else {
            // Else it is some value in the expression. (constant or variable)
            // If the parent is an integral, append Leibniz notation.
            if (type == "integral") {
                value = "d";
                value += child["value"].get<std::string>();
            } else {
                value = child["value"].get<std::string>();

                // Remove chalkboard R for ASCII R.
                if (value == "ℝ") {
                    value = "R";
                }
            }
        }

        ret += value;
        if (i == children.size() - 1) {
            // If it is a bracket or integral we do have to end it here
            if (type == "bracket" || type == "integral") {
                ret += ")";
            }

            // Don't format the string further
            break;
        }

        ret +=  " ";
        ret += m_op_map[type] + " ";
    }

    return ret;
}

bool Photomath::createRequest() {
    // ESP has no built-in library to handle multipart/form-data. Here we go.
    // Photomath likes the boundary to be 'BOUNDARY'. RFC says it can be any combination of ASCII but whatever
    WiFiClient client;
    String boundary = "BOUNDARY";
    camera_fb_t* fb = esp_camera_fb_get();

    String head =
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"image.jpg\"\r\n"
        "Content-Type: image/jpeg\r\n\r\n";

    String tail = "\r\n--" + boundary + "--\r\n";

    size_t totalLen = head.length() + fb->len + tail.length();

    if (!client.connect("192.168.50.232", 80)) {
        Serial.println(client.connected());
        Serial.println("HTTP begin failed");
        return false;
    }

    client.println("POST / HTTP/1.1");
    client.println("Host: 192.168.50.232");
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.println("Content-Length: " + String(totalLen));
    client.println();
    client.print(head);
    client.write(fb->buf, fb->len);
    client.print(tail);

    esp_camera_fb_return(fb);
    bool headers_ended = false;
    unsigned long timeout = millis();

    while (client.connected() && millis() - timeout < 5000) {
        while (client.available()) {
            String line = client.readStringUntil('\n');

            // End of headers is a blank line
            if (!headers_ended) {
                if (line == "\r" || line.length() == 0) {
                    headers_ended = true;
                }
            } else {
                // This is the body
                Serial.println("da body");
                m_body += line;
            }

            timeout = millis();
        }

        delay(1);
    }

    client.stop();

    // Parse JSON and determine success.
    auto j = json::parse(m_body);
    return j["success"].get<bool>();
}

