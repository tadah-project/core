#ifndef ELEMENT_H
#define ELEMENT_H

#include <iostream>
#include <functional>
#include <string>

struct Element {
  private:
    void check(const std::string &symbol, const int Z);

    friend std::ostream& operator<<(std::ostream& os, const Element& element)
    {
      os << "Symbol: " << element.symbol << std::endl;
      os << "Atomic Number: " << element.Z << std::endl;
      return os;
    }

  public:
    std::string symbol;
    int Z;
    Element();
    Element(const std::string &symbol_);
    Element(const int Z_);
    Element(const std::string &symbol_, const int Z_, bool check_=true);
    bool operator==(const Element& other) const;
    bool operator<(const Element& other) const;
    bool operator>(const Element& other) const;
};
namespace std {
    template <>
    struct hash<Element>;
}

#endif
