/** @file Defines the helper data structures for importing PLY files  */
#ifndef AI_PLYFILEHELPER_H_INC
#define AI_PLYFILEHELPER_H_INC

#include <string>
#include <vector>
#include <list>
#include <sstream>

#include "../include/aiTypes.h"
#include "../include/aiMesh.h"
#include "../include/aiAnim.h"

namespace Assimp
{

// http://local.wasp.uwa.edu.au/~pbourke/dataformats/ply/
// http://w3.impa.br/~lvelho/outgoing/sossai/old/ViHAP_D4.4.2_PLY_format_v1.1.pdf
// http://www.okino.com/conv/exp_ply.htm
namespace PLY
{


// ---------------------------------------------------------------------------------
/*
name        type        number of bytes
---------------------------------------
char       character                 1
uchar      unsigned character        1
short      short integer             2
ushort     unsigned short integer    2
int        integer                   4
uint       unsigned integer          4
float      single-precision float    4
double     double-precision float    8

int8
int16
uint8 ... forms are also used
*/
enum EDataType
{
	EDT_Char = 0x0u,
	EDT_UChar,
	EDT_Short,
	EDT_UShort,
	EDT_Int,
	EDT_UInt,
	EDT_Float,
	EDT_Double,

	// Marks invalid entries
	EDT_INVALID
};

// ---------------------------------------------------------------------------------
/**	\brief Specifies semantics for PLY element properties
 *
 *	Semantics define the usage of a property, e.g. x coordinate
*/
enum ESemantic
{
	//! vertex position x coordinate
	EST_XCoord = 0x0u,
	//! vertex position x coordinate
	EST_YCoord,
	//! vertex position x coordinate
	EST_ZCoord,

	//! vertex normal x coordinate
	EST_XNormal,
	//! vertex normal y coordinate
	EST_YNormal,
	//! vertex normal z coordinate
	EST_ZNormal,
	
	//! vertex colors, red channel
	EST_Red,		
	//! vertex colors, green channel
	EST_Green,
	//! vertex colors, blue channel
	EST_Blue,
	//! vertex colors, alpha channel
	EST_Alpha,

	//! vertex index list 
	EST_VertexIndex,

	//! texture index 
	EST_TextureIndex,

	//! texture coordinates (stored as element of a face)
	EST_TextureCoordinates,

	//! material index 
	EST_MaterialIndex,

	//! ambient color, red channel
	EST_AmbientRed,
	//! ambient color, green channel
	EST_AmbientGreen,
	//! ambient color, blue channel
	EST_AmbientBlue,
	//! ambient color, alpha channel
	EST_AmbientAlpha,

	//! diffuse color, red channel
	EST_DiffuseRed,
	//! diffuse color, green channel
	EST_DiffuseGreen,
	//! diffuse color, blue channel
	EST_DiffuseBlue,
	//! diffuse color, alpha channel
	EST_DiffuseAlpha,

	//! specular color, red channel
	EST_SpecularRed,
	//! specular color, green channel
	EST_SpecularGreen,
	//! specular color, blue channel
	EST_SpecularBlue,
	//! specular color, alpha channel
	EST_SpecularAlpha,

	//! specular power for phong shading
	EST_PhongPower,

	//! opacity between 0 and 1
	EST_Opacity,

	//! Marks invalid entries
	EST_INVALID
};

// ---------------------------------------------------------------------------------
/**	\brief Specifies semantics for PLY elements
 *
 *	Semantics define the usage of an element, e.g. vertex or material
*/
enum EElementSemantic
{
	//! The element is a vertex
	EEST_Vertex	= 0x0u,

	//! The element is a face description (index table)
	EEST_Face,

	//! The element is a tristrip description (index table)
	EEST_TriStrip,

	//! The element is an edge description (ignored)
	EEST_Edge,

	//! The element is a material description 
	EEST_Material,

	//! Marks invalid entries
	EEST_INVALID
};

// ---------------------------------------------------------------------------------
/**	\brief Helper class for a property in a PLY file.
 *
 *	This can e.g. be a part of the vertex declaration
 */
class Property
{
public:

	//! Default constructor
	Property()
		: eType (EDT_Int), bIsList(false), eFirstType(EDT_UChar)
	{}

