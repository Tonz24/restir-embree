#include "stdafx.h"
#include "tutorials.h"
#include "raytracer.h"

/* error reporting function */
void error_handler( void * user_ptr, const RTCError code, const char * str )
{
	if ( code != RTC_ERROR_NONE )
	{
		std::string descr = str ? ": " + std::string( str ) : "";

		switch ( code ) {

		case RTC_ERROR_UNKNOWN: throw std::runtime_error( "RTC_ERROR_UNKNOWN" + descr );
		case RTC_ERROR_INVALID_ARGUMENT: throw std::runtime_error( "RTC_ERROR_INVALID_ARGUMENT" + descr ); break;
		case RTC_ERROR_INVALID_OPERATION: throw std::runtime_error( "RTC_ERROR_INVALID_OPERATION" + descr ); break;
		case RTC_ERROR_OUT_OF_MEMORY: throw std::runtime_error( "RTC_ERROR_OUT_OF_MEMORY" + descr ); break;
		case RTC_ERROR_UNSUPPORTED_CPU: throw std::runtime_error( "RTC_ERROR_UNSUPPORTED_CPU" + descr ); break;
		case RTC_ERROR_CANCELLED: throw std::runtime_error( "RTC_ERROR_CANCELLED" + descr ); break;
		default: throw std::runtime_error( "invalid error code" + descr ); break;

		}
	}
}

/* raytracer mainloop */
int tutorial_3( const std::string file_name, const char * config )
{

	//dining room
	//Raytracer raytracer(640, 480, 65, glm::vec3(-2.416690, 1.429669, 1.635284), glm::vec3(0, 0, 0.5));
	//Raytracer raytracer(640, 480, 80, glm::vec3(-2.416690, 1.429669, 1.635284), glm::vec3(0, 0, 0));

	//living room
	Raytracer raytracer(1280, 720, 55, glm::vec3(1.877986, -7.724095, 1.602229), glm::vec3(0, 0, 0));
	//Raytracer raytracer(1600, 900, 55, glm::vec3(1.877986, -7.724095, 1.602229), glm::vec3(0, 0, 0));

	raytracer.LoadScene( file_name );
	raytracer.MainLoop();

	return EXIT_SUCCESS;
}