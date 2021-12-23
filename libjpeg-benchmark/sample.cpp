#include <cstdio>
#include <cstdint>
#include <cassert>
#include <vector>
#include <memory>
#include <string>
#include <turbojpeg.h>
#include <jpeglib.h>
#include <stdexcept>
#include <iostream>
#include <ctime>

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

inline void SaveFile(const std::string &filepath, const uint8_t *buffer, const size_t size)
{
    FILE *fp = fopen(filepath.c_str(), "wb+");
    assert(fp && "Unable to open file");

    fwrite(buffer, size, 1, fp);
    fclose(fp);
}

const char *Stringify(TJCS colorSpace)
{
#define CASE(x) case TJCS::TJCS_##x: return #x;
    switch (colorSpace)
    {
        CASE(RGB)
        CASE(YCbCr)
        CASE(GRAY)
        CASE(CMYK)
        CASE(YCCK)
    default:
         return "Undefined";
    }
#undef CASE
}

const char *Stringify(TJSAMP subSample)
{
#define CASE(x) case TJSAMP::TJSAMP_##x: return #x;
    switch (subSample)
    {
        CASE(444)
        CASE(422)
        CASE(420)
        CASE(GRAY)
        CASE(440)
        CASE(411)
    default:
        return "Undefined";
    }
#undef CASE
}

struct Jpeg
{
    void Setup()
    {
        Data.resize(Width * Height * 3);
    }

    int Width                = 0;
    int Height               = 0;
    TJSAMP SubSample         = TJSAMP::TJSAMP_420;
    TJCS ColorSpace          = TJCS::TJCS_GRAY;

    std::vector<uint8_t> Data;
};

class Timer
{
public:
    Timer()
    {
        start = clock();
    }

    ~Timer()
    {
        end = clock();
        std::cout << "Duration: " << ((double)end - (double)start) / CLOCKS_PER_SEC << std::endl;
    }

private:
    time_t start;
    time_t end;
};

struct Instance
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_decompress_struct dinfo;
};

int main(int argc, char **argv)
{
    // argv[1] = (char *)"/root/C/AMX-Benchmarks/images/1080p_sam.jpg";
    std::vector<uint8_t> buffer = ReadFileToBufer(argv[1]);
    tjhandle handle = tjInitDecompress();;

    Jpeg jpeg{};

    j_decompress_ptr dinfo;

    int ret = tjDecompressHeader3(
        handle,
        buffer.data(),
        buffer.size(),
        &jpeg.Width,
        &jpeg.Height,
        reinterpret_cast<int *>(&jpeg.SubSample),
        reinterpret_cast<int *>(&jpeg.ColorSpace)
        );

    dinfo = &((Instance *)handle)->dinfo;

    printf("ColorSpace=%s:Sample=%s:ProgressiveMode=%s:Width=%d:Height=%d\n", Stringify(jpeg.ColorSpace), Stringify(jpeg.SubSample), dinfo->progressive_mode ? "Yes" : "No", dinfo->image_width, dinfo->image_height);

    assert(!ret && "Unable to read header file! Check if it's a Jpeg file.");

    // jpeg.Setup();

    // {
    //     Timer timer{};
    //     for (int i = 0; i < 1000; i++)
    //     {
    //         ret = tjDecompress2(
    //             handle,
    //             buffer.data(),
    //             buffer.size(),
    //             jpeg.Data.data(),
    //             jpeg.Width,
    //             0,
    //             jpeg.Height,
    //             TJPF_RGB,
    //             0
    //             );
    //     }
    // }

    // ret = tjDecompress2(
    //     handle,
    //     buffer.data(),
    //     buffer.size(),
    //     jpeg.Data.data(),
    //     jpeg.Width,
    //     0,
    //     jpeg.Height,
    //     TJPF_RGB,
    //     0
    //     );

    // assert(!ret && "Unable to Decompress Jpeg file Check if it's a Jpeg file.");
    // SaveFile(argv[2], jpeg.Data.data(), jpeg.Data.size());

    // uint8_t *jpeg_buf = nullptr;
    // size_t jpeg_size = 0;
    // tjhandle compressHandle = tjInitCompress();
    // ret = tjCompress2(
    //     compressHandle,
    //     jpeg.Data.data(),
    //     jpeg.Width,
    //     0,
    //     jpeg.Height,
    //     TJPF_RGB,
    //     &jpeg_buf,
    //     &jpeg_size,
    //     TJSAMP_420,
    //     100,
    //     0
    // );

    // assert(!ret && "Failed to compress jpeg iamge");

    // SaveFile(std::string{ argv[2] }.append(std::string{ ".jpg" }), jpeg_buf, jpeg_size);

    tjDestroy(handle);

    return 0;
}
