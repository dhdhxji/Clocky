#include "cfg.hpp"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "sol/sol.hpp"
#include "sol/sol.hpp"
#include <fstream>
#include <vector>

extern const uint8_t serpent_script_start[] asm("_binary_serpent_lua_start");
extern const uint8_t serpent_script_end[]   asm("_binary_serpent_lua_end");

static const std::string CFG_TABLE_NAME = "config";


static void split_dot_separated_path(const std::string& path, std::vector<std::string> out) {
    const std::string delim = ".";
    
    auto start = 0U;
    auto end = path.find(delim);
    while (end != std::string::npos)
    {
        out.push_back( path.substr(start, end - start) );
        start = end + delim.length();
        end = path.find(delim, start);
    }
}

static sol::table_proxy<sol::basic_table_core<true, sol::reference> &, std::tuple<std::__cxx11::string>> get_item_ref(const std::string& path, sol::state& state) {
    std::vector<std::string> vpath;
    split_dot_separated_path(path, vpath);

    auto table = state[CFG_TABLE_NAME];
    for(int i = 0; i < vpath.size()-1; ++i) {
        const std::string& subpath = vpath[i];

        auto new_table = table[subpath];
        if(!new_table.valid()) {
            table[subpath] = sol::new_table();
            table = table[subpath];
        }
    }

    return table[ vpath[vpath.size()-1] ];
}

Cfg::Cfg(const std::string& cfgPath) {
    state = new sol::state;

    state->script(std::string((const char*)serpent_script_start));
    state->script_file(cfgPath);
}

Cfg::~Cfg() {
    delete state;
}

void Cfg::save() {
    save(loadPath);
}


void Cfg::save(const std::string& path) {
    auto table = (*state)["serpent"];
    if (!table.valid()) {
        throw std::runtime_error("Serpent not loaded!");
    }
    if (!(*state)[CFG_TABLE_NAME].valid()) {
        throw std::runtime_error(CFG_TABLE_NAME + " doesn't exist!");
    }

    std::ofstream out(path);

    out << CFG_TABLE_NAME << " = ";
    sol::function block = table["block"];
    std::string cont = block( (*state)[CFG_TABLE_NAME] );
    out << cont;
}

template<class T>
void Cfg::put(const std::string& path, const T& item) {
    auto table = get_item_ref(path, *state);
    table = item;
}

template<class T>
T Cfg::get(const std::string& path) {
    auto table = get_item_ref(path, *state);
    return table;
}


template int32_t Cfg::get<int32_t>(const std::string&);
template float Cfg::get<float>(const std::string&);
template double Cfg::get<double>(const std::string&);
template bool Cfg::get<bool>(const std::string&);
template std::string Cfg::get<std::string>(const std::string&);

template void Cfg::put<int32_t>(const std::string&, const int32_t&);
template void Cfg::put<float>(const std::string&, const float&);
template void Cfg::put<double>(const std::string&, const double&);
template void Cfg::put<bool>(const std::string&, const bool&);
template void Cfg::put<std::string>(const std::string&, const std::string&);