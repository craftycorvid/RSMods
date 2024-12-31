#pragma once

namespace GuitarSkeletons {
	xyzPosition StringAttachPosition(bool leftHanded, bool bass, int stringNumber, bool setToDefaults, double inputZ);
	xyzPosition FretBarPosition(bool leftHanded, bool topOfFretBar, bool setToDefaults, int fretNumber, double inputZ);
	xyzPosition CameraMarkerPosition(bool leftHanded, bool tipOfCameraMarker, bool topOfCameraMarker, bool setToDefaults, double inputZ);
	xyzPosition GetSkeletonPosition();

	inline double X, Y, Z;
};