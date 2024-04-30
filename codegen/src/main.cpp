#include <broma.hpp>
#include <format>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <ranges>
#include <regex>
#include <unordered_map>
#include <set>
#include <fstream>

char const* cpp_begin = R"BALL(//Codegen
#include <json_types.hpp>
#include <Geode/Geode.hpp>
#include <Geode/loader/IPC.hpp>

using namespace geode::prelude;

template <typename T>
std::string convert_to_hex(T value) {
    std::stringstream ss;
    ss << std::hex << (long)value;
    return ss.str();
}

matjson::Value point_to_json(const CCPoint& point) {
    std::vector<matjson::Value> arr = { point.x, point.y };
    return arr;
}

template <typename T>
struct _not_void {
    using type = T;
};
template <>
struct _not_void<void> {
    using type = std::nullptr_t;
};

template <>
struct _not_void<TodoReturnPlaceholder> {
    using type = std::nullptr_t;
};

template <typename T>
using not_void = typename _not_void<T>::type;

template <typename T>
struct swap_todo;

template <typename R, typename J, typename ...Args>
struct swap_todo<R(J::*)(Args...)> {
    using type = void(J::*)(Args...);
};

template <typename R, typename ...Args>
struct swap_todo<R(*)(Args...)> {
    using type = void(*)(Args...);
};

template <typename R, typename J, typename ...Args>
struct swap_todo<R(J::*)(Args...) const> {
    using type = void(J::*)(Args...) const;
};

template <typename T>
using swap_todo_t = typename swap_todo<T>::type;


template <typename T>
struct remove_const_reference {
    using type = T;
};

template <typename T> requires (std::is_const_v<std::remove_reference_t<T>>)
struct remove_const_reference<T&> {
    using type = T;
};

template <typename T>
using remove_const_reference_t = typename remove_const_reference<T>::type;


template <typename T>
remove_const_reference_t<T> fromJson(matjson::Value const& val) {
    if constexpr (std::is_same_v<T, char const*>) {
        auto tmp = CCString::createWithFormat("%s", val.as_string().c_str());
        return tmp->getCString();
    }

    // if a non-const reference, turn into a pointer
    if constexpr (std::is_reference_v<T> && !std::is_const_v<std::remove_reference_t<T>>) {
        return *val.as<std::remove_reference_t<T>*>();
    } else {
        return val.as<std::remove_const_t<std::remove_reference_t<T>>>();
    }
}

template <typename T>
matjson::Value toJson(T const& val) {
    if constexpr (std::is_same_v<T, _ccColor3B>) {
        return std::vector {val.r, val.g, val.b};
    } else if constexpr (std::is_same_v<T, CCArray*>) {
        return matjson::Serialize<T>::to_json(val);
    } else if constexpr (std::is_pointer<T>::value) {
        //todo move to serialize
        if constexpr (std::is_base_of_v<CCObject, std::remove_pointer_t<T>>) {
            const_cast<std::remove_const_t<std::remove_pointer_t<T>>*>(val)->retain();
        }

        return convert_to_hex(val);
    } else {
        return val;
    }
}

template <typename T>
remove_const_reference_t<T> fromJsonArr(std::vector<matjson::Value>& val) {
    auto first = val[0];
    val.erase(val.begin());
    return fromJson<T>(first);
}

template <typename R, typename U, typename V, typename ...Args>
not_void<R> callMemberFn(U fn, V first, Args... args) {
    if constexpr (std::is_same_v<R, void>) {
        (first->*fn)(args...);
        return nullptr;
    } else if constexpr (std::is_same_v<R, TodoReturnPlaceholder>) {
        auto fn2 = reference_cast<swap_todo_t<U>>(fn);
        (first->*fn2)(args...);
        //reference_cast<int>(&(first->*fn)(args...));
        return nullptr;
    } else {
        return (first->*fn)(args...);
    }
}