	//!	Data type of the property
	EDataType eType;

	//!	Semantical meaning of the property
	ESemantic Semantic;

	//! Of the semantic of the property could not be parsed:
	//! Contains the semantic specified in the file
	std::string szName;

	//!	Specifies whether the data type is a list where
	//! the first element specifies the size of the list
	bool bIsList;
	EDataType eFirstType;

	//! Parse a property from a string. The end of the
	//! string is either '\n', '\r' or '\0'. Return valie is false
	//! if the input string is NOT a valid property (E.g. does
	//! not start with the "property" keyword)
	static bool ParseProperty (const char* p_szIn, const char** p_szOut, 
		Property* pOut);

	//! Parse a data type from a string
	static EDataType ParseDataType(const char* p_szIn,const char** p_szOut);

	//! Parse a semantic from a string
	static ESemantic ParseSemantic(const char* p_szIn,const char** p_szOut);
};

// ---------------------------------------------------------------------------------
/**	\brief Helper class for an element in a PLY file.
 *
 *	This can e.g. be the vertex declaration. Elements contain a
 *	well-defined number of properties.
 */
class Element
{
public:

	//! Default constructor
	Element()
		: NumOccur(0), eSemantic (EEST_INVALID)
	{}

	//! List of properties assigned to the element
	//! std::vector to support operator[]
	std::vector<Property> alProperties;

	//! Semantic of the element
	EElementSemantic eSemantic;

	//! Of the semantic of the element could not be parsed:
	//! Contains the semantic specified in the file
	std::string szName;

	//! How many times will the element occur?
	unsigned int NumOccur;

	//! Parse an element from a string. 
	//! The function will parse all properties contained in the
	//! element, too.
	static bool ParseElement (const char* p_szIn, const char** p_szOut, 
		Element* pOut);

	//! Parse a semantic from a string
	static EElementSemantic ParseSemantic(const char* p_szIn,
		const char** p_szOut);
};

// ---------------------------------------------------------------------------------
/**	\brief Instance of a property in a PLY file
 */
class PropertyInstance 
{
public:

	//! Default constructor
	PropertyInstance ()
	{}

	union ValueUnion
	{

		//! uInt32 representation of the property. All
		// uint types are automatically converted to uint32
		uint32_t iUInt;

		//! Int32 representation of the property. All
		// int types are automatically converted to int32
		int32_t iInt;

		//! Float32 representation of the property
		float fFloat;

		//! Float64 representation of the property
		double fDouble;

	};

	//! List of all values parsed. Contains only one value
	// for non-list propertys
	std::list<ValueUnion> avList;

	//! Parse a property instance 
	static bool ParseInstance (const char* p_szIn,const char** p_szOut,
		const Property* prop, PropertyInstance* p_pcOut);

	//! Parse a property instance in binary format
	static bool ParseInstanceBinary (const char* p_szIn,const char** p_szOut,
		const Property* prop, PropertyInstance* p_pcOut,bool p_bBE);

	//! Get the default value for a given data type
	static ValueUnion DefaultValue(EDataType eType);

	//! Parse a value
	static bool ParseValue(const char* p_szIn,const char** p_szOut,
		EDataType eType,ValueUnion* out);

	//! Parse a binary value
	static bool ParseValueBinary(const char* p_szIn,const char** p_szOut,
		EDataType eType,ValueUnion* out,bool p_bBE);

	//! Convert a property value to a given type TYPE
	template <typename TYPE>
	static TYPE ConvertTo(ValueUnion v, EDataType eType);
};

// ---------------------------------------------------------------------------------
/**	\brief Class for an element instance in a PLY file
 */
class ElementInstance 
{
public:

	//! Default constructor
	ElementInstance ()
	{}

	//! List of all parsed properties
	std::vector< PropertyInstance > alProperties;

	//! Parse an element instance
	static bool ParseInstance (const char* p_szIn,const char** p_szOut,
		const Element* pcElement, ElementInstance* p_pcOut);

	//! Parse a binary element instance
	static bool ParseInstanceBinary (const char* p_szIn,const char** p_szOut,
		const Element* pcElement, ElementInstance* p_pcOut,bool p_bBE);
};

// ---------------------------------------------------------------------------------
/**	\brief Class for an element instance list in a PLY file
 */
class ElementInstanceList 
{
public:

