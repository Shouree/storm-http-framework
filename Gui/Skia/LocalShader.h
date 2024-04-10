#pragma once
#include "Skia.h"

#ifdef GUI_ENABLE_SKIA

namespace gui {

	/**
	 * A version of the SkLocalMatrixShader.
	 *
	 * This is so that we can freely modify the transform without re-creating objects all the
	 * time. While this is possible to some extent in Skia, there is seemingly no good way to simply
	 * set the shader's matrix to some value. We can only append the matrix to the previous one.
	 */
	class LocalShader : public SkShaderBase {
	public:
		// Create a local shader as a proxy for some other object.
		LocalShader(sk_sp<SkShader> proxy, const SkMatrix &matrix);

		// Overrides for some needed functions.
		bool isOpaque() const override { return as_SB(proxy)->isOpaque(); }
		bool isConstant() const override { return as_SB(proxy)->isConstant(); }
		GradientType asGradient(GradientInfo *info, SkMatrix *localMatrix) const override;
		ShaderType type() const override { return ShaderType::kLocalMatrix; }

		sk_sp<SkShader> makeAsALocalMatrixShader(SkMatrix *localMatrix) const override {
			if (localMatrix) {
				*localMatrix = matrix;
			}
			return proxy;
		}

		// Local matrix.
		SkMatrix matrix;

	protected:
		// Flatten.
		void flatten(SkWriteBuffer &to) const override;

#ifdef SK_ENABLE_LEGACY_SHADERCONTEXT
		Context* onMakeContext(const ContextRec&, SkArenaAlloc*) const override;
#endif

		// To an image.
		SkImage *onIsAImage(SkMatrix *matrix, SkTileMode *mode) const override;

		// Append stages.
		bool appendStages(const SkStageRec &, const SkShaders::MatrixRec &) const override;

	private:
		SK_FLATTENABLE_HOOKS(LocalShader);

		// Shader we're wrapping.
		sk_sp<SkShader> proxy;
	};

}

#endif
