#pragma once

#include "SceneObject.hpp"

class Transform {
	// Trims angle to 360 degrees
	static float trimAngle(float a) {
		int b = a;
		return b % 360 + (a - b);
	}

	// Trims angle to [-180;180] and converts to radian
	static float convertToDeltaDegree(float a) {
		return trimAngle(a) * 3.1415926f * 2 / 360;
	}

	// Get rotation angle around the axe.
	// Calculates a simple sum of absolute values of angles from all axes,
	// converts angle from radians to degrees
	// and then trims it to 180 degrees.
	static float getDirectedDeltaAngle(const glm::vec3& rotation) {
		float angle = 0;
		// Rotation axe determines the rotating direction 
		// so the rotation angles should be positive
		for (auto i = 0; i < 3; i++)
			angle += abs(rotation[i]);
		return trimAngle(angle) * 3.1415926f * 2 / 360;
	}

	// Get rotation angle around the axe.
	// Calculates a simple sum of absolute values of angles from all axes,
	// trims angle to 180 degrees and then converts to radians
	static bool tryGetLocalRotation(const glm::vec3& rotation, glm::vec3& axe, float& angle) {
		glm::vec3 t[2] = { glm::vec3(), glm::vec3() };
		float angles[2] = { 0, 0 };

		// Supports rotation around 1,2 axes simultaneously.
		auto modifiedAxeNumber = 0;
		for (auto i = 0; i < 3; i++) {
			assert(modifiedAxeNumber < 3);
			if (rotation[i] != 0) {
				t[modifiedAxeNumber][i] = rotation[i] > 0 ? 1 : -1;
				angles[modifiedAxeNumber] = abs(rotation[i]);
				modifiedAxeNumber++;
			}
		}

		if (modifiedAxeNumber == 0) {
			axe = glm::vec3();
			return false;
		}
		if (modifiedAxeNumber == 1) {
			angle = convertToDeltaDegree(angles[0]);
			axe = t[0];
			return true;
		}

		angle = convertToDeltaDegree(angles[0] + angles[1]);
		axe = t[0] + t[1];
		return true;
	}

	// Convert local rotation to global rotation relative to cross rotation.
	// 
	// Shortly, Isolates rotation in cross' local space.
	// 	   
	// Based on quaternion intransitivity, applying new local rotation and then applying inverted initial global rotation 
	// results in new local rotation being converted to global rotation.
	// r0 * r1Local * r0^-1 = r1Global
	// 
	// Basically, moves rotation initial point from (0;0;0;1) to cross global rotation.
	// Compromises performance since cross new rotation is calculated both here and ourside of this method
	// but improves readability and maintainability of the code.
	static glm::quat getIsolatedRotation(const glm::quat& localRotation, const glm::quat crossGlobalRotation) {
		auto crossNewRotation = crossGlobalRotation * localRotation;
		auto rIsolated = crossNewRotation * glm::inverse(crossGlobalRotation);
		return rIsolated;
	}

public:
	static void Scale(const glm::vec3& center, const float& oldScale, const float& scale, std::list<PON>& targets) {
		for (auto& target : targets) {
			target->SetWorldPosition((target->GetWorldPosition() - center) / oldScale * scale + center);
			for (size_t i = 0; i < target->GetVertices().size(); i++)
				target->SetVertice(i, (target->GetVertices()[i] - center) / oldScale * scale + center);
		}
	}
	static void Translate(const glm::vec3& transformVector, std::list<PON>& targets, SceneObject* cross) {
		// Need to calculate average rotation.
		// https://stackoverflow.com/questions/12374087/average-of-multiple-quaternions/27410865#27410865
		if (Settings::SpaceMode().Get() == SpaceMode::Local) {
			auto r = glm::rotate(cross->GetWorldRotation(), transformVector);

			cross->SetWorldPosition(cross->GetWorldPosition() + r);
			for (auto o : targets) {
				o->SetWorldPosition(o->GetWorldPosition() + r);
				for (size_t i = 0; i < o->GetVertices().size(); i++)
					o->SetVertice(i, o->GetVertices()[i] + r);
			}
			return;
		}

		cross->SetWorldPosition(cross->GetWorldPosition() + transformVector);
		for (auto o : targets) {
			o->SetWorldPosition(o->GetWorldPosition() + transformVector);
			for (size_t i = 0; i < o->GetVertices().size(); i++)
				o->SetVertice(i, o->GetVertices()[i] + transformVector);
		}
	}
	static void Rotate(const glm::vec3& center, const glm::vec3& rotation, std::list<PON>& targets, SceneObject* cross) {
		glm::vec3 axe;
		float angle;

		if (!tryGetLocalRotation(rotation, axe, angle))
			return;

		auto r = glm::normalize(glm::angleAxis(angle, axe));

		// Convert local rotation to global rotation.
		// It is required to rotate all vertices since they don't store their own rotation
		// meaning that their rotation = (0;0;0;1).
		//
		// When multiple objects are selected cross' rotation is equal to the first 
		// object in selection. See TransformTool::OnSelectionChanged
		if (Settings::SpaceMode().Get() == SpaceMode::Local)
			r = getIsolatedRotation(r, cross->GetWorldRotation());

		cross->SetLocalRotation(r * cross->GetLocalRotation());

		for (auto& target : targets) {
			target->SetWorldPosition(glm::rotate(r, target->GetWorldPosition() - center) + center);
			for (size_t i = 0; i < target->GetVertices().size(); i++)
				target->SetVertice(i, glm::rotate(r, target->GetVertices()[i] - center) + center);

			target->SetWorldRotation(r * target->GetWorldRotation());
		}
	}
};

