#pragma once
#include "glm/glm.hpp"

/*! \fn long long GetFileSize64( const char * file_name )
\brief Vrátí velikost souboru v bytech.
\param file_name Úplná cesta k souboru
*/
long long GetFileSize64( const char * file_name );

/*! \fn char * LTrim( char * s )
\brief Oøeže øetìzec o bílé znaky zleva.
\param s ukazatel na øetìzec
\return Ukazatel na novou pozici v tomtéž øetìzci
*/
char * LTrim( char * s );

/*! \fn char * RTrim( char * s )
\brief Oøeže øetìzec o bílé znaky zprava.
\param s ukazatel na øetìzec
\return Ukazatel na novou pozici v tomtéž øetìzci
*/
char * RTrim( char * s );

/*! \fn char * Trim( char * s )
\brief Oøeže øetìzec o bílé znaky z obou stran.
\param s ukazatel na øetìzec
\return Ukazatel na novou pozici v tomtéž øetìzci
*/
char * Trim( char *s );


class Utils{
public:

	//no instantiation of this class
	Utils() = delete;

	//gamma expands a single float value
	static void expand(float& u);
	//gamma compresses a single float value
	static void compress(float& u);
	//gamma expands an RGB color
	static void expand(glm::vec3& u);
	//gamma compresses an RGB color
	static void compress(glm::vec3& u);

	static void aces(glm::vec3& x);

	static float getRandomValue(float a = 0.0f, float b = 1.0f);

	static glm::vec3 orthogonal(const glm::vec3& vec);

	static float powerHeuristic(float pdf, float pdfOther){
		
		const float pdf_sqr = pdf * pdf;
		const float pdfOther_sqr = pdfOther * pdfOther;
		const float result = pdf_sqr / (pdfOther_sqr + pdf_sqr);
		return result;
	}

	static float maxComponent(const glm::vec3& vec) {
		return glm::max(glm::max(vec.x, vec.y), vec.z);
	}

	/**
	 * \brief
	 * \param ior1 index of refraction of original medium
	 * \param ior2 index of refraction of hit medium
	 * \param cosTheta dot product between view vector and surface normal
	 * \return reflection coefficient R(Theta)
	 */

	static float schlickApprox(const glm::vec3& incident, const glm::vec3& normal, float ior1, float ior2 = 1.0f);
	static glm::vec3 schlickApprox2(const glm::vec3& incident, const glm::vec3& normal, float ior1, float ior2 = 1.0f);
	static glm::vec3 schlickApprox3(const glm::vec3& incident, const glm::vec3& normal, const glm::vec3& F0);

	/*
	 * Rolls a value x in range [0,1].
	 * If x > max(throughput), the path should be terminated (returns -FLT_MAX)
	 * else it returns max(throughput) (don't forget to divide the throughput by this value to keep the estimator unbiased)
	 */
	static float rouletteTermination(const glm::vec3& throughput);

	/*
	 * Rolls a value x in range [0,1].
	 * If x > throughput, the path should be terminated (returns -FLT_MAX)
	 * else it returns throughput (don't forget to divide the throughput by this value to keep the estimator unbiased)
	 */
	static float rouletteTermination(float throughput);


	// returns {theta (azimuth), phi (polar), r (length)}
	static glm::vec3 cartesianToSpherical(const glm::vec3& pos);
	static glm::vec3 sphericalToCartesian(const glm::vec3& pos);

private:

	static std::mt19937 generator;
	static std::uniform_real_distribution<float> unifDist;
};