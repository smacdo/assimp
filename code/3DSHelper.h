/** @file Defines the helper data structures for importing XFiles */
#ifndef AI_3DSFILEHELPER_H_INC
#define AI_3DSFILEHELPER_H_INC

#include <string>
#include <vector>
#include <sstream>

#include "../include/aiTypes.h"
#include "../include/aiQuaternion.h"
#include "../include/aiMesh.h"
#include "../include/aiAnim.h"
#include "SpatialSort.h"

namespace Assimp
{
	namespace Dot3DS
	{

#if defined(_MSC_VER) ||  defined(__BORLANDC__) ||	defined (__BCPLUSPLUS__)
#	pragma pack(push,2)
#	define PACK_STRUCT
#elif defined( __GNUC__ )
#	define PACK_STRUCT	__attribute__((packed))
#else
#	error Compiler not supported
#endif

// ---------------------------------------------------------------------------
/** Dot3DSFile class: Helper class for loading 3ds files. Defines chunks
*  and data structures.
*/
class Dot3DSFile
{
public:
	inline Dot3DSFile() {}

	// data structure for a single chunk in a .3ds file
	struct Chunk
	{
		unsigned short	Flag;
		long			Size;
	} PACK_STRUCT;

	// source for this used own structures,
	// replaced it with out standard math helpers
	typedef aiMatrix3x3 MatTransform;
	typedef aiVector3D MatTranslate;

