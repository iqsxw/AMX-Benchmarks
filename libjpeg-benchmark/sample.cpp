#include <cstdio>
#include <cstdint>
#include <cassert>
#include <vector>
#include <memory>
#include <string>
#include <turbojpeg.h>
#include <jpeglib.h>

inline std::vector<uint8_t> ReadFileToBufer(const std::string &&filename)
{
    FILE *fp = fopen(filename.c_str(), "rb+");
    assert(fp && "Unable to open file");

    fseek(fp, 0, SEEK_END);
    auto size = ftell(fp);
    rewind(fp);

    std::vector<uint8_t> buffer;
    buffer.resize(size);

    fread(buffer.data(), 1, buffer.size(), fp);
    fclose(fp);

    return buffer;
}

struct Jpeg
{
    void Setup()
    {
        Data.reset(new uint8_t[Width * Height * 3]);
    }

    int Width                = 0;
    int Height               = 0;
    TJSAMP SubSample         = TJSAMP::TJSAMP_420;
    J_COLOR_SPACE ColorSpace = J_COLOR_SPACE::JCS_UNKNOWN;

    std::unique_ptr<uint8_t> Data;
};

int main()
{
    std::vector<uint8_t> buffer = ReadFileToBufer("/root/C/AMX-Benchmarks/1080p_sam.jpg");
    tjhandle handle = tjInitDecompress();;

    Jpeg jpeg{};

    int ret = tjDecompressHeader3(
        handle,
        buffer.data(),
        buffer.size(),
        &jpeg.Width,
        &jpeg.Height,
        reinterpret_cast<int *>(&jpeg.SubSample),
        reinterpret_cast<int *>(&jpeg.ColorSpace)
        );

    assert(!ret && "Unable to read header file! Check if it's a Jpeg file.");

    jpeg.Setup();
    for (int i = 0; i < 1000000; i++)
    {
        ret = tjDecompress2(
            handle,
            buffer.data(),
            buffer.size(),
            jpeg.Data.get(),
            jpeg.Width,
            0,
            jpeg.Height,
            TJPF_RGB,
            0
            );
    }

    return 0;
}
