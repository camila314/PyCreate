#include <broma.hpp>
#include <format>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <ranges>
#include <regex>
#include <unordered_map>
#include <set>
#include <fstream>
#include <filesystem>

std::string parseCpp(broma::Root& root);
std::string parseCppHook(broma::Root& root);
std::string parsePython(broma::Root& root);

void pretreat(broma::Root& root) {
    for (auto& cls : root.classes) {
        std::vector<int> toRemove;
        for (int i = 0; i < cls.fields.size(); ++i) {
            if (auto fn = cls.fields[i].get_fn()) {
                if (fn->type == broma::FunctionType::Normal) {
                    for (auto& [k, _] : fn->args) {
                        if (k.name == "va_list" || k.name == "cocos2d::CCDataVisitor&") {
                            toRemove.push_back(i);
                        }
                    }
                }
            }
        }

        for (int i = toRemove.size() - 1; i >= 0; --i) {
            cls.fields.erase(cls.fields.begin() + toRemove[i]);
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    auto prev_path = std::filesystem::current_path();
    auto path = std::filesystem::path(argv[1]);
    std::filesystem::current_path(path.parent_path());

    auto root = broma::parse_file("Entry.bro");
    pretreat(root);

    std::filesystem::current_path(prev_path);
    std::ofstream cpp("bind.cpp");
    std::ofstream cpphook("hook.cpp");
    std::ofstream py("pycreate.py");

    cpp << parseCpp(root);
    cpphook << parseCppHook(root);
    py << parsePython(root);
}
