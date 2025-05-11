#ifndef VERTEX_H_
#define VERTEX_H_

#include "glm/glm.hpp"

/*! \def NO_TEXTURE_COORDS
\brief Poèet texturovacích souøadnic.
*/
#define NO_TEXTURE_COORDS 1

//class Surface;

/*! \struct Vertex
\brief Struktura popisující všechny atributy vertexu.

\author Tomáš Fabián
\version 1.0
\date 2013
*/
struct /*ALIGN*/ Vertex
{
public:
	glm::vec3 position; /*!< Pozice vertexu. */
	glm::vec3 normal; /*!< Normála vertexu. */
	glm::vec3 color; /*!< RGB barva vertexu <0, 1>^3. */
	glm::vec2 texture_coords[NO_TEXTURE_COORDS]; /*!< Texturovací souøadnice. */
	glm::vec3 tangent; /*!< První osa souøadného systému tangenta-bitangenta-normála. */

	//char pad[8]; // doplnìní na 64 bytù, mìlo by to mít alespoò 4 byty, aby se sem vešel 32-bitový ukazatel

	//! Výchozí konstruktor.
	/*!
	Inicializuje všechny složky vertexu na hodnotu nula.
	*/
	Vertex() { }

	//! Obecný konstruktor.
	/*!
	Inicializuje vertex podle zadaných hodnot parametrù.

	\param position pozice vertexu.
	\param normal normála vertexu.
	\param color barva vertexu.
	\param texture_coords nepovinný ukazatel na pole texturovacích souøadnic.
	*/
	Vertex( const glm::vec3& position, const glm::vec3& normal, const glm::vec3& color, const glm::vec3& tangent, const glm::vec2 * texture_coords = NULL );

	//void Print();
};

#endif
