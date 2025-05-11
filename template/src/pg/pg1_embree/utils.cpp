#include "stdafx.h"
#include "Utils.h"

long long GetFileSize64( const char * file_name )
{
	FILE * file = fopen( file_name, "rb" );

	if ( file != NULL )
	{		
		_fseeki64( file, 0, SEEK_END ); // pøesun na konec souboru
		long long file_size = _ftelli64( file ); // zjištìní aktuální pozice
		_fseeki64( file, 0, SEEK_SET ); // pøesun zpìt na zaèátek
		fclose( file );
		file = NULL;

		return file_size;
	}

	return 0;	
}

void PrintTime( double t, char * buffer )
{
	// rozklad èasu
	int days = ( int )( t / ( 24.0 * 60.0 * 60.0 ) );
	int hours = ( int )( ( t - days * 24.0 * 60.0 * 60.0 ) / ( 60.0 * 60.0 ) );
	int minutes = ( int )( ( t - days * 24.0 * 60.0 * 60.0 - hours * 60.0 * 60.0 ) / 60.0 );
	double seconds = t - days * 24.0 * 60.0 * 60.0 - hours * 60.0 * 60.0 - minutes * 60.0;

	// ošetøení chybných stavù jako napø. 1m60s a pøevedení na korektní zápis 2m00s
	if ( seconds >= 59.5 )
	{
		seconds = 0.0;
		minutes++;
		if ( minutes == 60 )
		{
			minutes = 0;
			hours++;
			if ( hours == 24 )
			{
				hours = 0;
				days++;
			}
		}
	}

	// tisk èasu s odfiltrováním nulových poèáteèních hodnot 0d0h10m14s => 10m14s
	if ( days == 0 )
	{
		if ( hours == 0 )
		{
			if ( minutes == 0 )
			{
				if ( seconds < 1.0 )
				{
					sprintf( buffer, "%0.0f ms", seconds*1e+3 );
				}
				else
				{
					sprintf( buffer, "%0.1f s", seconds );
				}
			}
			else
			{
				sprintf( buffer, "%d m%02.0f s", minutes, seconds );
			}
		}
		else
		{
			sprintf( buffer, "%d h%02d m%02.0f s", hours, minutes, seconds );
		}
	}
	else
	{
		sprintf( buffer, "%d d%02d h%02d m%02.0f s", days, hours, minutes, seconds );
	}
}

std::string TimeToString( const double t )
{
	// rozklad èasu
	int days = static_cast<int>( t / ( 24.0 * 60.0 * 60.0 ) );
	int hours = static_cast<int>( ( t - days * 24.0 * 60.0 * 60.0 ) / ( 60.0 * 60.0 ) );
	int minutes = static_cast<int>( ( t - days * 24.0 * 60.0 * 60.0 - hours * 60.0 * 60.0 ) / 60.0 );
	double seconds = t - days * 24.0 * 60.0 * 60.0 - hours * 60.0 * 60.0 - minutes * 60.0;

	// ošetøení chybných stavù jako napø. 1m60s a pøevedení na korektní zápis 2m00s
	if ( seconds >= 59.5 )
	{
		seconds = 0.0;
		++minutes;

		if ( minutes == 60 )
		{
			minutes = 0;
			++hours;

			if ( hours == 24 )
			{
				hours = 0;
				++days;
			}
		}
	}

	char buffer[32] = { 0 };

	// tisk èasu s odfiltrováním nulových poèáteèních hodnot 0d0h10m14s => 10m14s
	if ( days == 0 )
	{
		if ( hours == 0 )
		{
			if ( minutes == 0 )
			{
				if ( seconds < 10 )
				{
					if ( seconds < 1 )
					{
						sprintf( buffer, "%0.1fms", seconds * 1e+3 );						
					}
					else
					{
						sprintf( buffer, "%0.1fs", seconds );
					}
				}
				else
				{
					sprintf( buffer, "%0.0fs", seconds );
				}
			}
			else
			{
				sprintf( buffer, "%dm%02.0fs", minutes, seconds );
			}
		}
		else
		{
			sprintf( buffer, "%dh%02dm%02.0fs", hours, minutes, seconds );
		}
	}
	else
	{
		sprintf( buffer, "%dd%02dh%02dm%02.0fs", days, hours, minutes, seconds );
	}

	return std::string( buffer );
}

