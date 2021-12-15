#pragma once

#include <stdint.h>
#include <set>

struct Atom
{
    uint32_t size;
    uint32_t type;
    uint64_t largetSize;
    uint8_t  *data;
};

class AtomSet
{
public:
    AtomSet();

private:
    std::set<Atom> set;
};
