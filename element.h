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
            os << "Symbol: " << element.symbol[0] << element.symbol[1] << std::endl;
            os << "Atomic Number: " << element.Z << std::endl;
            return os;
        }

    public:
        char symbol[3];
        int Z;
        Element();
        Element(const char *symbol_, const int Z_, bool check_=true);
        bool operator==(const Element& other) const;
        bool operator<(const Element& other) const;
        bool operator>(const Element& other) const;
};
#endif
