/*
*	Copyright (C) 2015 by Liangliang Nan (liangliang.nan@gmail.com)
*	https://3d.bk.tudelft.nl/liangliang/
*
*	This file is part of Easy3D: software for processing and rendering
*   meshes and point clouds.
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


#include <easy3d/viewer/texture.h>


namespace easy3d {

    Texture::Texture()
        : id_(0)
        , dimension_(0)
    {
        sizes_[0] = 0;
        sizes_[1] = 0;
        sizes_[2] = 0;

        wrap_mode_ = REPEAT;
        filter_mode_ = MIPMAP;
    }


    Texture::~Texture() {
        if (id_ != 0) {
            glDeleteTextures(1, &id_);
            id_ = 0;
        }
    }


    int Texture::dimension() const {
        if (depth() > 1)	{ return 3; }
        if (height() > 1)	{ return 2; }
        if (width() > 0)	{ return 1; }
        return 0;
    }

  /*
    Texture* Texture::create_from_file(const std::string& file_name, WrapMode wrap, FilterMode filter) {
        if (file_name.length() == 0) {
            return nil;
        }
        Image::Ptr image = ImageIO::read(file_name);
        if (image.is_nil())
            return nil;

        return create_from_image(image, wrap, filter);
    }


    Texture* Texture::create_from_image(const Image* image, WrapMode wrap, FilterMode filter) {
        Texture* result = new Texture;
        if (result->create(image, wrap, filter))
            return result;
        else {
            delete result;
            return nil;
        }
    }


    bool Texture::create(const Image* image, WrapMode wrap, FilterMode filter) {
        int width = image->width();
        int height = image->height();
        int depth = image->depth();
        sizes_[0] = width;
        sizes_[1] = height;
        sizes_[2] = depth;

        switch (dimension()) {
        case 2:	target_ = GL_TEXTURE_2D;	break;
        case 3:	target_ = GL_TEXTURE_3D;	break;
        default: Logger::err("Texture") << "only 2D and 3D texture are supported" << std::endl;		return false;
        }

        Image::ComponentCoding component_coding = image->component_coding();
        if (component_coding != Image::BYTE) {
            Logger::err("Texture") << "only images of BYPE component coding are supported" << std::endl;
            return false;
        }
        Memory::pointer ptr = image->base_mem();
        Image::ColorCoding color_coding = image->color_coding();

        id_ = 0;
        glGenTextures(1, &id_);
        if (id_ == 0) {
            Logger::err("Texture") << "failed generating texture" << std::endl;
            return false;
        }

        filter_mode_ = filter;
        wrap_mode_ = wrap;

        glBindTexture(target_, id_);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        if (dimension() == 2) {
            if (color_coding == Image::RGB) {
                glTexImage2D(target_, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, ptr);
            }
            else if (color_coding == Image::RGBA) {
                glTexImage2D(target_, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
            }
            else if (color_coding == Image::INDEXED) {
                ptr = indexed_to_rgb(image, Image::RGB);
                glTexImage2D(target_, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, ptr);
                delete[] ptr;
            }
            else if (color_coding == Image::GRAY) {
                ptr = gray_scale_to_rgb(image, Image::RGB);
                glTexImage2D(target_, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, ptr);
                delete[] ptr;
            }
            else {
                Logger::err("Texture") << "image color encoding not supported" << std::endl;
                glDeleteTextures(1, &id_);
                glBindTexture(target_, 0);
                return false;
            }
        }
        else if (dimension() == 3) {
            if (color_coding == Image::RGB)
                glTexImage3D(target_, 0, GL_RGB8, width, height, depth, 0, GL_RGB, GL_UNSIGNED_BYTE, ptr);
            else if (color_coding == Image::RGBA)
                glTexImage3D(target_, 0, GL_RGBA8, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
            else if (color_coding == Image::INDEXED) {
                ptr = indexed_to_rgb(image, Image::RGB);
                glTexImage3DEXT(target_, 0, GL_RGB8, width, height, depth, 0, GL_RGB, GL_UNSIGNED_BYTE, ptr);
                delete[] ptr;
            }
            else if (color_coding == Image::GRAY)
                glTexImage3D(target_, 0, GL_LUMINANCE, width, height, depth, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ptr);
            else {
                Logger::err("Texture") << "image color encoding not supported" << std::endl;
                glDeleteTextures(1, &id_);
                glBindTexture(target_, 0);
                return false;
            }
        }

        set_parameters();

        glBindTexture(target_, 0);
        return true;
    }


    void Texture::set_parameters() {
        if (filter_mode_ == MIPMAP) {
            if (dimension() == 2) {
                glGenerateMipmap(GL_TEXTURE_2D);		mpl_debug_gl_error;
            }
            else if (dimension() == 3) {
                glGenerateMipmap(GL_TEXTURE_3D);		mpl_debug_gl_error;
            }

            // https://www.opengl.org/wiki/Common_Mistakes#Automatic_mipmap_generation
            // The magnification filter can't specify the use of mipmaps; only the minification filter can do that.
            glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);	mpl_debug_gl_error;
            glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);					mpl_debug_gl_error;

            // See: http://www.opengl.org/wiki/Common_Mistakes#Creating_a_complete_texture
            // turn off mip map filter or set the base and max level correctly.
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);		mpl_debug_gl_error;
            //// The default value for GL_TEXTURE_MAX_LEVEL is 1000
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1000);		mpl_debug_gl_error;
        }
        else { // filter_mode_ == LINEAR
            glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);		mpl_debug_gl_error;
            glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		mpl_debug_gl_error;
        }

        // https://www.opengl.org/wiki/Common_Mistakes#Automatic_mipmap_generation
        // Never use GL_CLAMP; what you intended was GL_CLAMP_TO_EDGE. Indeed, GL_CLAMP was removed from core GL 3.1+,
        // so it's not even an option anymore.
        GLint wrap_mode = (wrap_mode_ == REPEAT) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
        glTexParameteri(target_, GL_TEXTURE_WRAP_S, wrap_mode);		mpl_debug_gl_error;
        glTexParameteri(target_, GL_TEXTURE_WRAP_T, wrap_mode);		mpl_debug_gl_error;
        if (dimension() == 3)
            glTexParameteri(target_, GL_TEXTURE_WRAP_R, wrap_mode);	mpl_debug_gl_error;
    }


    void Texture::bind() {
        glActiveTexture(GL_TEXTURE0);		mpl_debug_gl_error;
        glBindTexture(target_, id_);		mpl_debug_gl_error;

        {
            //glEnable(GL_TEXTURE_2D);		// deprecated function since 3.0 version of OpenGL
            //glEnable(GL_ALPHA_TEST);		// deprecated function since 3.1 version of OpenGL
            //glAlphaFunc(GL_GREATER, 0);	// deprecated function since 3.1 version of OpenGL
        }
    }


    void Texture::unbind() {
        glActiveTexture(GL_TEXTURE0);	mpl_debug_gl_error;
        glBindTexture(target_, 0);		mpl_debug_gl_error;

        {
            //glDisable(GL_ALPHA_TEST);		// deprecated function since 3.1 version of OpenGL
            //glDisable(GL_TEXTURE_2D);		// deprecated function since 3.0 version of OpenGL
        }
    }


    Memory::pointer Texture::indexed_to_rgb(
        const Image* image,
        Image::ColorCoding coding_out)
    {
        if (coding_out != Image::RGB && coding_out != Image::RGBA) {
            Logger::err("Texture") << "indexed_to_rgb: only RGB and RGBA supported" << std::endl;
            return nil;
        }

        const Colormap* colormap = image->colormap();
        int nb_colors_ = colormap->size();
        mpl_debug_assert(nb_colors_ <= 512);

        Memory::byte i2r[512];
        Memory::byte i2g[512];
        Memory::byte i2b[512];
        Memory::byte i2a[512];
        for (int i = 0; i < nb_colors_; i++) {
            i2r[i] = colormap->color_cell(i).r();
            i2g[i] = colormap->color_cell(i).g();
            i2b[i] = colormap->color_cell(i).b();
            i2a[i] = colormap->color_cell(i).a();
        }

        int size = image->width() * image->height() * image->depth();
        int compnent = 3;	// default Image::RGB
        if (coding_out == Image::RGBA)
            compnent = 4;

        Memory::pointer buffer = new Memory::byte[size * compnent];
        Memory::pointer data_in = image->base_mem();
        if (coding_out == Image::RGB) {
            for (int i = 0; i < size; i++) {
                Memory::byte index = data_in[i];
                buffer[3 * i] = i2r[index];
                buffer[3 * i + 1] = i2g[index];
                buffer[3 * i + 2] = i2b[index];
            }
        }
        else if (coding_out == Image::RGBA) {
            for (int i = 0; i < size; i++) {
                Memory::byte index = data_in[i];
                buffer[4 * i] = i2r[index];
                buffer[4 * i + 1] = i2g[index];
                buffer[4 * i + 2] = i2b[index];
                buffer[4 * i + 3] = i2a[index];
            }
        }

        return buffer;
    }


    Memory::pointer Texture::gray_scale_to_rgb(
        const Image* image,
        Image::ColorCoding coding_out)
    {
        if (coding_out != Image::RGB && coding_out != Image::RGBA) {
            Logger::err("Texture") << "indexed_to_rgb: only RGB and RGBA supported" << std::endl;
            return nil;
        }

        int nb_colors = 256;

        Memory::byte i2r[512];
        Memory::byte i2g[512];
        Memory::byte i2b[512];
        Memory::byte i2a[512];
        for (int i = 0; i < nb_colors; i++) {
            Memory::byte c = Memory::byte(255.0 * float(i) / float(nb_colors - 1));
            i2r[i] = c;
            i2g[i] = c;
            i2b[i] = c;
            i2a[i] = c;
        }

        int size = image->width() * image->height() * image->depth();
        int compnent = 3;	// default Image::RGB
        if (coding_out == Image::RGBA)
            compnent = 4;

        Memory::pointer buffer = new Memory::byte[size * compnent];
        Memory::pointer data_in = image->base_mem();
        if (coding_out == Image::RGB) {
            for (int i = 0; i < size; i++) {
                Memory::byte index = data_in[i];
                buffer[3 * i] = i2r[index];
                buffer[3 * i + 1] = i2g[index];
                buffer[3 * i + 2] = i2b[index];
            }
        }
        else if (coding_out == Image::RGBA) {
            for (int i = 0; i < size; i++) {
                Memory::byte index = data_in[i];
                buffer[4 * i] = i2r[index];
                buffer[4 * i + 1] = i2g[index];
                buffer[4 * i + 2] = i2b[index];
                buffer[4 * i + 3] = i2a[index];
            }
        }

        return buffer;
    }

    */

} // namespace easy3d

