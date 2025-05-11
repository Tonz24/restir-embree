#include "stdafx.h"
#include "vertex.h"

Vertex::Vertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec3& color, const glm::vec3& tangent, const glm::vec2 * texture_coords) :
	position(position), normal(normal), color(color), tangent(tangent)
{
	if ( texture_coords != NULL )
	{
		for ( int i = 0; i < NO_TEXTURE_COORDS; ++i )
		{
			this->texture_coords[i] = texture_coords[i];
		}
	}	
}
