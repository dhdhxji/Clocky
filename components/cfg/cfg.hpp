#ifndef CFG_HPP
#define CFG_HPP

#include <string>

namespace sol { class state;}

/**
 * @brief This library designed to store and receive configuration values
 * inside lua tables in file. This library handles next value types:
 * - int32_t
 * - double, float
 * - std::string
 * - bool
 * 
 */

template<class T>
class CfgValue {
public:

};

class Cfg {
public:
    Cfg(const std::string& cfgPath);
    ~Cfg();

    void save();
    void save(const std::string& path);
    
    template<class T>
    void put(const std::string& path, const T& item);

    template<class T>
    T get(const std::string& path);

protected:
    std::string loadPath;
    sol::state* state;
};

#endif // CFG_HPP