	//! Default constructor
	ElementInstanceList ()
	{}

	ElementInstanceList (const Element* pc)
	{
		alInstances.reserve(pc->NumOccur);
	}

	//! List of all element instances
	std::vector< ElementInstance > alInstances;

	//! Parse an element instance list
	static bool ParseInstanceList (const char* p_szIn,const char** p_szOut,
		const Element* pcElement, ElementInstanceList* p_pcOut);

	//! Parse a binary element instance list
	static bool ParseInstanceListBinary (const char* p_szIn,const char** p_szOut,
		const Element* pcElement, ElementInstanceList* p_pcOut,bool p_bBE);
};
// ---------------------------------------------------------------------------------
/**	\brief Class to represent the document object model of an ASCII or binary 
 *		  (both little and big-endian) PLY file
 */
class DOM
{
public:

	//! Default constructor
	DOM()
	{}


	std::vector<Element> alElements;
	std::vector<ElementInstanceList> alElementData;

	//! Parse the DOM for a PLY file. The input string is assumed
	//! to be terminated with zero
	static bool ParseInstance (const char* p_szIn,DOM* p_pcOut);
	static bool ParseInstanceBinary (const char* p_szIn,
		DOM* p_pcOut,bool p_bBE);

	//! Skip all comment lines after this
	static bool SkipComments (const char* p_szIn,const char** p_szOut);

private:

	//! Handle the file header and read all element descriptions
	bool ParseHeader (const char* p_szIn,const char** p_szOut);

	//! Read in all element instance lists
	bool ParseElementInstanceLists (const char* p_szIn,const char** p_szOut);

	//! Read in all element instance lists for a binary file format
	bool ParseElementInstanceListsBinary (const char* p_szIn,
		const char** p_szOut,bool p_bBE);
};

// ---------------------------------------------------------------------------------
/**	\brief Helper class to represent a loaded face
 */
class Face
{
public:

	Face()
		: iMaterialIndex(0xFFFFFFFF)
	{
		// set all indices to zero by default
		mIndices.resize(3,0);
	}

public:

	//! List of vertex indices
	std::vector<unsigned int> mIndices;

	//! Material index
	unsigned int iMaterialIndex;
};

// ---------------------------------------------------------------------------------
template <typename TYPE>
TYPE PLY::PropertyInstance::ConvertTo(
	PLY::PropertyInstance::ValueUnion v, PLY::EDataType eType)
{
	switch (eType)
	{
	case EDT_Float:
		return (TYPE)v.fFloat;
	case EDT_Double:
		return (TYPE)v.fDouble;

	case EDT_UInt:
	case EDT_UShort:
	case EDT_UChar:
		return (TYPE)v.iUInt;

	case EDT_Int:
	case EDT_Short:
	case EDT_Char:
		return (TYPE)v.iInt;
	};
	return (TYPE)0;
}
// ---------------------------------------------------------------------------------
inline bool IsSpace( const char in)
{
	return (in == ' ' || in == '\t');
}
// ---------------------------------------------------------------------------------
inline bool IsLineEnd( const char in)
{
	return (in == '\r' || in == '\n' || in == '\0');
}
// ---------------------------------------------------------------------------------
inline bool IsSpaceOrNewLine( const char in)
{
	return IsSpace(in) || IsLineEnd(in);
}
// ---------------------------------------------------------------------------------
inline bool SkipSpaces( const char* in, const char** out)
{
	while (*in == ' ' || *in == '\t')in++;
	*out = in;
	return !IsLineEnd(*in);
}
// ---------------------------------------------------------------------------------
inline bool SkipLine( const char* in, const char** out)
{
	while (*in != '\r' && *in != '\n' && *in != '\0')in++;

	if (*in == '\0')
	{
		*out = in;
		return false;
	}
	in++;
	*out = in;
	return true;
}
// ---------------------------------------------------------------------------------
inline void SkipSpacesAndLineEnd( const char* in, const char** out)
{
	while (*in == ' ' || *in == '\t' || *in == '\r' || *in == '\n')in++;
	*out = in;
}

};
};

#endif // !! include guard