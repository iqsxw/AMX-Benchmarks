#pragma once

#include <iostream>
#include <array>

struct TileConfig
{
    TileConfig(uint8_t start_row = 0, std::array<uint16_t, 8> colsb = { 64, 64, 64, 64, 64, 64, 64, 64 }, std::array<uint8_t, 8> rows = { 16, 16, 16, 16, 16, 16, 16, 16 }) :
        palette_id{ 1 },
        start_row{ start_row },
        reserved_0{ 0 },
        reserved_1{ 0 },
        reserved_2{ 0 }
    {
        std::copy(colsb.begin(), colsb.end(), this->colsb);
        std::copy(rows.begin(), rows.end(), this->rows);
    }

    TileConfig(int m, int k, int n) :
        palette_id{ 1 },
        start_row{ start_row },
        reserved_0{ 0 },
        reserved_1{ 0 },
        reserved_2{ 0 }
    {

    }

    uint8_t  palette_id;
    uint8_t  start_row;
    uint8_t  reserved_0[14];
    uint16_t colsb[8];
    uint16_t reserved_1[8];
    uint8_t  rows[8];
    uint8_t  reserved_2[8];
};

struct Tile
{
    uint8_t buf[1024];
    int rows;
    int colsb;
};
