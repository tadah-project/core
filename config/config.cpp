#include "config.h"
#include <sstream>

Config::Config()
{
    read_tadah_configuration();
    set_defaults();
}

Config::Config(std::string fn) {

    read_tadah_configuration();
    read(fn);
    set_defaults();
    postprocess();
}

void Config::read(std::string fn) {
    std::ifstream ifs(fn);
    if (!ifs.good())
        throw std::runtime_error("Configuration file does not exist:\n"+fn+"\n");
    std::string s;

    while(std::getline(ifs,s)) {
        std::istringstream iss(s);
        std::string key, value;
        iss >> key >> value;

        // skip empty lines
        if(key.empty()) continue;

        // skip comments starting with #
        if(key[0]=='#') continue;

        // enforce UPPERCASE KEY
        for (auto & k: key) k = std::toupper(k);

        // 1.0 Check is key allowed
        auto allowed_key = tdc[key];
        if (!!allowed_key==0)
            throw std::runtime_error("Disallowed key "+key+" "+value);
        // 2.0 Check if user is permitted to set this key
        if ((internal_keys.count(key)>0))
            throw std::runtime_error("User cannot set this key.\n\
                    This key is used internally\n\
                    and its value is calculated by the library "+key+" "+value);
        // 3.0 Check for duplicate keys
        int valN = *allowed_key["value_N"][0].value<int>();
        if (c[key].size() >= (size_t)abs(valN))
            throw std::runtime_error("Repeated key "+key+" "+value);

        // end of checks, proceed...
        c[key].push_back(value);
        while (iss >> value) {
            if (value[0]=='#') break;
            c[key].push_back(value);
        }
    }
    ifs.close();
}

void Config::postprocess() {
    // calculate and set max cutoff distance
    double c2b=0, c3b=0,cmb=0;
    int n=get<int>("OUTPREC");


    if (c.count("INIT2B") && (*this).get<bool>("INIT2B"))
        c2b=get<double>("RCUT2B");
    if (c.count("INIT3B") && (*this).get<bool>("INIT3B"))
        c3b=get<double>("RCUT3B");
    if (c.count("INITMB") && (*this).get<bool>("INITMB"))
        c3b=get<double>("RCUTMB");
    //if(c2b+c3b+cmb>std::numeric_limits<double>::min())
    c["RCUTMAX"]= std::vector<std::string>(1,to_string(std::max(cmb,std::max(c2b,c3b)),n));
    //else {
    //    throw std::runtime_error("RCUTMAX is zero. Check your settings.\n");
    //}
}

bool Config::check_for_training() {

    if ((*this).get<bool>("INIT2B")==false &&
        (*this).get<bool>("INIT3B")==false &&
        (*this).get<bool>("INITMB")==false) {
        throw std::runtime_error("At least one type must be initialised (true) \n\
                in a config file: INIT2B, INIT3B, INITMB.\n");
    }

    if (get<double>("RCUTMAX")<=0)
        throw std::runtime_error("RCUTMAX is zero. Check your settings.\n");

    if (!c.count("TYPE2B") && !c.count("TYPE3B")
            && !c.count("TYPEMB") )
        throw std::runtime_error("At least one type must be specified \n\
                in a config file: TYPE2B, TYPE3B, TYPEMB.\n");

    if (!c.count("RCTYPE2B") && !c.count("RCTYPE3B")
            && !c.count("RCTYPEMB") )
        throw std::runtime_error("At least one type must be specified \n\
                in a config file: RCTYPE2B, RCTYPE3B, RCTYPEMB.\n");

    return true;
}
bool Config::check_for_predict() {
    return true;
}
bool Config::check_pot_file() {
    return true;
}

void Config::set_defaults() {
    for (auto&& [k, v] : tdc) {
        std::string key = std::string(k.str().begin(), k.str().end());
        toml::array& arr = *tdc[key]["default"].as_array();
        for (auto &el : arr)
        {
            if (!c.count(key)) {
                if (el.is_string()) {
                    auto value = *el.value<std::string>();
                    c[key].push_back(value);
                }
                if (el.is_integer()) {
                    auto value = *el.value<int>();
                    c[key].push_back(to_string(value));
                }
                if (el.is_boolean()) {
                    auto value = *el.value<bool>();
                    c[key].push_back(value ? "true" : "false");
                }
                if (el.is_floating_point()) {
                    auto value = *el.value<double>();
                    c[key].push_back(to_string(value,16));
                }
            }
        }

    }
}
void Config::clear_internal_keys() {
    for (auto it = c.cbegin(); it != c.cend() /* not hoisted */; /* no increment */)
    {
        if (internal_keys.count(it->first)>0) {
            c.erase(it++);    // or "it = m.erase(it)" since C++11
        }
        else {
            ++it;
        }
    }
}

const std::vector<std::string>& Config::operator()(const std::string key) const{
    if (c.count(to_upper(key))==0)
        throw std::runtime_error(knf+to_upper(key));
    return c.at(to_upper(key));
}
size_t Config::size(const std::string key) const {
    if (c.count(to_upper(key))==0)
        throw std::runtime_error(knf+to_upper(key));
    return c.at(to_upper(key)).size();
}
bool Config::exist(const std::string key) const {
    return c.count(to_upper(key));
}

const std::map<std::string,int> Config:: internal_keys =  Config::create_internal_keys();
const std::string Config::knf = "Key not found: ";
bool Config::operator==(const Config &ctest) const {
    return c==ctest.c;
}
size_t Config::remove(const std::string key) {
    return c.erase(to_upper(key));
}
void Config::add(const std::string fn) {
    read(fn);
    set_defaults();
    postprocess();
}
int Config::read_tadah_configuration() {
    std::string config_keys="config_keys.toml";
    std::string config_dir= TADAH_CONFIG_DIR;
    fs::path cfile = config_dir;
    cfile /= config_keys;
    try
    {
        tdc = toml::parse_file(cfile.string());
    }
    catch (const toml::parse_error& err)
    {
        std::cerr << "Parsing of tadah configuration has failed:\n"
            << "This is a BUG!\n"
            << err << "\n";
        return 1;
    }
    return 0;
}
