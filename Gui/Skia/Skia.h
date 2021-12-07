#pragma once

#ifdef GUI_ENABLE_SKIA

/**
 * Skia includes. Ignored by Mymake.
 */

// Xlib defines Status and None... They are used as identifiers in Skia...
#undef Status
#undef None

#include "include/gpu/GrDirectContext.h"
#include "include/gpu/gl/GrGLInterface.h"
#include "include/gpu/gl/GrGLAssembleInterface.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRect.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/effects/SkGradientShader.h"
#include "include/core/SkTextBlob.h"
#include "include/core/SkRSXform.h"

// This is technically not a part of the public API. We statically link to Skia, so we will make it work.
// These are only used in LocalShader.h
#include "src/shaders/SkShaderBase.h"
#include "src/gpu/GrFragmentProcessor.h"
#include "src/core/SkVM.h"
#include "src/core/SkReadBuffer.h"
#include "src/core/SkWriteBuffer.h"
// #include "src/gpu/effects/GrMatrixEffect.h"
// #include "src/core/SkMatrixProvider.h"
// #include "src/core/SkTLazy.h"
// #include "src/shaders/SkShaderBase.h"

// For font rendering.
#include "src/ports/SkFontHost_FreeType_common.h"
#include "src/core/SkFontDescriptor.h"

GrGLFuncPtr egl_get(void* ctx, const char name[]);
GrGLFuncPtr glx_get(void* ctx, const char name[]);

namespace gui {

	/**
	 * Convert various of our data types to Skia data types.
	 */

	// Color.
	inline SkColor4f skia(Color c) {
		return {c.r, c.g, c.b, c.a};
	}

	// Point.
	inline SkPoint skia(Point p) {
		return {p.x, p.y};
	}

	// Rectangle.
	inline SkRect skia(Rect r) {
		return {r.p0.x, r.p0.y, r.p1.x, r.p1.y};
	}

	// Matrix.
	inline SkMatrix skia(Transform *tfm) {
		return SkMatrix::MakeAll(tfm->at(0, 0), tfm->at(1, 0), tfm->at(3, 0),
								tfm->at(0, 1), tfm->at(1, 1), tfm->at(3, 1),
								tfm->at(0, 3), tfm->at(1, 3), tfm->at(3, 3));
	}

}

#endif

