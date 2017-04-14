#include <iostream>
#include <fstream>
#include <sstream>

#include "json11.hpp"

using std::string;

const string CONFIG_FILE = "cache.config";
const string CONFIGS[] = {"Number of sets", "Associativity", "Line size", "Cache coherence protocol", "Write strategy", "Interconnection", "Total processors"};

int main()
{
    std::ifstream fs(CONFIG_FILE);
    std::stringstream ss;
    ss << fs.rdbuf();
    string str;
    const auto json = json11::Json::parse(ss.str(), str);

    if (!str.empty()) {
        std::cerr << "Failed to load config file with content: \n" << ss.str() << std::endl;
    }

    std::cout << "Running simulator with the following configs:" << std::endl;
    for (const auto& item : CONFIGS)
    {
        string val = json[item].is_string() ? json[item].string_value() : std::to_string(json[item].int_value());
        std::cout << "- " << item << ":  " << val << std::endl;
    }
}