// dont use void, just use nullptr
template <typename R, typename U, typename ...Args>
matjson::Value listenerBuild(ipc::IPCEvent* event, U fn) {
    auto args = (*event->messageData).as_array();

    if constexpr (std::is_member_function_pointer<U>::value) {
        return toJson(callMemberFn<R>(fn, fromJsonArr<Args>(args)...));
    } else {
        if constexpr (std::is_same_v<R, void> || std::is_same_v<R, TodoReturnPlaceholder>) {
            auto fn2 = reference_cast<swap_todo_t<U>>(fn);
            fn2(fromJsonArr<Args>(args)...);
            return nullptr;
        } else {
            return toJson(fn(fromJsonArr<Args>(args)...));
        }
    }
}

template <int32_t id>
struct ListenerClass {
    static void apply() {}
};

#define LISTEN(id, rt, function, ...) \
    constexpr int32_t num##id = __COUNTER__; \
    template <> \
    struct geode::modifier::ModifyDerive<ListenerClass<num##id>, cocos2d::CCSpriteFrameCache> { \
        public: static void apply() { \
            ipc::listen("bind-" #id, +[](ipc::IPCEvent* event) -> matjson::Value { \
                auto args = (*event->messageData).as_array(); \
                matjson::Value out; \
                bool done = false; \
                Loader::get()->queueInMainThread([&]() { \
                    out = listenerBuild<rt, decltype(function)__VA_OPT__(,) __VA_ARGS__>(event, function); \
                    done = true; \
                }); \
                while (!done) {} \
                return out; \
            }); \
        } \
    }; \
    template <> \
    struct ListenerClass<num##id> { \
        static void apply() { \
            geode::modifier::ModifyDerive<ListenerClass<num##id>, cocos2d::CCSpriteFrameCache>::apply(); \
            ListenerClass<num##id - 1>::apply(); \
        } \
    };

)BALL";

constexpr char const* cpp_end = R"BALL(
$execute {
    ListenerClass<__COUNTER__ - 1>::apply();
}
)BALL";

constexpr char const* enums[] = {
    "cocos2d::ccGLServerState",
    "cocos2d::ccTouchesMode",
    "cocos2d::enumKeyCodes",
    "cocos2d::CCTexture2DPixelFormat",
    "cocos2d::PopTransition",
    "IconType",
    "UnlockType",
    "DialogAnimationType",
    "DialogChatPlacement",
    "BoomListType",
    "cocos2d::CCTextAlignment",
    "cocos2d::CCImage::EImageFormat",
    "GauntletType",
    "MenuAnimationType",
    "TableViewCellEditingStyle",
    "DemonDifficultyType",
    "LikeItemType",
    "CommentError",
    "UpdateResponse",
    "cocos2d::tCCPositionType",
    "GameObjectType",
    "GJTimedLevelType",
    "UserListType",
    "CommentKeyType",
    "LevelLeaderboardType",
    "LevelLeaderboardMode",
    "CommentType",
    "GJScoreType",
    "GJHttpType",
    "SearchType",
    "StatKey",
    "ShopType",
    "PlayerButton",
    "CellAction",
    "GJRewardType",
    "SpecialRewardItem",
    "ChestSpriteState",
    "GJSongError",
    "GJMusicAction",
    "LeaderboardState",
    "GJDifficultyName",
};

constexpr char const* structs[] = {
    "cocos2d::ccColor3B",
    "cocos2d::ccColor4B",
    "cocos2d::ccColor4F",
    "cocos2d::_ccColor3B",
    "cocos2d::_ccColor4B",
    "cocos2d::_ccColor4F",
    "cocos2d::ccHSVValue",
    "cocos2d::_ccBlendFunc",
    "cocos2d::CCRect",
    "cocos2d::CCSize",
    "cocos2d::CCPoint",
    "cocos2d::CCAffineTransform",
    "cocos2d::CCIMEKeyboardNotificationInfo",
    "cocos2d::CCArray*"
};

/*constexpr char const* cpp_template = R"BALL(
    ipc::listen("{id}", +[](ipc::IPCEvent* event) -> matjson::Value {{
        auto args = (*event->messageData).as_array();
        return listenerBuild<{return}, decltype(&{function}){args}>(event, &{function});
    }});
)BALL";*/

constexpr char const* cpp_template = "LISTEN({id}, {return}, {function}{args})\n";
constexpr char const* cpp_cast_template = "static_cast<{return}({member}*)({args}){const}>(&{value})";

