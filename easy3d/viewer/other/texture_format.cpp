#include "texture_format.h"


TextureFormat::TextureFormat()
	: target_(GL_TEXTURE_2D)
	, internal_format_(GL_RGBA8)
	, min_filter_(GL_NEAREST)
	, mag_filter_(GL_NEAREST)
	, wrap_s_(GL_CLAMP_TO_EDGE)
	, wrap_t_(GL_CLAMP_TO_EDGE)
{
}


TextureFormat::TextureFormat(const TextureFormat& other) 
	: target_(other.target())
	, internal_format_(other.internal_format())
	, min_filter_(other.min_filter())
	, mag_filter_(other.mag_filter())
	, wrap_s_(other.wrap_s())
	, wrap_t_(other.wrap_t())
{
}


TextureFormat& TextureFormat::operator=(const TextureFormat& other)
{
	target_ = other.target();
	internal_format_ = other.internal_format();
	min_filter_ = other.min_filter();
	mag_filter_ = other.mag_filter();
	wrap_s_ = other.wrap_s();
	wrap_t_ = other.wrap_t();
	return *this;
}


bool TextureFormat::operator==(const TextureFormat& other)
{
	return 	target_ == other.target() &&
	internal_format_ == other.internal_format() &&
	min_filter_ == other.min_filter() &&
	mag_filter_ == other.mag_filter() &&
	wrap_s_ == other.wrap_s() &&
	wrap_t_ == other.wrap_t();
}


TextureFormat::~TextureFormat() {}

// NOTE: now only GL_TEXTURE_2D and GL_TEXTURE_3D are supported. Default is GL_TEXTURE_2D.
void   TextureFormat::set_target(GLenum target) { target_ = target; }
GLenum TextureFormat::target() const { return target_; }

void   TextureFormat::set_internal_format(GLenum internal_format) { internal_format_ = internal_format; }
GLenum TextureFormat::internal_format() const { return internal_format_; }

// Sets the wrapping behavior when a texture coordinate falls outside the range of [0,1]. 
// Possible values are GL_REPEAT, GL_CLAMP_TO_EDGE, etc. Default is GL_CLAMP_TO_EDGE.
void   TextureFormat::set_wrap(GLenum wrap_s, GLenum wrap_t) { wrap_s_ = wrap_s; wrap_t_ = wrap_t; }
void   TextureFormat::set_wrap_s(GLenum wrap_s) { wrap_s_ = wrap_s; }
void   TextureFormat::set_wrap_t(GLenum wrap_t) { wrap_t_ = wrap_t; }

GLenum TextureFormat::wrap_s() const { return wrap_s_; }
GLenum TextureFormat::wrap_t() const { return wrap_t_; }

// Sets the filtering behavior when a texture is displayed at a lower resolution than its 
// native resolution. Default is GL_LINEAR unless mipmapping is enabled, in which case 
// GL_LINEAR_MIPMAP_LINEAR
void   TextureFormat::set_min_filter(GLenum min_filter) { min_filter_ = min_filter; }
// Sets the filtering behavior when a texture is displayed at a higher resolution than its 
// native resolution. Default is GL_LINEAR.
void   TextureFormat::set_mag_filter(GLenum mag_filter) { mag_filter_ = mag_filter; }
void   TextureFormat::set_filter(GLenum min_filter, GLenum mag_filter) { min_filter_ = min_filter; mag_filter_ = mag_filter; }

GLenum TextureFormat::min_filter() const { return min_filter_; }
GLenum TextureFormat::mag_filter() const { return mag_filter_; }