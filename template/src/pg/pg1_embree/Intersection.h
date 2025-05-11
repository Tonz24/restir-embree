#pragma once
#include "IntersectionResult.h"
#include "Scene.h"

class Intersection {
public:

	static IntersectionResult intersectEmbree(const Scene& scene, const Ray& ray) {
		HitInfo hit{};
		Material* material{};
		RTCRayHit rayHit = getClosestIntersection(scene, ray);

		if (rayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {

			const RTCGeometry geometry = rtcGetGeometry(scene.getRTCScene(), rayHit.hit.geomID);

			material = static_cast<Material*>(rtcGetGeometryUserData(geometry));
			hit = getGeometryAttributes(rayHit, geometry);

			hit.didHit = true;
			hit.hitPoint = ray.getOrg() + ray.getDir() * rayHit.ray.tfar;
			hit.dst = rayHit.ray.tfar;
		}

		//if a normal map exists, use its normal instead of the interpolated normal
		if (material && material->get_texture(Material::kNormalMapSlot)) {

			const auto* normalMap = material->get_texture(Material::kNormalMapSlot);

			glm::vec3 T = hit.tangent - glm::dot(hit.tangent, hit.normal) * hit.normal;
			T = normalize(T);
			glm::vec3 B = glm::normalize(glm::cross(hit.normal, T));

			glm::mat3x3 TBN{ T, B,hit.normal };

			glm::vec3 N = normalMap->get_texel(hit.uv) * 2.0f - 1.0f;

			hit.normal = TBN * N;
		}
		return { hit,material };
	}

	static bool testOcclusion(const glm::vec3& from, const glm::vec3& to, const Scene& scene, const RenderParams& renderParams) {
		const float dist = glm::length(to - from);
		const glm::vec3 dir = glm::normalize(to - from);

		RTCIntersectContext context;
		rtcInitIntersectContext(&context);

		RTCRay ray{
			from.x, from.y, from.z,
			FLT_MIN + renderParams.tnearOffset,
			dir.x, dir.y, dir.z,
			0,
			dist - renderParams.tfarOffset,
			0, 0, 0
		};
		rtcOccluded1(scene.getRTCScene(), &context, &ray);
		return ray.tfar < 0.0f;
	}

private:
	static RTCRayHit getClosestIntersection(const Scene& scene, const Ray& ray) {
		RTCIntersectContext context;
		// setup a hit
		RTCHit hit;
		hit.geomID = RTC_INVALID_GEOMETRY_ID;
		hit.primID = RTC_INVALID_GEOMETRY_ID;
		hit.Ng_x = 0.0f; // geometry normal
		hit.Ng_y = 0.0f;
		hit.Ng_z = 0.0f;

		// merge ray and hit structures
		RTCRayHit ray_hit;
		ray_hit.ray = ray.getRTCRay();
		ray_hit.hit = hit;

		// intersect ray with the scene
		rtcInitIntersectContext(&context);
		rtcIntersect1(scene.getRTCScene(), &context, &ray_hit);

		return ray_hit;
	}

	static HitInfo getGeometryAttributes(const RTCRayHit& rayHit, const RTCGeometry& geometry) {

		HitInfo hitInfo{};

		rtcInterpolate0(geometry, rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v,
			RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, &hitInfo.normal.x, 3);

		glm::vec3 rayDir{ rayHit.ray.dir_x, rayHit.ray.dir_y, rayHit.ray.dir_z };

		hitInfo.normal = glm::normalize(hitInfo.normal);
		if (glm::dot(-rayDir, hitInfo.normal) <= 0.0f) {
			hitInfo.normal *= -1.0f;
			hitInfo.fromInside = true;
		}

		rtcInterpolate0(geometry, rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v,
			RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, &hitInfo.uv.x, 2);

		float idFloat{ -1 };
		rtcInterpolate0(geometry, rayHit.hit.primID, 0.0f, 0.0f,
			RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 2, &idFloat, 1);

		rtcInterpolate0(geometry, rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v,
			RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 3, &hitInfo.tangent.x, 3);

		hitInfo.hitTriId = static_cast<uint32_t>(idFloat);

		return hitInfo;
	}
};