std::string parseFnCpp(broma::Class const& cls, broma::MemberFunctionProto* fn, size_t id) {

    std::string args;
    for (auto& arg : fn->args)
        args += ", " + arg.first.name;
    std::string stripped_args = args.empty() ? args : args.substr(2);

    if (!fn->is_static)
        args = ", " + cls.name + "*" + args;

    std::string member = fn->is_static ? "" : (cls.name + "::");
    std::string function = fmt::format(cpp_cast_template,
        fmt::arg("return", fn->ret.name),
        fmt::arg("member", member),
        fmt::arg("args", stripped_args),
        fmt::arg("value", cls.name + "::" + fn->name),
        fmt::arg("const", fn->is_const ? " const" : "")
    );

    return fmt::format(cpp_template,
        fmt::arg("id", id),
        fmt::arg("return", fn->ret.name),
        fmt::arg("function", function),
        fmt::arg("args", args)
    );
}

std::string parseCpp(broma::Root& root) {
    std::string out = cpp_begin;

    for (auto& cls : root.classes) {
        for (auto& field : cls.fields) {
            if (auto bind = field.get_as<broma::FunctionBindField>()) {
                if (bind->binds.mac == -1) {
                    continue;
                }
            }

            if (auto fn = field.get_fn()) {
                if (fn->type == broma::FunctionType::Normal) {
                    out += parseFnCpp(cls, fn, field.field_id);
                }
            }
        }
    }

    out += cpp_end;

    return out;
}

constexpr char const* python_begin = R"BALL(
import re
import json
import ctypes
from multimethod import multimethod

bind = ctypes.CDLL("libPyBind.dylib")
sendMessage = bind.sendMessage
sendMessage.restype = ctypes.c_char_p
sendMessage.argtypes = [ctypes.c_char_p]

def sendMessagePort(message, dat):
    jsdata = json.dumps({
        "mod": "camila314.pytest",
        "message": message,
        "data": dat
    })

    reply = sendMessage(jsdata.encode())
    return json.loads(reply.decode())

vector_re = re.compile(r"gd::vector<(.+)>")
def conv_to(conv_type, obj):
    # hotfixes
    conv_type = conv_type.replace("cocos2d::", "")
    if conv_type == conv_type == "CCArray*":
        conv_type = "gd::vector<CCObject*>"
    if conv_type == "TodoReturn" or conv_type == "void":
        conv_type = "None"

    if conv_type.startswith("gd::vector"):
        item = vector_re.match(conv_type)
        if not item:
            raise Exception("Failed to match vector type")
        if type(obj) != list:
            raise Exception("Object is not a list")

        return [conv_to(item.group(1), i) for i in obj]

    if conv_type.endswith("*"): # lol
        conv_type = conv_type[:-1]

    maybe_type = eval(conv_type)
    if type(maybe_type) == type:
        try:
            return maybe_type(obj)
        except:
            return maybe_type._from_ptr(obj)

def conv_from(conv_types, objs):
    # trol
    ret = []
    for obj in objs:
        try:
            ret.append(obj.ptr)
        except:
            ret.append(obj)
    return ret
)BALL";

constexpr char const* python_class_template = R"BALL(
class {name}({supers}):
    @classmethod
    def _from_ptr(cls, ptr):
        out = cls.__new__(cls)
        out.ptr = ptr
        return out
    def __del__(self):
        if "ptr" not in self.__dict__:
            return
        if issubclass(type(self), CCObject) or type(self) == CCObject:
            self.release()
    def __init__(self, item):
        self.ptr = item.ptr
        if issubclass(type(self), CCObject) or type(self) == CCObject:
            self.retain() )BALL";

constexpr char const* python_method_template = R"BALL(
    @multimethod
    def {name}({argobj}):
        return conv_to("{return}", sendMessagePort("bind-{id}", conv_from([{args}], [{argnum}])))
)BALL";

constexpr char const* python_superclass_template = R"BALL(
if "{name}" not in globals():
    class {name}:
        pass)BALL";