struct Convert {
	struct Unit {
		// Millimeters to pixels
		static glm::vec3 MillimetersToPixels(const glm::vec3& vMillimeters) {
			static float inchToMillimeter = 0.0393701;
			// vMillimiters[millimiter]
			// inchToMillemeter[inch/millimeter]
			// PPI[pixel/inch]
			// vMillimiters*PPI*inchToMillemeter[millimeter*(pixel/inch)*(inch/millimeter) = millimeter*(pixel/millimeter) = pixel]
			auto vPixels = Settings::PPI().Get() * inchToMillimeter * vMillimeters;
			return vPixels;
		}

		// Millimeters to [-1;1]
		// World center-centered
		// (0;0;0) in view coordinates corresponds to (0;0;0) in world coordinates
		static glm::vec3 MillimetersToViewCoordinates(const glm::vec3& vMillimeters, const glm::vec2& viewSizePixels, const float& viewSizeZMillimeters) {
			static float inchToMillimeter = 0.0393701;
			auto vsph = viewSizePixels / 2.f;
			auto vszmh = viewSizeZMillimeters / 2.f;

			auto vView = glm::vec3(
				vMillimeters.x * Settings::PPI().Get() * inchToMillimeter / vsph.x,
				vMillimeters.y * Settings::PPI().Get() * inchToMillimeter / vsph.y,
				vMillimeters.z / vszmh
			);
			return vView;
		}

		// Millimeters to [-1;1]
		// World center-centered
		// (0;0;0) in view coordinates corresponds to (0;0;0) in world coordinates
		static float MillimetersToViewCoordinates(const float& vMillimeters, const float& viewSizePixels) {
			static float inchToMillimeter = 0.0393701;
			auto vsph = viewSizePixels / 2.f;

			auto vView = vMillimeters * Settings::PPI().Get() * inchToMillimeter / vsph;
			return vView;
		}

		// Pixels to Millimeters
		static glm::vec3 PixelsToMillimeters(const glm::vec3& vPixels) {
			static float inchToMillimeter = 0.0393701;
			// vPixels[pixel]
			// inchToMillimeter[inch/millimeter]
			// PPI[pixel/inch]
			// vPixels/(PPI*inchToMillimeter)[pixel/((pixel/inch)*(inch/millimeter)) = pixel/(pixel/millimeter) = (pixel/pixel)*(millimeter) = millimiter]
			auto vMillimiters = vPixels / Settings::PPI().Get() / inchToMillimeter;
			return vMillimiters;
		}
		static glm::vec2 PixelsToMillimeters(const glm::vec2& vPixels) {
			static float inchToMillimeter = 0.0393701;
			// vPixels[pixel]
			// inchToMillimeter[inch/millimeter]
			// PPI[pixel/inch]
			// vPixels/(PPI*inchToMillimeter)[pixel/((pixel/inch)*(inch/millimeter)) = pixel/(pixel/millimeter) = (pixel/pixel)*(millimeter) = millimiter]
			auto vMillimiters = vPixels / Settings::PPI().Get() / inchToMillimeter;
			return vMillimiters;
		}

	};
	class Stereo {
		static glm::vec3 getLeft(const glm::vec3& posMillimeters, const glm::vec3& cameraPos, float eyeToCenterDistance, const glm::vec2& viewSize, float viewSizeZ) {
			auto pos = Convert::Unit::MillimetersToViewCoordinates(posMillimeters, viewSize, viewSizeZ);
			float denominator = cameraPos.z - pos.z;
			return glm::vec3(
				(pos.x * cameraPos.z - pos.z * (cameraPos.x - eyeToCenterDistance)) / denominator,
				(cameraPos.z * -pos.y + cameraPos.y * pos.z) / denominator,
				0
			);
		}
		static glm::vec3 getRight(const glm::vec3& posMillimeters, const glm::vec3& cameraPos, float eyeToCenterDistance, const glm::vec2& viewSize, float viewSizeZ) {
			auto pos = Convert::Unit::MillimetersToViewCoordinates(posMillimeters, viewSize, viewSizeZ);
			float denominator = cameraPos.z - pos.z;
			return glm::vec3(
				(pos.x * cameraPos.z - pos.z * (cameraPos.x + eyeToCenterDistance)) / denominator,
				(cameraPos.z * -pos.y + cameraPos.y * pos.z) / denominator,
				0
			);
		}
	public:
		static glm::vec3 GetLeft(const glm::vec3& v, const glm::vec3& cameraPos, float eyeToCenterDistance, const glm::vec2& viewSize, float viewSizeZ) {
			return getLeft(v, cameraPos, eyeToCenterDistance, viewSize, viewSizeZ);
		}
		static glm::vec3 GetRight(const glm::vec3& v, const glm::vec3& cameraPos, float eyeToCenterDistance, const glm::vec2& viewSize, float viewSizeZ) {
			return getRight(v, cameraPos, eyeToCenterDistance, viewSize, viewSizeZ);
		}

	};
};

