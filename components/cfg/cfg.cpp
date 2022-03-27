#include "cfg.hpp"

#include <iostream>
#include <fstream>
#include <exception>
#include <vector>
#include <cstdio>

using namespace nlohmann;
using namespace std;

static const std::string CFG_TABLE_NAME = "Config";


static void split_dot_separated_path(const std::string& path, std::vector<std::string>& out) {
    std::stringstream ss(path);
    std::string segment;

    while(std::getline(ss, segment, '.')) {
        out.push_back(segment);
    }
}


json& get_parent_ref_recursive(
    json& root, 
    vector<string>::iterator it,
    vector<string>::iterator end,
    bool eraseTables=false
) {
    if(it == end-1) {
        return root;
    }

    if(eraseTables){
        root = json::object();
    }

    return get_parent_ref_recursive(root[*it], it+1, end, eraseTables);
}


Cfg::Cfg(const string& cfgPath) {
    loadPath = cfgPath;

    // Create config file if it is does not exists
    FILE* f = fopen(cfgPath.c_str(), "ab+");
    if(f) {fclose(f);}

    try {
        ifstream(cfgPath) >> state;
    } 
    catch(exception& e) {
        cerr << "Exception occured while loading config file: " 
             << e.what() << endl;
    }
}

void Cfg::save() {
    save(loadPath);
}


void Cfg::save(const std::string& path) {
    std::ofstream(path) << state;
}

template<class T>
void Cfg::put(const std::string& path, const T& item) {
    vector<string> vpath;
    split_dot_separated_path(path, vpath);

    get_parent_ref_recursive(
        state[CFG_TABLE_NAME], 
        vpath.begin(), 
        vpath.end(), true
    )[*(vpath.end()-1)] = item;
}

template<class T>
T Cfg::get(const std::string& path) {
    vector<string> vpath;
    split_dot_separated_path(path, vpath);

    json& item = get_parent_ref_recursive(
        state[CFG_TABLE_NAME], vpath.begin(), vpath.end()
    );
    
    if( item.is_null() ) {
        throw std::runtime_error("Value " + path + " does not exists");
    }

    return item[*(vpath.end()-1)];
}

template<class T>
T Cfg::get(const std::string& path, const T& defaultValue) {
    try {
        return get<T>(path);
    } catch(std::runtime_error&) {
        return defaultValue;
    }
}


template int32_t Cfg::get<int32_t>(const std::string&);
template float Cfg::get<float>(const std::string&);
template double Cfg::get<double>(const std::string&);
template bool Cfg::get<bool>(const std::string&);
template std::string Cfg::get<std::string>(const std::string&);

template int32_t Cfg::get<int32_t>(const std::string&, const int32_t&);
template float Cfg::get<float>(const std::string&, const float&);
template double Cfg::get<double>(const std::string&, const double&);
template bool Cfg::get<bool>(const std::string&, const bool&);
template std::string Cfg::get<std::string>(const std::string&, const std::string&);

template void Cfg::put<int32_t>(const std::string&, const int32_t&);
template void Cfg::put<float>(const std::string&, const float&);
template void Cfg::put<double>(const std::string&, const double&);
template void Cfg::put<bool>(const std::string&, const bool&);
template void Cfg::put<std::string>(const std::string&, const std::string&);