char * LTrim( char * s )
{
    while ( isspace( *s ) || ( *s == 0 ) )
	{
		++s;
	};

    return s;
}

char * RTrim( char * s )
{
    char * back = s + strlen( s );

    while ( isspace( *--back ) );

    *( back + 1 ) = '\0';

    return s;
}

char * Trim( char * s )
{
	return RTrim( LTrim( s ) );
}

std::mt19937 Utils::generator{ 123 };
std::uniform_real_distribution<float> Utils::unifDist{ 0.0f,1.0f };

void Utils::compress(glm::vec3& u) {
	compress(u.x);
	compress(u.y);
	compress(u.z);
}

void Utils::expand(glm::vec3& u) {
	expand(u.x);
	expand(u.y);
	expand(u.z);
}

void Utils::aces(glm::vec3& x) {
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	x = glm::clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

float Utils::getRandomValue(float a, float b) {
	float val = unifDist(generator);
	return a + (b - a) * val;
}

glm::vec3 Utils::orthogonal(const glm::vec3& vec)
{
	return glm::abs(vec.x) > glm::abs(vec.z) ? glm::vec3{ vec.y, -vec.x, 0.0f } : glm::vec3{ 0.0f, vec.z, -vec.y };
}

void Utils::expand(float& u) {
	if (u <= 0.0f) 
		u = 0.0f;
	else if (u >= 1.0f)
		u = 1.0f;
	else if (u <= 0.04045f)
		u /=  12.92f;
	else
		u = powf((u + 0.055f) / 1.055f, 2.4f);
}

void Utils::compress(float& u)
{
	if (u <= 0.0f)
		u = 0.0f;
	else if (u >= 1.0f) 
		u = 1.0f;
	else if (u <= 0.0031308) 
		u *= 12.92f;
	else
		u = 1.055f * powf(u, 1.0f / 2.4f) - 0.055f;
}

float Utils::schlickApprox(const glm::vec3& incident, const glm::vec3& normal, float ior1, float ior2) {

	float F0 = (ior1 - ior2) / (ior1 + ior2);
	F0 *= F0;

	const float cosTheta = glm::max(glm::dot(-incident, normal), 0.0f);
	const float schlick = F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);

	return schlick;
}

glm::vec3 Utils::schlickApprox2(const glm::vec3& incident, const glm::vec3& normal, float ior1, float ior2){
	glm::vec3 F0 = glm::vec3{ glm::pow((ior1 - ior2) / (ior1 + ior2), 2.0f) };
	F0 *= F0;

	const float cosTheta = glm::max(glm::dot(-incident, normal), 0.0f);
	const glm::vec3 schlick = F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);

	return schlick;
}

glm::vec3 Utils::schlickApprox3(const glm::vec3& incident, const glm::vec3& normal, const glm::vec3& F0){
	const float cosTheta = glm::max(glm::dot(-incident, normal), 0.0f);
	const glm::vec3 schlick = F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);

	return schlick;
}

float Utils::rouletteTermination(const glm::vec3& throughput) {
	const float rrTerminator = Utils::getRandomValue(0.0f, 1.0f);
	const float contrib = maxComponent(throughput);

	return rrTerminator > contrib ? -FLT_MAX : contrib;
}

float Utils::rouletteTermination(float throughput) {
	const float rrTerminator = Utils::getRandomValue(0.0f, 1.0f);
	return rrTerminator > throughput ? -FLT_MAX : throughput;
}

glm::vec3 Utils::cartesianToSpherical(const glm::vec3& pos) {
	const float theta = atan2(pos.y, pos.x);
	const float phi = atan2(glm::sqrt(pos.x * pos.x + pos.y * pos.y), pos.z);
	const float r = glm::length(pos);

	return { theta, phi, r };
}

glm::vec3 Utils::sphericalToCartesian(const glm::vec3& pos) {
	const float theta = pos.x;
	const float phi = pos.y;
	const float r = pos.z;

	const float x = r * glm::cos(theta) * glm::sin(phi);
	const float y = r * glm::sin(theta) * glm::sin(phi);
	const float z = r * glm::cos(phi);

	return { x,y,z };
}