class Build {
	static glm::quat getRotation(glm::vec3 ac, glm::vec3 ab) {
		auto zPlaneLocal = glm::cross(ac, ab);
		zPlaneLocal = glm::normalize(zPlaneLocal);

		auto zPlaneGlobal = glm::vec3(0, 0, 1);

		auto r1 = glm::rotation(zPlaneGlobal, zPlaneLocal);

		auto acRotatedBack = glm::rotate(glm::inverse(r1), ac);
		acRotatedBack = glm::normalize(acRotatedBack);

		auto r2 = glm::rotation(glm::vec3(1, 0, 0), acRotatedBack);
		auto r = r1 * r2;
		return r;
	}

public:
	
	/// <summary>
	///    B
	///  / |  \
	/// A--D---C
	/// </summary>
	static const std::vector<glm::vec3> Sine(const glm::vec3 vertices[3]) {
		auto ac = vertices[2] - vertices[0];
		auto ab = vertices[1] - vertices[0];
		auto bc = vertices[2] - vertices[1];

		auto acUnit = glm::normalize(ac);
		auto abadScalarProjection = glm::dot(ab, acUnit);
		auto db = ab - acUnit * abadScalarProjection;
		auto abdbScalarProjection = glm::dot(ab, glm::normalize(db));

		if (isnan(abadScalarProjection) || isnan(abdbScalarProjection))
			return { vertices[1], vertices[2] };

		auto r = getRotation(ac, ab);

		if (isnan(r.x) || isnan(r.y) || isnan(r.z) || isnan(r.w))
			return { vertices[1], vertices[2] };

		auto rotatedY = glm::rotate(r, glm::vec3(0, 1, 0));
		auto sameDirection = glm::dot(glm::normalize(db), rotatedY) > 0;

		auto acLength = glm::length(ac);
		auto abLength = glm::length(ab);
		auto bdLength = abdbScalarProjection;
		auto adLength = abadScalarProjection;
		auto dcLength = acLength - adLength;

		static const auto hpi = 3.1415926 / 2.;

		auto bcLength = glm::length(bc);

		int abNumber = abLength / 5 + 1;
		int bcNumber = bcLength / 5 + 1;

		std::vector<glm::vec3> points;

		for (size_t j = 0; j < abNumber; j++) {
			auto x = -hpi + hpi * j / (float)abNumber;
			auto nx = j / (float)abNumber * adLength;
			auto ny = cos(x) * bdLength;
			points.push_back(glm::vec3(nx, ny, 0));
		}
		for (size_t j = 0; j <= bcNumber; j++) {
			auto x = hpi * j / (float)bcNumber;
			auto nx = j / (float)bcNumber * dcLength + adLength;
			auto ny = cos(x) * bdLength;
			points.push_back(glm::vec3(nx, ny, 0));
		}

		if (!sameDirection)
			for (size_t j = 0; j < points.size(); j++)
				points[j].y = -points[j].y;

		for (size_t j = 0; j < points.size(); j++)
			points[j] = glm::rotate(r, points[j]) + vertices[0];

		return points;
	}
};