#include <cpr/cpr.h>
#include <curl/curl.h>
#include <json/json.h>
#include <iostream>
#include <memory>
#include <regex>

int main() {
    cpr::Response r = cpr::Get(
        cpr::Url{"https://usosapps.uw.edu.pl/services/courses/search"},
        cpr::Parameters{{"lang", "pl"}, {"name", "Analiza matematyczna"}},
        cpr::Timeout{10000});
    if (r.status_code != 200) {
        std::cerr << "HTTP " << r.status_code << " " << r.error.message << "\n";
        return 1;
    }

    std::cout << "\nGot successful response from " << std::endl;

    Json::Value jsonData;
    Json::Reader jsonReader;

    if (!jsonReader.parse(r.text, jsonData)) {
        std::cerr << "NIEPOPRAWNY JSON\n";
        return 1;
    }
    const Json::Value& items = jsonData["items"];
    for (const Json::Value& item : items) {
        std::string id = item["course_id"].asString();
        std::string match = item["match"].asString();
        std::string clean =
            std::regex_replace(match, std::regex("<[^>]*>"), "");
        std::cout << id << " | " << clean << "\n";
    }
    std::cout << "Wpisz kod: ";
    std::string kod;
    std::cin >> kod;
    Json::Value selected;
    for (const Json::Value& item : items) {
        if (item["course_id"].asString() == kod) {
            selected = item;
        }
    }
    if (!selected.isNull()) {
        std::cout << "WYBRANO: " +
                         std::regex_replace(selected["match"].asString(),
                                            std::regex("<[^>]*>"), "") +
                         "\n";
    } else {
        std::cout << "NIE ZNALEZIONO KODU" << std::endl;
    }
    // std::cout << jsonData.toStyledString();
}
