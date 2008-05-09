/** @file 4x4 matrix structure, including operators when compiling in C++ */
#ifndef AI_MATRIX4X4_H_INC
#define AI_MATRIX4X4_H_INC

#ifdef __cplusplus
extern "C" {
#endif

struct aiMatrix3x3;

// Set packing to 4
#if defined(_MSC_VER) ||  defined(__BORLANDC__) ||	defined (__BCPLUSPLUS__)
	#pragma pack(push,4)
	#define PACK_STRUCT
#elif defined( __GNUC__ )
	#define PACK_STRUCT	__attribute__((packed))
#else
	#error Compiler not supported
#endif

// ---------------------------------------------------------------------------
/** Represents a column-major 4x4 matrix, 
*  use this for homogenious coordinates 
*/
// ---------------------------------------------------------------------------
typedef struct aiMatrix4x4
{
#ifdef __cplusplus
	aiMatrix4x4 () :	
		a1(1.0f), a2(0.0f), a3(0.0f), a4(0.0f), 
		b1(0.0f), b2(1.0f), b3(0.0f), b4(0.0f), 
		c1(0.0f), c2(0.0f), c3(1.0f), c4(0.0f),
		d1(0.0f), d2(0.0f), d3(0.0f), d4(1.0f){}

	aiMatrix4x4 (	float _a1, float _a2, float _a3, float _a4,
					float _b1, float _b2, float _b3, float _b4,
					float _c1, float _c2, float _c3, float _c4,
					float _d1, float _d2, float _d3, float _d4) :	
		a1(_a1), a2(_a2), a3(_a3), a4(_a4),  
		b1(_b1), b2(_b2), b3(_b3), b4(_b4), 
		c1(_c1), c2(_c2), c3(_c3), c4(_c4),
		d1(_d1), d2(_d2), d3(_d3), d4(_d4)
		{}

	/** Constructor from 3x3 matrix. The remaining elements are set to identity. */
	explicit aiMatrix4x4( const aiMatrix3x3& m);

	aiMatrix4x4& operator *= (const aiMatrix4x4& m);
	aiMatrix4x4 operator* (const aiMatrix4x4& m) const;
	aiMatrix4x4& Transpose();
	aiMatrix4x4& Inverse();
	float Determinant() const;

	float* operator[](unsigned int p_iIndex);
	const float* operator[](unsigned int p_iIndex) const;
#endif // __cplusplus

	float a1, a2, a3, a4;
	float b1, b2, b3, b4;
	float c1, c2, c3, c4;
	float d1, d2, d3, d4;

} PACK_STRUCT aiMatrix4x4_t;


// Reset packing
#if defined(_MSC_VER) ||  defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#pragma pack( pop )
#endif
#undef PACK_STRUCT

#ifdef __cplusplus
} // end extern "C"


#endif // __cplusplus
#endif // AI_MATRIX4X4_H_INC