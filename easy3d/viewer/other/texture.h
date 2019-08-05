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


#ifndef EASY3D_OPENGL_TEXTURE_H
#define EASY3D_OPENGL_TEXTURE_H


#include <string>

#include <easy3d/viewer/opengl.h>


namespace easy3d {

    class Image;

    class Texture
    {
    public:
        // currently only TEXTURE_2D and TEXTURE_3D are implemented
        enum Target { TEXTURE_2D, TEXTURE_3D /*, TEXTURE_CUBE_MAP, TEXTURE_BUFFER */ };

        enum WrapMode { REPEAT, CLAMP };
        enum FilterMode { MIPMAP, LINEAR };

    public:
        static Texture* create_from_file(const std::string& image_file, WrapMode wrap = REPEAT, FilterMode filter = MIPMAP);

        static Texture* create_from_image(const Image* image, WrapMode wrap = REPEAT, FilterMode filter = MIPMAP);

        static Texture* create_from_data(const unsigned char* data, WrapMode wrap = REPEAT, FilterMode filter = MIPMAP);

        int dimension() const;

        int width() const { return sizes_[0]; }
        int height() const { return sizes_[1]; }
        int depth() const { return sizes_[2]; }

        GLuint id() const { return id_; }
        GLenum target() const { return target_; }

        void bind();
        void unbind();

    protected:
        bool create(const Image* image, WrapMode wrap, FilterMode filter);

        void set_parameters();

//        unsigned char* indexed_to_rgb(const Image* image, Image::ColorCoding coding_out);
//        unsigned char* gray_scale_to_rgb(const Image* image, Image::ColorCoding coding_out);

    private:
        GLuint	id_;
        GLenum	target_;

        int dimension_;
        int sizes_[3];

        WrapMode	wrap_mode_;
        FilterMode	filter_mode_;

    private:
        Texture();
        ~Texture();
    };

} // namespace easy3d


#endif  // EASY3D_OPENGL_TEXTURE_H

