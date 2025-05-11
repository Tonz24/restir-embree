#pragma once

#include "GBufferElement.h"
#include "HitInfo.h"
#include "PTInfo.h"
#include "Ray.h"
#include "Texture.h"


/*! \def NO_TEXTURES
\brief Maximální poèet textur pøiøazených materiálu.
*/
#define NO_TEXTURES 4

/*! \def IOR_AIR
\brief Index lomu vzduchu za normálního tlaku.
*/
#define IOR_AIR 1.000293f

/*! \def IOR_AIR
\brief Index lomu vody.
*/
#define IOR_WATER 1.33f

/*! \def IOR_GLASS
\brief Index lomu skla.
*/
#define IOR_GLASS 1.5f


class Material
{
public:	
	//! Implicitní konstruktor.
	/*!
	Inicializuje všechny složky materiálu na výchozí matnì šedý materiál.
	*/
	Material();

	//! Specializovaný konstruktor.
	/*!
	Inicializuje materiál podle zadaných hodnot parametrù.

	\param name název materiálu.
	\param ambient barva prostøedí.
	\param diffuse barva rozptylu.
	\param specular barva odrazu.
	\param emission  barva emise.
	\param shininess lesklost.
	\param ior index lomu.
	\param textures pole ukazatelù na textury.
	\param no_textures délka pole \a textures. Maximálnì \a NO_TEXTURES - 1.
	*/
	Material( std::string & name, const glm::vec3 & ambient, const glm::vec3 & diffuse,
		const glm::vec3 & specular, const glm::vec3 & emission, const float reflectivity,
		const float shininess, const float ior,
		Texture ** textures = NULL, const int no_textures = 0 );

	//! Destruktor.
	/*!
	Uvolní všechny alokované zdroje.
	*/
	virtual ~Material();

	//void Print();

	//! Nastaví název materiálu.
	/*!	
	\param name název materiálu.
	*/
	void set_name( const char * name );

	//! Vrátí název materiálu.
	/*!	
	\return Název materiálu.
	*/
	std::string get_name() const;

	//! Nastaví texturu.
	/*!	
	\param slot èíslo slotu, do kterého bude textura pøiøazena. Maximálnì \a NO_TEXTURES - 1.
	\param texture ukazatel na texturu.
	*/
	void set_texture( const int slot, Texture * texture );

	//! Vrátí texturu.
	/*!	
	\param slot èíslo slotu textury. Maximálnì \a NO_TEXTURES - 1.
	\return Ukazatel na zvolenou texturu.
	*/
	Texture * get_texture( const int slot ) const;

	virtual PTInfoGI evaluateLightingGI(const HitInfo& hitInfo, const Ray& ray) const;

	//same as evaluateLightingGI, but omega_i is known (direction to a point on a selected light source), so the result is just the BRDF value f_r
	virtual BRDFEval evaluateBRDF(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i) const;
	bool isEmitter() const;
	virtual float getPdfForSample(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i) const { return 0; }

	virtual bool recurse() const;
	virtual MaterialType getType() const;


	glm::vec3 ambient; /*!< RGB barva prostøedí \f$\left<0, 1\right>^3\f$. */
	glm::vec3 diffuse; /*!< RGB barva rozptylu \f$\left<0, 1\right>^3\f$. */
	glm::vec3 specular; /*!< RGB barva odrazu \f$\left<0, 1\right>^3\f$. */
	glm::vec3 attenuation;
	float roughness;

	glm::vec3 emission; /*!< RGB barva emise \f$\left<0, 1\right>^3\f$. */

	float shininess_; /*!< Koeficient lesklosti (\f$\ge 0\f$). Èím je hodnota vìtší, tím se jeví povrch lesklejší. */

	float reflectivity; /*!< Koeficient odrazivosti. */
	float ior; /*!< Index lomu. */

	static const char kDiffuseMapSlot; /*!< Èíslo slotu difuzní textury. */
	static const char kSpecularMapSlot; /*!< Èíslo slotu spekulární textury. */
	static const char kNormalMapSlot; /*!< Èíslo slotu normálové textury. */
	static const char kShininessMapSlot;


	glm::vec3 getDiffuseColor(const glm::vec2 & uv) const;
	glm::vec3 getSpecularColor(const glm::vec2& uv) const;
	float getShininess(const glm::vec2& uv) const;

	void setAssimpMatIndex(int index){
		assimpMatIndex = index;
	}

	int getAssimpMatIndex() const {
		return assimpMatIndex;
	}

	bool isEmissive() const {
		return emission.x + emission.y + emission.z > 0;
	}

protected:
	Texture * textures_[NO_TEXTURES]; /*!< Pole ukazatelù na textury. */
	/*
	slot 0 - diffuse map + alpha
	slot 1 - specular map + opaque alpha
	slot 2 - normal map
	slot 3 - transparency map
	*/
	std::string name_; /*!< Název materiálu. */
	int assimpMatIndex{};
};