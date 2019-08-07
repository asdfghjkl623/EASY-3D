
#include "image_io.h"
#include "image.h"
#include "image_serializer.h"
#include "image_serializer_bmp.h"
#include "image_serializer_jpeg.h"
#include "image_serializer_png.h"
#include "image_serializer_ppm.h"
#include "image_serializer_xpm.h"
#include <basic/logger.h>
#include <basic/file_utils.h>
#include <basic/string_algo.h>



Image* ImageIO::read(const std::string& file_name) {
	ImageSerializer::Ptr serializer = resolve_serializer(file_name);
	if (!serializer.is_nil())
		return serializer->serialize_read(file_name);
	else
		return nil;
}


bool ImageIO::save(const std::string& file_name, const Image* image) {
	ImageSerializer::Ptr serializer = resolve_serializer(file_name);
	if (!serializer.is_nil())
		return serializer->serialize_write(file_name, image);
	else
		return false;
}

ImageSerializer* ImageIO::resolve_serializer(const std::string& file_name) {
	std::string ext = FileUtils::extension(file_name) ;
	ext = String::to_lowercase(ext);
	if (ext.length() == 0) {
		Logger::err("ImageIO") << "No extension in file name" << std::endl ;
		return nil ;
	}

	ImageSerializer* serializer = nil;

	if (ext == "bmp")
		serializer = new ImageSerializer_bmp();
	else if (ext == "jpg")
		serializer = new ImageSerializer_jpeg();
	else if (ext == "png")
		serializer = new ImageSerializer_png();
	else if (ext == "ppm")
		serializer = new ImageSerializer_ppm();
	else if (ext == "xpm")
		serializer = new ImageSerializer_xpm();
	else {
		Logger::err("ImageIO") << "Unknown image file format \'" << ext << "\'" << std::endl;
		return nil;
	}

	return serializer;
}


bool ImageIO::query_image_size(const std::string& file_name, int& width, int& height) {
	ImageSerializer::Ptr serializer = resolve_serializer(file_name);
	if (!serializer.is_nil())
		return serializer->query_image_size(file_name, width, height);
	else
		return false;
}
