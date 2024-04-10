#include "stdafx.h"
#include "LocalShader.h"

#ifdef GUI_ENABLE_SKIA

namespace gui {

	LocalShader::LocalShader(sk_sp<SkShader> proxy, const SkMatrix &matrix)
		: proxy(proxy), matrix(matrix) {}

	SkShaderBase::GradientType LocalShader::asGradient(GradientInfo *info, SkMatrix *localMatrix) const {
		GradientType type = as_SB(proxy)->asGradient(info, localMatrix);
		if (type != SkShaderBase::GradientType::kNone && localMatrix) {
			*localMatrix = ConcatLocalMatrices(matrix, *localMatrix);
		}
		return type;
	}

	sk_sp<SkFlattenable> LocalShader::CreateProc(SkReadBuffer &buffer) {
		SkMatrix local;
		buffer.readMatrix(&local);
		auto base{buffer.readShader()};
		if (!base)
			return null;
		return sk_make_sp<LocalShader>(base, local);
	}

	void LocalShader::flatten(SkWriteBuffer &to) const {
		to.writeMatrix(matrix);
		to.writeFlattenable(proxy.get());
	}

#ifdef SK_ENABLE_LEGACY_SHADERCONTEXT
	SkShaderBase::Context* LocalShader::onMakeContext(const ContextRec& rec, SkArenaAlloc* alloc) const {
		return as_SB(proxy)->makeContext(ContextRec::Concat(rec, matrix), alloc);
	}
#endif

	SkImage *LocalShader::onIsAImage(SkMatrix *matrix, SkTileMode *mode) const {
		SkMatrix imageMatrix;
		SkImage *image = proxy->isAImage(&imageMatrix, mode);
		if (image && matrix)
			*matrix = ConcatLocalMatrices(this->matrix, imageMatrix);
		return image;
	}

	bool LocalShader::appendStages(const SkStageRec &rec, const SkShaders::MatrixRec &mRec) const {
		return as_SB(proxy)->appendStages(rec, mRec.concat(matrix));
	}

}

#endif
