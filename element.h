#ifndef ELEMENT_H
#define ELEMENT_H

#include <iostream>
#include <functional>
#include <string>
#include <map>

struct Element {
    private:
        void check(const std::string &symbol, const int Z) {
            if (symbol.size() > 3)
                throw std::runtime_error("Wrong symbol: "+symbol);
            if (Z < 1)
                throw std::runtime_error("Atomic number Z < 1");
            if (Z > 118)
                throw std::runtime_error("Atomic number Z > 118");
        }

        friend std::ostream& operator<<(std::ostream& os, const Element& element)
        {
            os << "Name: " << element.name << std::endl;
            os << "Symbol: " << element.symbol << std::endl;
            os << "Atomic Number: " << element.Z << std::endl;
            return os;
        }

    public:
        std::string symbol;
        std::string name;
        int Z;
        Element() {};
        Element(const std::string &symbol, const std::string &name, const int Z):
            symbol(symbol),
            name(name),
            Z(Z)
    {
        check(symbol, Z);
    };
        bool operator==(const Element& other) const {
            return this->symbol == other.symbol && this->Z == other.Z;
        };
        bool operator<(const Element& other) const {
            return this->Z < other.Z;
        };

};

namespace std {
template <>
class hash<Element> {
 public:
  size_t operator()(const Element &e) const
  {
      std::hash<int> hash_f;
      return hash_f(e.Z);
  }
};
}
#endif
