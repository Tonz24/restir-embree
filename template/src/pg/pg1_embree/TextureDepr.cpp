#include "stdafx.h"
#include "TextureDepr.h"

#include <freeimage.h>

#include "raytracer.h"
#include "utils.h"

TextureDepr::TextureDepr( const char * file_name, TextureInterp interpType, TextureClamp clampType) : interpType(interpType), clampType(clampType){
	// image format
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	// pointer to the image, once loaded
	FIBITMAP * dib =  nullptr;
	// pointer to the image data
	BYTE * bits = nullptr;

	// check the file signature and deduce its format
	fif = FreeImage_GetFileType( file_name, 0 );
	// if still unknown, try to guess the file format from the file extension
	if ( fif == FIF_UNKNOWN )
	{
		fif = FreeImage_GetFIFFromFilename( file_name );
	}
	// if known
	if ( fif != FIF_UNKNOWN )
	{
		// check that the plugin has reading capabilities and load the file
		if ( FreeImage_FIFSupportsReading( fif ) )
		{
			dib = FreeImage_Load( fif, file_name );
		}
		// if the image loaded
		if ( dib )
		{
			// retrieve the image data
			bits = FreeImage_GetBits( dib );
			//FreeImage_ConvertToRawBits()
			// get the image width and height
			width_ = int( FreeImage_GetWidth( dib ) );
			height_ = int( FreeImage_GetHeight( dib ) );

			// if each of these is ok
			if ( ( bits != 0 ) && ( width_ != 0 ) && ( height_ != 0 ) )
			{				
				// texture loaded
				scan_width_ = FreeImage_GetPitch( dib ); // in bytes
				pixel_size_ = FreeImage_GetBPP( dib ) / 8; // in bytes

				data_ = new BYTE[scan_width_ * height_]; // BGR(A) format
				FreeImage_ConvertToRawBits( data_, dib, scan_width_, pixel_size_ * 8, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE );
			}

			FreeImage_Unload( dib );
			bits = nullptr;
		}
	}	
}

TextureDepr::~TextureDepr()
{	
	if ( data_ )
	{
		// free FreeImage's copy of the data
		delete[] data_;
		data_ = nullptr;
		
		width_ = 0;
		height_ = 0;
	}
}

glm::vec3 TextureDepr::get_texel( const int x, const int y ) const{	

	//clamp
	int c_x{}, c_y{};
	switch (clampType){
		case CLAMP_TO_EDGE:
			c_x = glm::clamp(x, 0, width_ - 1);
			c_y = glm::clamp(y, 0, height_ - 1);
			break;

		case REPEAT:
			c_x = glm::abs(x % width_);
			c_y = glm::abs(y % height_);
			break;
	}


	const int offset = c_y * scan_width_ + c_x * pixel_size_;
	
	if ( pixel_size_ > 4 * 1 ) // HDR, EXR
	{
		const float r = ( ( float * )( data_ + offset ) )[0];
		const float g = ( ( float * )( data_ + offset ) )[1];
		const float b = ( ( float * )( data_ + offset ) )[2];

		return glm::vec3{ r, g, b };
	}
	else // PNG, JPG etc.
	{
		const float b = data_[offset] / 255.0f;
		const float g = data_[offset + 1] / 255.0f;
		const float r = data_[offset + 2] / 255.0f;

		return glm::vec3{ r, g, b };
	}
}

glm::vec3 TextureDepr::get_texel(const glm::vec2& uv) const{
	switch (interpType){
	case NEAREST:
		return getTexelNearestNeighbor(uv);
	case BILINEAR:
		return getTexelBilinear(uv);
	}
}

int TextureDepr::width() const
{
	return width_;
}

int TextureDepr::height() const
{
	return height_;
}

void TextureDepr::compress(){
	for (int x = 0; x < width_; ++x) {
		for (int y = 0; y < height_; ++y) {
			const int offset = y * scan_width_ + x * pixel_size_;
			const BYTE pixel_b = data_[offset];
			float pixel_f = static_cast<float>(pixel_b) / 255.0f;

			Utils::compress(pixel_f);
			data_[offset] = static_cast<BYTE>(pixel_f * 255.0f);
		}
	}
}

void TextureDepr::expand(){

	for (int x = 0; x < width_ ; ++x) {
		for (int y = 0; y < height_ ; ++y) {

			const int offset_pixel = y * scan_width_ + x * pixel_size_;

			for (int c = 0; c < pixel_size_; ++c){
				const int total_offset = offset_pixel + c;

				const BYTE pixel_b = data_[total_offset];
				float pixel_f = static_cast<float>(pixel_b) / 255.0f;

				Utils::expand(pixel_f);

				data_[total_offset] = static_cast<BYTE>(pixel_f * 255.0f);
			}
		}
	}
}

glm::vec3 TextureDepr::getTexelNearestNeighbor(const glm::vec2& uv) const{

	const int x = std::max(0, std::min(width_ - 1, int(uv.x * width_)));
	const int y = std::max(0, std::min(height_ - 1, int(uv.y * height_)));

	return get_texel(x, y);
}

glm::vec3 TextureDepr::getTexelBilinear(const glm::vec2& uv) const {

	glm::vec2 pixelCoords = { uv.x * width_, (1 -uv.y) * height_ };

	glm::vec2 flooredCoords = glm::floor(pixelCoords);
	glm::vec2 t = pixelCoords - flooredCoords;

	//Q00
	glm::vec3 x0y0 = get_texel(flooredCoords.x, flooredCoords.y);
	//Q10
	glm::vec3 x1y0 = get_texel(flooredCoords.x + 1, flooredCoords.y);
	//Q01
	glm::vec3 x0y1 = get_texel(flooredCoords.x, flooredCoords.y + 1);
	//Q11
	glm::vec3 x1y1 = get_texel(flooredCoords.x + 1, flooredCoords.y + 1);

	//Q00 * t.x + (1 - t.x) * Q10
	glm::vec3 x1 = glm::mix(x0y0, x1y0, t.x);

	//Q01 * t.x + (1 - t.x) * Q11
	glm::vec3 x2 = glm::mix(x0y1, x1y1, t.x);

	//x1 * t.y + (1 - t.y) * x2
	return glm::mix(x1, x2, t.y);
}