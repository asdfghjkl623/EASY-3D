/*
*	Copyright (C) 2015 by Liangliang Nan (liangliang.nan@gmail.com)
*	https://3d.bk.tudelft.nl/liangliang/
*
*	This file is part of Easy3D. If it is useful in your research/work,
*   I would be grateful if you show your appreciation by citing it:
*   ------------------------------------------------------------------
*           Liangliang Nan.
*           Easy3D: a lightweight, easy-to-use, and efficient C++
*           library for processing and rendering 3D data. 2018.
*   ------------------------------------------------------------------
*
*	Easy3D is free software; you can redistribute it and/or modify
*	it under the terms of the GNU General Public License Version 3
*	as published by the Free Software Foundation.
*
*	Easy3D is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#include <easy3d/image/image.h>

#include <algorithm>


namespace easy3d {


    Image::Image()
        : data_(nullptr)
        , width_(0)
        , height_(0)
        , nb_channels_(0)
    {
    }


    Image::Image(int width, int height, int nb_channels)
        : data_(nullptr)
        , width_(width)
        , height_(height)
        , nb_channels_(nb_channels)
    {
        data_ = new unsigned char[width_ * height_ * nb_channels_];
    }


    Image::~Image() {
        delete[] data_;
        width_ = 0;
        height_ = 0;
        nb_channels_ = 0;
    }


    void Image::flip_vertically() {
        int bpp = nb_channels();
        int h = height();
        int w = width();
        int row_len = w * bpp;
        for (int j = 0; j < h / 2; j++) {
            // get a pointer to the two lines we will swap
            unsigned char* row1 = data_ + j * row_len;
            unsigned char* row2 = data_ + (h - 1 - j) * row_len;
            // for each point on line, swap all the channels
            for (int i = 0; i < w; i++) {
                for (int k = 0; k < bpp; k++) {
                    std::swap(row1[bpp*i + k], row2[bpp*i + k]);
                }
            }
        }
    }


    void Image::swap_channels(int channel1, int channel2) {
        int bpp = nb_channels();
        if (bpp < 3) // e.g., gray scale image
            return;

        int bytes_per_comp = 1;
        assert(channel1 < nb_channels());
        assert(channel2 < nb_channels());
        int nb_pix = width_ * height_;
        unsigned char* pixel_base = data_;
        int channel1_offset = bytes_per_comp*channel1;
        int channel2_offset = bytes_per_comp*channel2;
        for (int i = 0; i < nb_pix; ++i) {
            unsigned char* channel1_pos = pixel_base + channel1_offset;
            unsigned char* channel2_pos = pixel_base + channel2_offset;
            std::swap(*channel1_pos, *channel2_pos);
            pixel_base += bpp;
        }
    }

}
