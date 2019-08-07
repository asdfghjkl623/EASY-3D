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


#ifndef EASY3D_IMAGE_IMAGE_H
#define EASY3D_IMAGE_IMAGE_H


#include <cassert>


namespace easy3d {


    class Image
    {
    public:
        Image() ;

        // nb_channels = 1: when color coding is GRAY or INDEXED
        // nb_channels = 3: when color coding is RGB or BGR
        // nb_channels = 4: when color coding is RGBA
        // each channel of a pixel is an unsigned char
        Image(int width, int height, int nb_channels = 3);

        virtual ~Image() ;

        int width() const  { return width_ ; }
        int height() const { return height_ ; }

        int nb_channels() const { return nb_channels_; }

        unsigned char* data() const {
            return data_ ;
        }

        void flip_vertically();

        void swap_channels(int channel1, int channel2);

    protected:
        unsigned char* data_ ;
        int width_, height_;
        int nb_channels_;

    private:
        //copying disabled
        Image(const Image&) ;
        Image& operator=(const Image&) ;
    } ;

}

#endif
