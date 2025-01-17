/*
 * Copyright (c) 2008-2011 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <iostream>
#include "RayFile.h"

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>


RayFile::RayFile()
    :   m_fp(NULL),
        m_write(false),
        m_width(0),
        m_height(0)
{
}

RayFile::~RayFile()
{
    Close();
}

int RayFile::Open(const std::string &name)
{
    if (m_fp)
        Close();

    m_fp = fopen(name.c_str(), "rb");
    if (!m_fp)
        return -1;

    m_write = false;

    RayHeader header;

    if (fread(&header, sizeof(header), 1, m_fp) < 1) {
        Close();
        return -1;
    }

    if (header.magic != RAY_HEADER_MAGIC) {
        Close();
        return -1;
    }

    m_width = header.width;
    m_height = header.height;

//  std::cout << __PRETTY_FUNCTION__ << " width " << m_width << " height " << m_height << std::endl;

    return 0;
}

int RayFile::OpenWrite(const std::string &name, uint32_t width, uint32_t height)
{
    if (m_fp)
        Close();

    m_fp = fopen(name.c_str(), "wb+");
    if (!m_fp)
        return -1;

    m_write = true;

    RayHeader header;

    m_width = width;
    m_height = height;

    header.magic = RAY_HEADER_MAGIC;
    header.width = m_width;
    header.height = m_height;

    fwrite(&header, sizeof(header), 1, m_fp);

//  std::cout << __PRETTY_FUNCTION__ << " width " << m_width << " height " << m_height << std::endl;

    return 0;
}

void RayFile::Close()
{
    if (m_fp) {
        if (m_write) {
            // write EOF token
            uint32_t type = TYPE_EOF;
            fwrite(&type, sizeof(type), 1, m_fp);
        }

        fclose(m_fp);
        m_fp = NULL;
        m_write = false;
    }
}

void RayFile::SetXY(int x, int y, colorf color)
{
    // write a pixel packet
    if (m_fp && m_write) {
        uint32_t type = TYPE_PIXEL;

        fwrite(&type, sizeof(type), 1, m_fp);

        RayDataPixel pixel;
        pixel.x = x;
        pixel.y = y;
        pixel.r = color.r;
        pixel.g = color.g;
        pixel.b = color.b;
        fwrite(&pixel, sizeof(pixel), 1, m_fp);
    }
}

void RayFile::SetXYRun(int x, int y, int count, const colorf *color)
{
//  std::cout << __PRETTY_FUNCTION__ << " " << x << " " << y << " " << count << std::endl;
    if (m_fp && m_write) {
        uint32_t type = TYPE_PIXEL_RUN;

        fwrite(&type, sizeof(type), 1, m_fp);

        RayDataPixelRun run;

        run.x = x;
        run.y = y;
        run.length = count;
        fwrite(&run, sizeof(run), 1, m_fp);

        float colors[3];

        for (int i = 0; i < count; i++) {
            colors[0] = color[i].r;
            colors[1] = color[i].g;
            colors[2] = color[i].b;
            fwrite(colors, sizeof(colors), 1, m_fp);
        }
        fflush(m_fp);
    }
}

void RayFile::SetXYRun(int x, int y, int count, const float *color)
{
//  std::cout << __PRETTY_FUNCTION__ << " " << x << " " << y << " " << count << std::endl;
    if (m_fp && m_write) {
        uint32_t type = TYPE_PIXEL_RUN;

        fwrite(&type, sizeof(type), 1, m_fp);

        RayDataPixelRun run;

        run.x = x;
        run.y = y;
        run.length = count;
        fwrite(&run, sizeof(run), 1, m_fp);

        float colors[3];

        for (int i = 0; i < count; i++) {
            colors[0] = color[i*3];
            colors[1] = color[i*3+1];
            colors[2] = color[i*3+2];
            fwrite(colors, sizeof(colors), 1, m_fp);
        }
    }
}

static int ReadData(FILE *fp, void *_buf, size_t len)
{
    if (fread(_buf, len, 1, fp) < 1)
        return -1;

    return len;
}

int RayFile::Parse(int (*callback)(RayFile &ray, void *, int x, int y, const float rgb[3]), void *arg)
{
    if (!m_fp || m_write)
        return -1;

    for (;;) {
        uint32_t type;

        if (ReadData(m_fp, &type, sizeof(type)) < 0)
            goto done;
//      printf("type %d\n", type);

        switch (type) {
            case TYPE_PIXEL: {
                RayDataPixel pixel;

                if (ReadData(m_fp, &pixel, sizeof(pixel)) < 0)
                    goto done;

//              printf("pixel data: x %d y %d, r %f g %f b %f\n", pixel.x, pixel.y, pixel.r, pixel.g, pixel.b);

                float rgb[3] = { pixel.r, pixel.g, pixel.b };
                if (callback(*this, arg, pixel.x, pixel.y, rgb) < 0)
                    return -1;
                break;
            }
            case TYPE_PIXEL_RUN: {
                RayDataPixelRun run;

                if (ReadData(m_fp, &run, sizeof(run)) < 0)
                    goto done;

//              printf("pixel run data: x %d y %d, len %d\n", run.x, run.y, run.length);

                for (unsigned int i = 0; i < run.length; i++) {
                    float c[3];
                    if (ReadData(m_fp, &c, sizeof(c)) < 0)
                        goto done;

                    if (callback(*this, arg, run.x + i, run.y, c) < 0)
                        return -1;
                }
                break;
            }
            case TYPE_EOF:
                goto done;
            case TYPE_NULL:
            default:
                ;
        }

    }
done:
    printf("hit EOF\n");

    return 0;

}

static int BufferCallback(RayFile &file, void *_buf, int x, int y, const float rgb[3])
{
    float *buf = (float *)_buf;

    buf[(y * file.Width() + x) * 3] = rgb[0];
    buf[(y * file.Width() + x) * 3 + 1] = rgb[1];
    buf[(y * file.Width() + x) * 3 + 2] = rgb[2];

    return 0;
}

int RayFile::ReadIntoBuffer(float *buf)
{
    return Parse(&BufferCallback, (void *)buf);
}

int ReadIntoMmap(RayFile &file, const std::string &mmapfile, float **buf, uint64_t *len)
{
    // read in the input file into a memory mapped file
    int fd = open(mmapfile.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0)
        return -1;

    *len = file.GetPixelCount() * 3;

    int rc = ftruncate(fd, *len * sizeof(float));
    if (rc < 0) {
        goto err;
    }

    *buf = (float *)mmap(NULL, *len * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (!buf) {
        goto err;
    }

//  std::cout << "converting " << argv[1] << " to pure float array" << std::endl;
    if (file.ReadIntoBuffer(*buf) < 0) {
        goto err1;
    }

    munmap(buf, *len);
    close(fd);
    return 0;

err1:
    munmap(buf, *len);
err:
    close(fd);

    return -1;
}

// ***************
// TGA support
// ***************

struct TGACallbackArgs {
    FILE *fp;
    long dataOffset;
    long lastOffset;
};

static int TGACallback(RayFile &file, void *_buf, int x, int y, const float rgb[3])
{
    TGACallbackArgs *args = (TGACallbackArgs *)_buf;

    long seekto = args->dataOffset + (y * file.Width() + x) * 3;

    if (seekto != args->lastOffset) {
        fseek(args->fp, seekto, SEEK_SET);
        args->lastOffset = seekto;
    }

    color32 c32 = color32(colorf(rgb[0], rgb[1], rgb[2]));

    fwrite(&c32.b, 1, 1, args->fp);
    fwrite(&c32.g, 1, 1, args->fp);
    fwrite(&c32.r, 1, 1, args->fp);

    args->lastOffset += 3;

    return 0;
}

int ConvertToTGA(RayFile &file, const std::string &filename)
{
    FILE *fp;

    fp = fopen(filename.c_str(), "w+");
    if (!fp) {
        std::cerr << "error opening tga file " << filename << std::endl;
        return -1;
    }

    // TGA header
    unsigned char b;
    unsigned short s;

    b = 0; // identsize - none
    fwrite(&b, 1, 1, fp);
    b = 0; // colormaptype - none
    fwrite(&b, 1, 1, fp);
    b = 2; // imagetype - rgb
    fwrite(&b, 1, 1, fp);

    s = 0; // colormapstart - none
    fwrite(&s, 2, 1, fp);
    s = 0; // colormaplength - none
    fwrite(&s, 2, 1, fp);
    b = 0; // colormapbits - none
    fwrite(&b, 1, 1, fp);

    s = 0; // xstart
    fwrite(&s, 2, 1, fp);
    s = 0; // ystart
    fwrite(&s, 2, 1, fp);
    s = file.Width(); // width
    fwrite(&s, 2, 1, fp);
    s = file.Height(); // height
    fwrite(&s, 2, 1, fp);
    b = 24; // bits per pixel - 24
    fwrite(&b, 1, 1, fp);
    b = (1<<5); // descriptor bits - upper left
    fwrite(&b, 1, 1, fp);

    TGACallbackArgs args;

    args.fp = fp;
    args.dataOffset = ftell(fp);
    args.lastOffset = args.dataOffset;

    file.Parse(&TGACallback, (void *)&args);

    fclose(fp);

    return 0;
}


