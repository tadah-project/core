#include <tadah/core/element.h>

void Element::check(const std::string &symbol, const int Z) {
  if (symbol.size() > 2)
    throw std::runtime_error("Wrong symbol: "+symbol);
  if (Z < 1)
    throw std::runtime_error("Atomic number Z < 1");
  if (Z > 118)
    throw std::runtime_error("Atomic number Z > 118");
}

Element::Element() {}
Element::Element(const std::string &symbol_, const int Z_, bool check_)
{
  if (check_) check(symbol_, Z_);
  symbol=symbol_;
  Z=Z_;
}
bool Element::operator==(const Element& other) const {
  return this->symbol[0] == other.symbol[0]
    && this->symbol[1] == other.symbol[1]
    && this->Z == other.Z;
}
bool Element::operator<(const Element& other) const {
  return this->Z < other.Z;
}
bool Element::operator>(const Element& other) const {
  return this->Z > other.Z;
}
