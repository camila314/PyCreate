#include <fmt/core.h>
#include <fmt/ranges.h>
#include <broma.hpp>
#include <unordered_map>
#include <set>

constexpr char const* python_begin = R"BALL(
import re
import json
import ctypes
from multimethod import multimethod
from http.server import *
from threading import Thread

_hooks = {}
_hookEnabled = False

class PyCreateHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        cont = json.loads(self.rfile.read(int(self.headers['Content-Length'])).decode())
        id = int(self.path[1:])

        if id in _hooks:
            for h in _hooks[id]:
                h(cont)

        self.send_response(200)
        self.end_headers()
    def log_message(self, format, *args):
        return

def enableHooks():
    _hookEnabled = True
    Thread(target=lambda: HTTPServer(('localhost', 3945), PyCreateHandler).serve_forever()).start()

bind = ctypes.CDLL("libPyBind.dylib")
sendMessage = bind.sendMessage
sendMessage.restype = ctypes.c_char_p
sendMessage.argtypes = [ctypes.c_char_p]

def sendMessagePort(message, dat):
    jsdata = json.dumps({
        "mod": "camila314.pycreate",
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
    @multimethod
    def hook_{name}({argobjnoself}):
        if _hookEnabled == False:
            enableHooks()
        def _h(func):
            if {id} not in _hooks:
                sendMessagePort("hook-{id}", [])
                _hooks[{id}] = []
            args = [{args}]
            if not {if_static}:
                args = ["{cls}*", *args]
            _hooks[{id}].append(lambda x: func(*[conv_to(a, b) for a, b in zip(args, x)]))
        return _h
)BALL";

constexpr char const* python_superclass_template = R"BALL(
if "{name}" not in globals():
    class {name}:
        pass)BALL";

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
    std::vector<std::string> argobjnoself_v;

    if (!fn->is_static) {
        argnum_v.push_back("self");
        argobj_v.push_back("self");
    }

    int i = 0;
    for (auto& arg : fn->args) {
        args_v.push_back("\"" +  arg.first.name + "\"");
        argnum_v.push_back("arg" + std::to_string(i));
        argobj_v.push_back("arg" + std::to_string(i) + ": object");
        argobjnoself_v.push_back("arg" + std::to_string(i) + ": object");
        ++i;
    }

    auto args = fmt::join(args_v, ", ");
    auto argnum = fmt::join(argnum_v, ", ");
    auto argobj = fmt::join(argobj_v, ", ");
    auto argobjnoself = fmt::join(argobjnoself_v, ", ");


    return fmt::format(python_method_template,
        fmt::arg("cls", cls.name),
        fmt::arg("name", fn->name),
        fmt::arg("return", unqualify(fn->ret.name)),
        fmt::arg("id", id),
        fmt::arg("args", args),
        fmt::arg("argnum", argnum),
        fmt::arg("argobj", argobj),
        fmt::arg("argobjnoself", argobjnoself),
        fmt::arg("argcomma", fn->args.empty() ? "" : ", "),
        fmt::arg("if_static", fn->is_static ? "True" : "False")
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
                #if __APPLE__
                    if (bind->binds.mac == -1)
                        continue;
                #else
                    if (bind->binds.win == -1)
                        continue;
                #endif
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
            if (names.find(i->name) == names.end()) {
                if (i->is_static)
                    out += fmt::format("    {name} = staticmethod({name})\n", fmt::arg("name", i->name));
                out += fmt::format("    hook_{name} = staticmethod(hook_{name})\n", fmt::arg("name", i->name));
                names.emplace(i->name);
            }
        }
    }

    return out;
}
