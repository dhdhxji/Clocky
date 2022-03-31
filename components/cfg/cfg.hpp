#ifndef CFG_HPP
#define CFG_HPP

#include <string>
#include "nlohmann/json.hpp"

using nlohmann::json;

/**
 * @brief This library designed to store and receive configuration values
 * in json files. This library handles next value types:
 * - int32_t
 * - double, float
 * - std::string
 * - bool
 * 
 */


class Cfg {
public:
    Cfg(const std::string& cfgPath);

    void save();
    void save(const std::string& path);
    
    template<class T>
    void put(const std::string& path, const T& item);

    template<class T>
    T get(const std::string& path);

    template<class T>
    T get(const std::string& path, const T& defaultValue);

protected:
    std::string loadPath;
    json        state;
};

#endif // CFG_HPP
