#include "cfg.hpp"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "sol/sol.hpp"
#include "sol/sol.hpp"
#include <fstream>
#include <vector>
#include <cstdio>

extern const uint8_t serpent_script_start[] asm("_binary_serpent_lua_start");
extern const uint8_t serpent_script_end[]   asm("_binary_serpent_lua_end");

static const std::string CFG_TABLE_NAME = "Config";


static void split_dot_separated_path(const std::string& path, std::vector<std::string>& out) {
    std::stringstream ss(path);
    std::string segment;

    while(std::getline(ss, segment, '.')) {
        out.push_back(segment);
    }
}

static sol::object get_table_ref_recursively(
    sol::object object, 
    std::vector<std::string>::iterator it,
    std::vector<std::string>::iterator end,
    bool isCreateSubTables
) {
    if(it == end) {
        return object;
    }

    sol::object next = object.as<sol::table>()[*it];
    if(!next.valid() || !next.is<sol::table>()) {
        if(isCreateSubTables) {
            object.as<sol::table>()[*it] = sol::new_table();
        }
        else {
            throw std::runtime_error("Can not create subtable: read only table");
        }
    }

    return get_table_ref_recursively(object.as<sol::table>()[*it], it+1, end, isCreateSubTables);
}

static sol::table get_parent_table_ref(
    const std::string& path, 
    sol::state& state, 
    std::string& item_name, 
    bool createSubTables=false
) {
    if(path == "") {
        throw std::runtime_error("Path can not be empty");
    }
    
    std::vector<std::string> vpath;
    split_dot_separated_path(path, vpath);
    item_name = *(vpath.end()-1);

    if(!state[CFG_TABLE_NAME].valid()) {
        state[CFG_TABLE_NAME] = sol::new_table();
    }

    return get_table_ref_recursively(
        state[CFG_TABLE_NAME], 
        vpath.begin(),
        vpath.end()-1,
        createSubTables
    );
}

Cfg::Cfg(const std::string& cfgPath) {
    loadPath = cfgPath;
    state = new sol::state;
    state->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
    state->require_script("serpent", (const char*)serpent_script_start);

    // Create config file if it is does not exists
    FILE* f = fopen(cfgPath.c_str(), "ab+");
    if(f) {fclose(f);}

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
    std::string name;
    auto item_ref = get_parent_table_ref(path, *state, name, true);
    item_ref[name] = item;
}

template<class T>
T Cfg::get(const std::string& path) {
    std::string name;
    auto table = get_parent_table_ref(path, *state, name);
    if(!table[name].valid()) {
        throw std::runtime_error("Value " + path + " does not exists");
    }

    return table[name];
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