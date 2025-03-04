#include <cstdint>
#include <iostream>

typedef struct {
    uint32_t value;
} Register;

typedef struct {
    uint32_t PC;
    bool PCSrc;
} IF;