/*std::string stripQualifiers(std::string const& type) {
    std::regex re_template(R"(([\w:]+)<(.+)>)");
    std::smatch match;
    std::regex_match(type, match, re_template);
    if (match.size() > 1) {
        return std::string(match[1]) + "<" + stripQualifiers(match[2]) + ">";
    }

    std::regex re(R"(.*?((^[\w:]+\*?$|[\w:]*[A-Z]+[\w:]*\*?)))");
    match = {};
    std::regex_match(type, match, re);
    return match[1];
}*/

void recurse(std::unordered_map<std::string, broma::Class> const& mapped, std::string const& cls, std::vector<broma::Class>& out) {
    if (auto it = mapped.find(cls); it != mapped.end()) {
        out.push_back(it->second);
        for (auto& super : it->second.superclasses) {
            recurse(mapped, super, out);
        }
    }
}

std::vector<broma::Class> reorderClasses(broma::Root& root) {
    // sort by superclasses. this is so that we can define the superclasses before the classes that inherit from them
    std::unordered_map<std::string, broma::Class> mapped;
    for (auto& cls : root.classes) {
        mapped[cls.name] = cls;
    }

    std::vector<broma::Class> ordered;
    for (auto& cls : root.classes) {
        recurse(mapped, cls.name, ordered);
    }


    std::vector<broma::Class> out;
    for (auto& cls : ordered) {
        if (std::find(out.begin(), out.end(), cls) == out.end())
            out.push_back(cls);
    }

    return out;
}

std::string unqualify(std::string const& qualified) {
    auto pos = qualified.find_last_of(':');
    if (pos == std::string::npos)
        return qualified;
    return qualified.substr(pos + 1);
}

std::string parseFnPython(broma::Class const& cls, broma::MemberFunctionProto* fn, size_t id) {
    std::vector<std::string> args_v;
    std::vector<std::string> argnum_v;
    std::vector<std::string> argobj_v;

    if (!fn->is_static) {
        argnum_v.push_back("self");
        argobj_v.push_back("self");
    }

    int i = 0;
    for (auto& arg : fn->args) {
        args_v.push_back("\"" +  arg.first.name + "\"");
        argnum_v.push_back("arg" + std::to_string(i));
        argobj_v.push_back("arg" + std::to_string(i) + ": object");
        ++i;
    }

    auto args = fmt::join(args_v, ", ");
    auto argnum = fmt::join(argnum_v, ", ");
    auto argobj = fmt::join(argobj_v, ", ");

    return fmt::format(python_method_template,
        fmt::arg("name", fn->name),
        fmt::arg("return", unqualify(fn->ret.name)),
        fmt::arg("id", id),
        fmt::arg("args", args),
        fmt::arg("argnum", argnum),
        fmt::arg("argobj", argobj),

        fmt::arg("if_static", fn->is_static ? "@staticmethod" : "")
    );
}


std::string parsePython(broma::Root& root) {
    std::string out = python_begin;

    auto reordered = reorderClasses(root);
    for (auto& cls : reordered) {
        std::string supers;
        for (auto& super : cls.superclasses) {
            supers += unqualify(super) + ", ";
            out += fmt::format(python_superclass_template, fmt::arg("name", unqualify(super)));
        }
        if (!supers.empty()) {
            supers.pop_back();
            supers.pop_back();
        }

        out += fmt::format(python_class_template,
            fmt::arg("name", unqualify(cls.name)),
            fmt::arg("supers", supers)
        );

        std::vector<broma::MemberFunctionProto*> fns;
        for (auto& field : cls.fields) {
            if (auto bind = field.get_as<broma::FunctionBindField>()) {
                if (bind->binds.mac == -1) {
                    continue;
                }
            }
            if (auto fn = field.get_fn()) {
                if (fn->type == broma::FunctionType::Normal) {
                    fns.push_back(fn);
                    out += parseFnPython(cls, fn, field.field_id);
                }
            }
        }

        std::set<std::string> names;
        for (auto& i : fns) {
            if (i->is_static && names.find(i->name) == names.end()) {
                out += fmt::format("    {name} = staticmethod({name})\n", fmt::arg("name", i->name));
                names.emplace(i->name);
            }
        }
    }

    return out;
}

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
    std::ofstream cpp("main.cpp");
    std::ofstream py("pycreate.py");

    cpp << parseCpp(root);
    py << parsePython(root);
}
