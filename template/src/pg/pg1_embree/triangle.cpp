#include "stdafx.h"
#include "triangle.h"

Triangle::Triangle( const Vertex & v0, const Vertex & v1, const Vertex & v2, Surface * surface ) : surface(surface){
	vertices_[0] = v0;
	vertices_[1] = v1;
	vertices_[2] = v2;	

	//assert( !is_degenerate() );

	centroid = (v0.position + v1.position + v2.position) / 3.0f;

	glm::vec3 u = vertices_[1].position - vertices_[0].position;
	glm::vec3 v = vertices_[2].position - vertices_[0].position;

	area = 0.5f * glm::length(glm::cross(u, v));
}

Vertex Triangle::vertex( const int i )
{
	return vertices_[i];
}

bool Triangle::is_degenerate() const
{
	return vertices_[0].position == vertices_[1].position ||
		vertices_[0].position == vertices_[2].position || 
		vertices_[1].position == vertices_[2].position;
}

const Surface& Triangle::getSurface() const {
	return *surface;
}

const glm::vec3& Triangle::getCentroid() const
{
	return centroid;
}