	// Used for shading field in material3ds structure
	// From AutoDesk 3ds SDK
	typedef enum
	{
		Wire = 0,
		Flat = 1,
		Gouraud = 2,
		Phong = 3,
		Metal = 4
	} shadetype3ds;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// enum for all chunks in 3ds files. Unused
	// ones are commented, list is not complete since
	// there are many undocumented chunks.
	//
	// Links: http://www.jalix.org/ressources/graphics/3DS/_unofficials/3ds-unofficial.txt
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	enum 
	{

		// **************************************************************
		// Base chunks which can be found everywhere in the file
		CHUNK_VERSION	= 0x0002,
		CHUNK_RGBF      = 0x0010,		// float4 R; float4 G; float4 B
		CHUNK_RGBB      = 0x0011,		// int1 R; int1 G; int B

		// Linear color values (gamma = 2.2?)
		CHUNK_LINRGBF      = 0x0013,	// float4 R; float4 G; float4 B
		CHUNK_LINRGBB      = 0x0012,	// int1 R; int1 G; int B

		CHUNK_PERCENTW	= 0x0030,		// int2   percentage
		CHUNK_PERCENTF	= 0x0031,		// float4  percentage
		// **************************************************************

		// Unknown and ignored
		CHUNK_PRJ       = 0xC23D,

		// Unknown. Possibly a reference to an external .mli file?
		CHUNK_MLI       = 0x3DAA,

		// Primary main chunk of the .3ds file
		CHUNK_MAIN      = 0x4D4D,

		// Mesh main chunk
		CHUNK_OBJMESH   = 0x3D3D,

		// Specifies the background color of the .3ds file
		// This is passed through the material system for
		// viewing purposes.
		CHUNK_BKGCOLOR  = 0x1200,

		// Specifies the ambient base color of the scene.
		// This is added to all materials in the file
		CHUNK_AMBCOLOR  = 0x2100,

		// Specifies the background image for the whole scene
		// This value is passed through the material system
		// to the viewer 
		CHUNK_BIT_MAP   = 0x1100,
		CHUNK_BIT_MAP_EXISTS  = 0x1101,

		// **************************************************************
		// Viewport related stuff. Ignored
		CHUNK_DEFAULT_VIEW = 0x3000,
		CHUNK_VIEW_TOP = 0x3010,
		CHUNK_VIEW_BOTTOM = 0x3020,
		CHUNK_VIEW_LEFT = 0x3030,
		CHUNK_VIEW_RIGHT = 0x3040,
		CHUNK_VIEW_FRONT = 0x3050,
		CHUNK_VIEW_BACK = 0x3060,
		CHUNK_VIEW_USER = 0x3070,
		CHUNK_VIEW_CAMERA = 0x3080,
		// **************************************************************

		// Mesh chunks
		CHUNK_OBJBLOCK  = 0x4000,
		CHUNK_TRIMESH   = 0x4100,
		CHUNK_VERTLIST  = 0x4110,
		CHUNK_VERTFLAGS = 0x4111,
		CHUNK_FACELIST  = 0x4120,
		CHUNK_FACEMAT   = 0x4130,
		CHUNK_MAPLIST   = 0x4140,
		CHUNK_SMOOLIST  = 0x4150,
		CHUNK_TRMATRIX  = 0x4160,
		CHUNK_MESHCOLOR = 0x4165,
		CHUNK_TXTINFO   = 0x4170,
		CHUNK_LIGHT     = 0x4600,
		CHUNK_SPOTLIGHT = 0x4610,
		CHUNK_CAMERA    = 0x4700,
		CHUNK_HIERARCHY = 0x4F00,

		// Specifies the global scaling factor. This is applied
		// to the root node's transformation matrix
		CHUNK_MASTER_SCALE    = 0x0100,

		// **************************************************************
		// Material chunks
		CHUNK_MAT_MATERIAL  = 0xAFFF,

			// asciiz containing the name of the material
			CHUNK_MAT_MATNAME   = 0xA000, 
			CHUNK_MAT_AMBIENT   = 0xA010, // followed by color chunk
			CHUNK_MAT_DIFFUSE   = 0xA020, // followed by color chunk
			CHUNK_MAT_SPECULAR  = 0xA030, // followed by color chunk

			// Specifies the shininess of the material
			// followed by percentage chunk
			CHUNK_MAT_SHININESS  = 0xA040, 

			// Specifies the shading mode to be used
			// followed by a short
			CHUNK_MAT_SHADING  = 0xA100, 

			// NOTE: Emissive color (self illumination) seems not
			// to be a color but a single value, type is unknown.
			// Make the parser accept both of them.
			// followed by percentage chunk (?)
			CHUNK_MAT_SELF_ILLUM = 0xA080,  

			// Always followed by percentage chunk	(?)
			CHUNK_MAT_SELF_ILPCT = 0xA084,  

			// Always followed by percentage chunk
			CHUNK_MAT_TRANSPARENCY = 0xA050, 

			// Diffuse texture channel 0 
			CHUNK_MAT_TEXTURE   = 0xA200,

			// Contains opacity information for each texel
			CHUNK_MAT_OPACMAP = 0xA210,

			// Contains a reflection map to be used to reflect
			// the environment. This is partially supported.
			CHUNK_MAT_REFLMAP = 0xA220,

			// Self Illumination map (emissive colors)
			CHUNK_MAT_SELFIMAP = 0xA33d,	

			// Bumpmap. Not specified whether it is a heightmap
			// or a normal map. Assme it is a heightmap since
			// artist normally prefer this format.
			CHUNK_MAT_BUMPMAP = 0xA230,

			// Specular map. Seems to influence the specular color
			CHUNK_MAT_SPECMAP = 0xA204,

			// Holds shininess data. I assume the specular exponent is
			// calculated like this:
			//
			// s[x,y] =  stex[x,y] * base_shininess;
			// I also assume the data in the texture must be renormalized
			// (normally by dividing / 255) after loading.
			CHUNK_MAT_MAT_SHINMAP = 0xA33C,

			// Scaling in U/V direction.
			// (need to gen separate UV coordinate set 
			// and do this by hand)
			CHUNK_MAT_MAP_USCALE 	  = 0xA354,
			CHUNK_MAT_MAP_VSCALE 	  = 0xA356,

			// Translation in U/V direction.
			// (need to gen separate UV coordinate set 
			// and do this by hand)
			CHUNK_MAT_MAP_UOFFSET 	  = 0xA358,
			CHUNK_MAT_MAP_VOFFSET 	  = 0xA35a,

			// UV-coordinates rotation around the z-axis
			// Assumed to be in radians.
			CHUNK_MAT_MAP_ANG = 0xA35C,

			// Specifies the file name of a texture
			CHUNK_MAPFILE   = 0xA300,
		// **************************************************************

		// Main keyframer chunk. Contains translation/rotation/scaling data
		CHUNK_KEYFRAMER		= 0xB000,

		// Supported sub chunks
		CHUNK_TRACKINFO		= 0xB002,
		CHUNK_TRACKOBJNAME  = 0xB010,
		CHUNK_TRACKPIVOT    = 0xB013,
		CHUNK_TRACKPOS      = 0xB020,
		CHUNK_TRACKROTATE   = 0xB021,
		CHUNK_TRACKSCALE    = 0xB022,

		// **************************************************************
		// Keyframes for various other stuff in the file
		// Ignored
		CHUNK_AMBIENTKEY    = 0xB001,
		CHUNK_TRACKMORPH    = 0xB026,
		CHUNK_TRACKHIDE     = 0xB029,
		CHUNK_OBJNUMBER     = 0xB030,
		CHUNK_TRACKCAMERA	= 0xB003,
		CHUNK_TRACKFOV		= 0xB023,
		CHUNK_TRACKROLL		= 0xB024,
		CHUNK_TRACKCAMTGT	= 0xB004,
		CHUNK_TRACKLIGHT	= 0xB005,
		CHUNK_TRACKLIGTGT	= 0xB006,
		CHUNK_TRACKSPOTL	= 0xB007,
		CHUNK_FRAMES		= 0xB008
		// **************************************************************
	};
};

#if defined(_MSC_VER) ||  defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#	pragma pack( pop )
#endif
#undef PACK_STRUCT

// ---------------------------------------------------------------------------
/** Helper structure representing a 3ds mesh face */
struct Face
{
	Face() : iSmoothGroup(0), bDirection(true)
	{
		// let the rest uninitialized for performance
	}

	
	// Indices. .3ds is using uint16. However, after
	// an unique vrtex set has been geneerated it might
	// be an index becomes > 2^16
	uint32_t i1;
	uint32_t i2;
	uint32_t i3;

