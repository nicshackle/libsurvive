#pragma once

#ifdef USE_OPENCV
#include "minimal_opencv.opencv.h"

#else

#include "linmath.h"
#include "string.h"

#define SV_SVD 1
#define SV_SVD_MODIFY_A 1
#define SV_SVD_SYM 2
#define SV_SVD_U_T 2
#define SV_SVD_V_T 4

#define SV_GEMM_A_T 1
#define SV_GEMM_B_T 2
#define SV_GEMM_C_T 4

#define SV_8U 0
#define SV_8S 1
#define SV_16U 2
#define SV_16S 3
#define SV_32S 4
#define SV_32F 5
#define SV_64F 6

#ifdef USE_EIGEN
#include "sv_matrix.eigen.h"
#else
#include "sv_matrix.blas.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SV_IS_MAT_HDR(mat)                                                                                             \
	((mat) != NULL && (((const SvMat *)(mat))->type & SV_MAGIC_MASK) == SV_MAT_MAGIC_VAL &&                            \
	 ((const SvMat *)(mat))->cols > 0 && ((const SvMat *)(mat))->rows > 0)

#define SV_IS_MAT_HDR_Z(mat)                                                                                           \
	((mat) != NULL && (((const SvMat *)(mat))->type & SV_MAGIC_MASK) == SV_MAT_MAGIC_VAL &&                            \
	 ((const SvMat *)(mat))->cols >= 0 && ((const SvMat *)(mat))->rows >= 0)

#define SV_IS_MAT(mat) (SV_IS_MAT_HDR(mat) && ((const SvMat *)(mat))->data.ptr != NULL)

#define SV_IS_MASK_ARR(mat) (((mat)->type & (SV_MAT_TYPE_MASK & ~SV_8SC1)) == 0)

#define SV_ARE_TYPES_EQ(mat1, mat2) ((((mat1)->type ^ (mat2)->type) & SV_MAT_TYPE_MASK) == 0)

#define SV_ARE_CNS_EQ(mat1, mat2) ((((mat1)->type ^ (mat2)->type) & SV_MAT_CN_MASK) == 0)

#define SV_ARE_DEPTHS_EQ(mat1, mat2) ((((mat1)->type ^ (mat2)->type) & SV_MAT_DEPTH_MASK) == 0)

#define SV_ARE_SIZES_EQ(mat1, mat2) ((mat1)->rows == (mat2)->rows && (mat1)->cols == (mat2)->cols)

#define SV_IS_MAT_CONST(mat) (((mat)->rows | (mat)->cols) == 1)

#define SV_IS_MATND_HDR(mat) ((mat) != NULL && (((const SvMat *)(mat))->type & SV_MAGIC_MASK) == SV_MATND_MAGIC_VAL)

#define SV_IS_MATND(mat) (SV_IS_MATND_HDR(mat) && ((const SvMat *)(mat))->data.ptr != NULL)
#define SV_MATND_MAGIC_VAL 0x42430000

void print_mat(const SvMat *M);

SvMat *svInitMatHeader(SvMat *arr, int rows, int cols, int type);
SvMat *svCreateMat(int height, int width);

enum svInvertMethod {
	SV_INVERT_METHOD_UNKNOWN = 0,
	SV_INVERT_METHOD_SVD = 1,
	SV_INVERT_METHOD_LU = 2,
};

double svInvert(const SvMat *srcarr, SvMat *dstarr, enum svInvertMethod method);

void svGEMM(const SvMat *src1, const SvMat *src2, double alpha, const SvMat *src3, double beta, SvMat *dst, int tABC);

/**
 * xarr = argmin_x(Aarr * x - Barr)
 */
int svSolve(const SvMat *Aarr, const SvMat *Barr, SvMat *xarr, enum svInvertMethod method);

void svSetZero(SvMat *arr);

void svCopy(const SvMat *src, SvMat *dest, const SvMat *mask);

SvMat *svCloneMat(const SvMat *mat);

void svReleaseMat(SvMat **mat);

void svSVD(SvMat *aarr, SvMat *warr, SvMat *uarr, SvMat *varr, int flags);

void svMulTransposed(const SvMat *src, SvMat *dst, int order, const SvMat *delta, double scale);

void svTranspose(const SvMat *M, SvMat *dst);

void print_mat(const SvMat *M);

double svDet(const SvMat *M);

#define SV_CREATE_STACK_MAT(name, rows, cols)                                                                          \
	FLT *_##name = alloca(rows * cols * sizeof(FLT));                                                                  \
	SvMat name = svMat(rows, cols, _##name);

static inline void sv_set_diag(struct SvMat *m, const FLT *v) {
	for (int i = 0; i < m->rows; i++) {
		for (int j = 0; j < m->cols; j++) {
			svMatrixSet(m, i, j, i == j ? v[i] : 0.);
		}
	}
}

static inline void sv_set_diag_val(struct SvMat *m, FLT v) {
	for (int i = 0; i < m->rows; i++) {
		for (int j = 0; j < m->cols; j++) {
			svMatrixSet(m, i, j, i == j ? v : 0.);
		}
	}
}

static inline void sv_set_zero(struct SvMat *m) { memset(SV_FLT_PTR(m), 0, sizeof(FLT) * m->rows * m->cols); }

/** Inline constructor. No data is allocated internally!!!
 * (Use together with svCreateData, or use svCreateMat instead to
 * get a matrix with allocated data):
 */
static inline SvMat svMat(int rows, int cols, FLT *data) {
	SvMat m;

	assert((unsigned)SV_MAT_DEPTH(SV_FLT) <= SV_64F);
	int type = SV_MAT_TYPE(SV_FLT);
	m.type = SV_MAT_MAGIC_VAL | SV_MAT_CONT_FLAG | type;
	m.cols = cols;
	m.rows = rows;
	m.step = m.cols * SV_ELEM_SIZE(type);
	if (!data) {
		m.data.ptr = (uint8_t *)calloc(1, SV_ELEM_SIZE(type) * m.cols * m.rows);
	} else {
		m.data.ptr = (uint8_t *)data;
	}
	m.refcount = 0;
	m.hdr_refcount = 0;

#if SURVIVE_ASAN_CHECKS
	volatile double v = cvmGet(&m, rows - 1, cols - 1);
	(void)v;
#endif

	return m;
}

#ifdef __cplusplus
}
#endif

#endif // NOT USE_OPENCV