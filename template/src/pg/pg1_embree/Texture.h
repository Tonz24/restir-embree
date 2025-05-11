#ifndef TEXTURE_H_
#define TEXTURE_H_
#include "glm/glm.hpp"

/*! \class Texture
\brief Single texture.

\author Tomáš Fabián
\version 0.95
\date 2012-2018
*/


enum TextureInterp{
	NEAREST,
	BILINEAR
};

enum TextureClamp{
	CLAMP_TO_EDGE,
	REPEAT
};

class Texture
{
public:
	explicit Texture( const char * file_name, TextureInterp interpType = BILINEAR, TextureClamp clampType = CLAMP_TO_EDGE);
	~Texture();

	glm::vec3 get_texel( const int x, const int y ) const;
	glm::vec3 get_texel(const glm::vec2& uv) const;

	int width() const;
	int height() const;

	void compress();
	void expand();

	bool isValid(){
		return data_ && width_ > 0 && height_ > 0;
	}

private:		
	int width_{ 0 }; // image width (px)
	int height_{ 0 }; // image height (px)
	int scan_width_{ 0 }; // size of image row (bytes)
	int pixel_size_{ 0 }; // size of each pixel (bytes)
	BYTE * data_{ nullptr }; // image data in BGR format
	TextureInterp interpType;
	TextureClamp clampType;

	glm::vec3 getTexelNearestNeighbor(const glm::vec2& uv) const;
	glm::vec3 getTexelBilinear(const glm::vec2& uv) const;

};
#endif