	// specifies to which smoothing group the face belongs to
	uint32_t iSmoothGroup;

	// Direction the normal vector of the face 
	// will be pointing to
	bool bDirection;
};
// ---------------------------------------------------------------------------
/** Helper structure representing a texture */
struct Texture
{
	Texture()
		: 
		mScaleU(1.0f),
		mScaleV(1.0f),
		mOffsetU(0.0f),
		mOffsetV(0.0f),
		mRotation(0.0f),
		iUVSrc(0)
	{
		mTextureBlend = std::numeric_limits<float>::quiet_NaN();
	}
	// Specifies the blending factor for the texture
	float mTextureBlend;

	// Specifies the filename of the texture
	std::string mMapName;

	// Specifies texture coordinate offsets/scaling/rotations
	float mScaleU;
	float mScaleV;
	float mOffsetU;
	float mOffsetV;
	float mRotation;

	// Used internally
	bool bPrivate;
	int iUVSrc;
};
// ---------------------------------------------------------------------------
/** Helper structure representing a 3ds material */
struct Material
{
	Material()
		: 
	mSpecularExponent	(0.0f),
	mShading(Dot3DSFile::Gouraud),
	mTransparency		(1.0f),
	mBumpHeight			(1.0f),
	iBakeUVTransform	(0),
	pcSingleTexture		(NULL)
	{
		static int iCnt = 0;
		std::stringstream ss(mName);
		ss << "%%_UNNAMED_" << iCnt++ << "_%%"; 
	}

	// Name of the material
	std::string mName;
	// Diffuse color of the material
	aiColor3D mDiffuse;
	// Specular exponent
	float mSpecularExponent;
	// Specular color of the material
	aiColor3D mSpecular;
	// Ambient color of the material
	aiColor3D mAmbient;
	// Shading type to be used
	Dot3DSFile::shadetype3ds mShading;
	// Opacity of the material
	float mTransparency;

	// Different texture channels
	Texture sTexDiffuse;
	Texture sTexOpacity;
	Texture sTexSpecular;
	Texture sTexBump;
	Texture sTexEmissive;
	Texture sTexShininess;
	
	/*
	float mReflectionTextureBlend;
	std::string mReflectionTexture;
	*/
	float mBumpHeight;

	// Emissive color
	aiColor3D mEmissive;

	// Used internally
	unsigned int iBakeUVTransform;
	Texture* pcSingleTexture;
};
// ---------------------------------------------------------------------------
/** Helper structure to represent a 3ds file mesh */
struct Mesh
{
	Mesh()
	{
		static int iCnt = 0;
		std::stringstream ss(mName);
		ss << "%%_UNNAMED_" << iCnt++ << "_%%"; 
#if 0
		for (unsigned int i = 0; i < 32;++i)
			bSmoothGroupRequired[i] = false;
#endif
	}
	std::string mName;

	std::vector<aiVector3D> mPositions;
	std::vector<Face> mFaces;
	std::vector<aiVector2D> mTexCoords;
	std::vector<unsigned int> mFaceMaterials;
	std::vector<aiVector3D> mNormals;

#if 0
	bool bSmoothGroupRequired[32];
#endif

	aiMatrix4x4 mMat;
};
// ---------------------------------------------------------------------------
/** Helper structure to represent a 3ds file node */
struct Node
{
	Node()
#if 0
		: vScaling(1.0f,1.0f,1.0f)
#endif
	{
		static int iCnt = 0;
		std::stringstream ss(mName);
		ss << "%%_UNNAMED_" << iCnt++ << "_%%"; 

		mHierarchyPos = 0;
		mHierarchyIndex = 0;
	}

	Node* mParent;
	std::vector<Node*> mChildren;
	std::string mName;
	int16_t mHierarchyPos;
	int16_t mHierarchyIndex;

#if 0
	aiVector3D vPivot;
	aiVector3D vScaling;
	aiMatrix4x4 mRotation;
	aiVector3D vPosition;
#endif

	inline Node& push_back(Node* pc)
	{
		mChildren.push_back(pc);
		pc->mParent = this;
		pc->mHierarchyPos = this->mHierarchyPos+1;
		return *this;
	}
};
// ---------------------------------------------------------------------------
/** Helper structure analogue to aiScene */
struct Scene
{

	// NOTE: 3ds references materials globally
	std::vector<Material> mMaterials;
	std::vector<Mesh> mMeshes;

	Node* pcRootNode;
};

// ---------------------------------------------------------------------------
inline bool is_qnan(float p_fIn)
{
	// NOTE: Comparison against qnan is generally problematic
	// because qnan == qnan is false AFAIK
	union FTOINT
	{
		float fFloat;
		int32_t iInt;
	} one, two;
	one.fFloat = std::numeric_limits<float>::quiet_NaN();
	two.fFloat = p_fIn;

	return (one.iInt == two.iInt);
}
// ---------------------------------------------------------------------------
inline bool is_not_qnan(float p_fIn)
{
	return !is_qnan(p_fIn);
}

} // end of namespace Dot3DS
} // end of namespace Assimp

#endif // AI_XFILEHELPER_H_INC