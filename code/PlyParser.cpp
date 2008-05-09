/** @file Implementation of the PLY parser class */
#include "PLYLoader.h"
#include "MaterialSystem.h"
#include "fast_atof.h"

#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"

#include <boost/scoped_ptr.hpp>

using namespace Assimp;


// ------------------------------------------------------------------------------------------------
PLY::EDataType PLY::Property::ParseDataType(const char* p_szIn,const char** p_szOut)
{
	PLY::EDataType eOut = PLY::EDT_INVALID;

	if (0 == ASSIMP_strincmp(p_szIn,"char",4) ||
		0 == ASSIMP_strincmp(p_szIn,"int8",4))
	{
		p_szIn+=4;
		eOut = PLY::EDT_Char;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"uchar",5) ||
			 0 == ASSIMP_strincmp(p_szIn,"uint8",5))
	{
		p_szIn+=5;
		eOut = PLY::EDT_UChar;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"short",5) ||
			 0 == ASSIMP_strincmp(p_szIn,"int16",5))
	{
		p_szIn+=5;
		eOut = PLY::EDT_Short;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"ushort",6) ||
			 0 == ASSIMP_strincmp(p_szIn,"uint16",6))
	{
		p_szIn+=6;
		eOut = PLY::EDT_UShort;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"int32",5))
	{
		p_szIn+=5;
		eOut = PLY::EDT_Int;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"uint32",6))
	{
		p_szIn+=6;
		eOut = PLY::EDT_UInt;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"int",3))
	{
		p_szIn+=3;
		eOut = PLY::EDT_Int;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"uint",4))
	{
		p_szIn+=4;
		eOut = PLY::EDT_UInt;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"float32",7))
	{
		p_szIn+=7;
		eOut = PLY::EDT_Float;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"float",5))
	{
		p_szIn+=5;
		eOut = PLY::EDT_Float;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"float64",7))
	{
		p_szIn+=7;
		eOut = PLY::EDT_Double;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"double64",8))
	{
		p_szIn+=8;
		eOut = PLY::EDT_Double;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"double",6))
	{
		p_szIn+=6;
		eOut = PLY::EDT_Double;
	}
	// either end of line or space, but no other characters allowed
	if (!(IsSpace(*p_szIn) || IsLineEnd(*p_szIn)))
	{
		eOut = PLY::EDT_INVALID;
	}
	*p_szOut = p_szIn;
	return eOut;
}
// ------------------------------------------------------------------------------------------------
PLY::ESemantic PLY::Property::ParseSemantic(const char* p_szIn,const char** p_szOut)
{
	PLY::ESemantic eOut = PLY::EST_INVALID;
	if (0 == ASSIMP_strincmp(p_szIn,"x",1))
	{
		p_szIn++;
		eOut = PLY::EST_XCoord;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"y",1))
	{
		p_szIn++;
		eOut = PLY::EST_YCoord;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"z",1))
	{
		p_szIn++;
		eOut = PLY::EST_ZCoord;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"red",3))
	{
		p_szIn+=3;
		eOut = PLY::EST_Red;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"green",4))
	{
		p_szIn+=5;
		eOut = PLY::EST_Green;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"blue",4))
	{
		p_szIn+=4;
		eOut = PLY::EST_Blue;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"alpha",5))
	{
		p_szIn+=5;
		eOut = PLY::EST_Alpha;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"vertex_index",12))
	{
		p_szIn+=12;
		eOut = PLY::EST_VertexIndex;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"vertex_indices",14))
	{
		p_szIn+=14;
		eOut = PLY::EST_VertexIndex;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"material_index",14))
	{
		p_szIn+=14;
		eOut = PLY::EST_MaterialIndex;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"ambient_red",11))
	{
		p_szIn+=11;
		eOut = PLY::EST_AmbientRed;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"ambient_green",13))
	{
		p_szIn+=13;
		eOut = PLY::EST_AmbientGreen;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"ambient_blue",12))
	{
		p_szIn+=12;
		eOut = PLY::EST_AmbientBlue;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"ambient_alpha",13))
	{
		p_szIn+=13;
		eOut = PLY::EST_AmbientAlpha;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"diffuse_red",11))
	{
		p_szIn+=11;
		eOut = PLY::EST_DiffuseRed;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"diffuse_green",13))
	{
		p_szIn+=13;
		eOut = PLY::EST_DiffuseGreen;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"diffuse_blue",12))
	{
		p_szIn+=12;
		eOut = PLY::EST_DiffuseBlue;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"diffuse_alpha",13))
	{
		p_szIn+=13;
		eOut = PLY::EST_DiffuseAlpha;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"specular_red",12))
	{
		p_szIn+=12;
		eOut = PLY::EST_SpecularRed;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"specular_green",14))
	{
		p_szIn+=14;
		eOut = PLY::EST_SpecularGreen;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"specular_blue",13))
	{
		p_szIn+=13;
		eOut = PLY::EST_SpecularBlue;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"specular_alpha",14))
	{
		p_szIn+=14;
		eOut = PLY::EST_SpecularAlpha;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"opacity",7))
	{
		p_szIn+=7;
		eOut = PLY::EST_Opacity;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"specular_power",6))
	{
		p_szIn+=7;
		eOut = PLY::EST_PhongPower;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"r",1))
	{
		p_szIn++;
		eOut = PLY::EST_Red;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"g",1))
	{
		p_szIn++;
		eOut = PLY::EST_Green;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"b",1))
	{
		p_szIn++;
		eOut = PLY::EST_Blue;
	}
	else
	{
		// ... find the next space or new line
		while (*p_szIn != ' ' && *p_szIn != '\t'  && 
			   *p_szIn != '\r' && *p_szIn != '\0' && *p_szIn != '\n')p_szIn++;
	}
	// either end of line or space, but no other characters allowed
	if (!(IsSpace(*p_szIn) || IsLineEnd(*p_szIn)))
	{
		eOut = PLY::EST_INVALID;
	}
	*p_szOut = p_szIn;
	return eOut;
}
// ------------------------------------------------------------------------------------------------
bool PLY::Property::ParseProperty (const char* p_szIn, const char** p_szOut, PLY::Property* pOut)
{
	// Forms supported:
	// "property float x"
	// "property list uchar int vertex_index"
	*p_szOut = p_szIn;

	// skip leading spaces
	if (!SkipSpaces(p_szIn,&p_szIn))return false;

	// skip the "property" string at the beginning
	if (0 != ASSIMP_strincmp(p_szIn,"property",8) || !IsSpace(*(p_szIn+8)))
	{
		// seems not to be a valid property entry
		return false;
	}
	// get next word
	p_szIn += 9;
	if (!SkipSpaces(p_szIn,&p_szIn))return false;

	if (0 == ASSIMP_strincmp(p_szIn,"list",4) && IsSpace(*(p_szIn+4)))
	{
		pOut->bIsList = true;

		// seems to be a list.
		p_szIn += 5;
		if(EDT_INVALID == (pOut->eFirstType = PLY::Property::ParseDataType(p_szIn, &p_szIn)))
		{
			// unable to parse list size data type
			SkipLine(p_szIn,&p_szIn);
			*p_szOut = p_szIn;
			return false;
		}
		if (!SkipSpaces(p_szIn,&p_szIn))return false;
		if(EDT_INVALID == (pOut->eType = PLY::Property::ParseDataType(p_szIn, &p_szIn)))
		{
			// unable to parse list data type
			SkipLine(p_szIn,&p_szIn);
			*p_szOut = p_szIn;
			return false;
		}
	}
	else
	{
		if(EDT_INVALID == (pOut->eType = PLY::Property::ParseDataType(p_szIn, &p_szIn)))
		{
			// unable to parse data type. Skip the property
			SkipLine(p_szIn,&p_szIn);
			*p_szOut = p_szIn;
			return false;
		}
	}
	
	if (!SkipSpaces(p_szIn,&p_szIn))return false;
	const char* szCur = p_szIn;
	pOut->Semantic = PLY::Property::ParseSemantic(p_szIn, &p_szIn);

	if (PLY::EST_INVALID == pOut->Semantic)
	{
		// store the name of the semantic
		uintptr_t iDiff = (uintptr_t)p_szIn - (uintptr_t)szCur;

		pOut->szName = std::string(szCur,iDiff);
	}

	SkipSpacesAndLineEnd(p_szIn,&p_szIn);
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
PLY::EElementSemantic PLY::Element::ParseSemantic(const char* p_szIn,const char** p_szOut)
{
	PLY::EElementSemantic eOut = PLY::EEST_INVALID;
	if (0 == ASSIMP_strincmp(p_szIn,"vertex",6))
	{
		p_szIn+=6;
		eOut = PLY::EEST_Vertex;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"face",4))
	{
		p_szIn+=4;
		eOut = PLY::EEST_Face;
	}
#if 0
	else if (0 == ASSIMP_strincmp(p_szIn,"range_grid",10))
	{
		p_szIn+=10;
		eOut = PLY::EEST_Face;
	}
#endif
	else if (0 == ASSIMP_strincmp(p_szIn,"tristrips",9))
	{
		p_szIn+=9;
		eOut = PLY::EEST_TriStrip;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"edge",4))
	{
		p_szIn+=4;
		eOut = PLY::EEST_Edge;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"material",8))
	{
		p_szIn+=8;
		eOut = PLY::EEST_Material;
	}

	// either end of line or space, but no other characters allowed
	if (!(IsSpace(*p_szIn) || IsLineEnd(*p_szIn)))
	{
		eOut = PLY::EEST_INVALID;
	}
	*p_szOut = p_szIn;
	return eOut;
}
// ------------------------------------------------------------------------------------------------
bool PLY::Element::ParseElement (const char* p_szIn, const char** p_szOut,
								 PLY::Element* pOut)
{
	// Example format: "element vertex 8"
	*p_szOut = p_szIn;

	// skip leading spaces
	if (!SkipSpaces(p_szIn,&p_szIn))return false;

	// skip the "element" string at the beginning
	if (0 != ASSIMP_strincmp(p_szIn,"element",7) || !IsSpace(*(p_szIn+7)))
	{
		// seems not to be a valid property entry
		return false;
	}
	// get next word
	p_szIn += 8;
	if (!SkipSpaces(p_szIn,&p_szIn))return false;

	// parse the semantic of the element
	const char* szCur = p_szIn;
	pOut->eSemantic = PLY::Element::ParseSemantic(p_szIn,&p_szIn);

	if (PLY::EEST_INVALID == pOut->eSemantic)
	{
		// store the name of the semantic
		uintptr_t iDiff = (uintptr_t)p_szIn - (uintptr_t)szCur;

		pOut->szName = std::string(szCur,iDiff);
	}

	if (!SkipSpaces(p_szIn,&p_szIn))return false;
	
	//parse the number of occurences of this element
	pOut->NumOccur = strtol10(p_szIn,&p_szIn);

	// go to the next line
	SkipSpacesAndLineEnd(p_szIn,&p_szIn);

	// now parse all properties of the element
	while(true)
	{
		// skip all comments
		PLY::DOM::SkipComments(p_szIn,&p_szIn);

		Property prop;
		if(!PLY::Property::ParseProperty(p_szIn,&p_szIn,&prop))break;

		// add the property to the property list
		pOut->alProperties.push_back(prop);
	}
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::DOM::SkipComments (const char* p_szIn,const char** p_szOut)
{
	*p_szOut = p_szIn;

	// skip spaces
	if (!SkipSpaces(p_szIn,&p_szIn))return false;

	if (0 == ASSIMP_strincmp(p_szIn,"comment",7))
	{
		p_szIn += 7;

		SkipLine(p_szIn,&p_szIn);

		SkipComments(p_szIn,&p_szIn);
		*p_szOut = p_szIn;
		return true;
	}
	return false;
}
// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseHeader (const char* p_szIn,const char** p_szOut)
{
	// after ply and format line
	*p_szOut = p_szIn;

	// parse all elements
	while (true)
	{
		// skip all comments
		PLY::DOM::SkipComments(p_szIn,&p_szIn);

		Element out;
		if(PLY::Element::ParseElement(p_szIn,&p_szIn,&out))
		{
			// add the element to the list of elements
			this->alElements.push_back(out);
		}
		else if (0 == ASSIMP_strincmp(p_szIn,"end_header",10) && IsSpaceOrNewLine(*(p_szIn+10)))
		{
			// we have reached the end of the header
			p_szIn += 11;
			break;
		}
		// ignore unknown header elements
	}
	SkipSpacesAndLineEnd(p_szIn,&p_szIn);
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseElementInstanceLists (const char* p_szIn,const char** p_szOut)
{
	this->alElementData.resize(this->alElements.size());

	std::vector<PLY::Element>::const_iterator i			=  this->alElements.begin();
	std::vector<PLY::ElementInstanceList>::iterator a	=  this->alElementData.begin();

	// parse all element instances
	for (;i != this->alElements.end();++i,++a)
	{
		*a = PLY::ElementInstanceList(&(*i)); // reserve enough storage
		PLY::ElementInstanceList::ParseInstanceList(p_szIn,&p_szIn,&(*i),&(*a));
	}
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseElementInstanceListsBinary (const char* p_szIn,const char** p_szOut,bool p_bBE)
{
	this->alElementData.resize(this->alElements.size());

	std::vector<PLY::Element>::const_iterator i			=  this->alElements.begin();
	std::vector<PLY::ElementInstanceList>::iterator a	=  this->alElementData.begin();

	// parse all element instances
	for (;i != this->alElements.end();++i,++a)
	{
		*a = PLY::ElementInstanceList(&(*i)); // reserve enough storage
		PLY::ElementInstanceList::ParseInstanceListBinary(p_szIn,&p_szIn,&(*i),&(*a),p_bBE);
	}
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseInstanceBinary (const char* p_szIn,DOM* p_pcOut,bool p_bBE)
{
	if(!p_pcOut->ParseHeader(p_szIn,&p_szIn))
	{
		return false;
	}
	if(!p_pcOut->ParseElementInstanceListsBinary(p_szIn,&p_szIn,p_bBE))
	{
		return false;
	}
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseInstance (const char* p_szIn,DOM* p_pcOut)
{
	if(!p_pcOut->ParseHeader(p_szIn,&p_szIn))
	{
		return false;
	}
	if(!p_pcOut->ParseElementInstanceLists(p_szIn,&p_szIn))
	{
		return false;
	}
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::ElementInstanceList::ParseInstanceList (const char* p_szIn,const char** p_szOut,
	 const PLY::Element* pcElement, PLY::ElementInstanceList* p_pcOut)
{
	if (EEST_INVALID == pcElement->eSemantic)
	{
		// if the element has an unknown semantic we can skip all lines
		// However, there could be comments
		for (unsigned int i = 0; i < pcElement->NumOccur;++i)
		{
			PLY::DOM::SkipComments(p_szIn,&p_szIn);
			SkipLine(p_szIn,&p_szIn);
		}
	}
	else
	{
		// be sure to have enough storage
		p_pcOut->alInstances.resize(pcElement->NumOccur);
		for (unsigned int i = 0; i < pcElement->NumOccur;++i)
		{
			PLY::DOM::SkipComments(p_szIn,&p_szIn);

			PLY::ElementInstance out;
			PLY::ElementInstance::ParseInstance(p_szIn, &p_szIn,pcElement, &out);
			// add it to the list
			p_pcOut->alInstances[i] = out;
		}
	}
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::ElementInstanceList::ParseInstanceListBinary (const char* p_szIn,const char** p_szOut,
	 const PLY::Element* pcElement, PLY::ElementInstanceList* p_pcOut,bool p_bBE)
{
	// we can add special handling code for unknown element semantics since
	// we can't skip it as a whole block (we don't know its exact size
	// due to the fact that lists could be contained in the property list 
	// of the unknown element)
	for (unsigned int i = 0; i < pcElement->NumOccur;++i)
	{
		PLY::ElementInstance out;
		PLY::ElementInstance::ParseInstanceBinary(p_szIn, &p_szIn,pcElement, &out, p_bBE);
		// add it to the list
		p_pcOut->alInstances[i] = out;
	}
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::ElementInstance::ParseInstance (const char* p_szIn,const char** p_szOut,
	const PLY::Element* pcElement, PLY::ElementInstance* p_pcOut)
{
	if (!SkipSpaces(p_szIn, &p_szIn))return false;

	p_pcOut->alProperties.resize(pcElement->alProperties.size());

	*p_szOut = p_szIn;
	std::vector<PLY::PropertyInstance>::iterator		i =  p_pcOut->alProperties.begin();
	std::vector<PLY::Property>::const_iterator			a =  pcElement->alProperties.begin();
	for (;i != p_pcOut->alProperties.end();++i,++a)
	{
		if(!(PLY::PropertyInstance::ParseInstance(p_szIn, &p_szIn,&(*a),&(*i))))
		{
			// skip the rest of the instance
			SkipLine(p_szIn, &p_szIn);

			PLY::PropertyInstance::ValueUnion v = PLY::PropertyInstance::DefaultValue((*a).eType);
			(*i).avList.push_back(v);
		}
	}
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::ElementInstance::ParseInstanceBinary (const char* p_szIn,const char** p_szOut,
	const PLY::Element* pcElement, PLY::ElementInstance* p_pcOut, bool p_bBE)
{
	p_pcOut->alProperties.resize(pcElement->alProperties.size());

	*p_szOut = p_szIn;
	std::vector<PLY::PropertyInstance>::iterator		i =  p_pcOut->alProperties.begin();
	std::vector<PLY::Property>::const_iterator			a =  pcElement->alProperties.begin();
	for (;i != p_pcOut->alProperties.end();++i,++a)
	{
		if(!(PLY::PropertyInstance::ParseInstance(p_szIn, &p_szIn,&(*a),&(*i))))
		{
			PLY::PropertyInstance::ValueUnion v = PLY::PropertyInstance::DefaultValue((*a).eType);
			(*i).avList.push_back(v);
		}
	}
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::PropertyInstance::ParseInstance (const char* p_szIn,const char** p_szOut,
	const PLY::Property* prop, PLY::PropertyInstance* p_pcOut)
{
	*p_szOut = p_szIn;

	// skip spaces at the beginning
	if (!SkipSpaces(p_szIn, &p_szIn))return false;

	if (prop->bIsList)
	{
		// parse the number of elements in the list
		PLY::PropertyInstance::ValueUnion v;
		PLY::PropertyInstance::ParseValue(p_szIn, &p_szIn,prop->eFirstType,&v);

		// convert to unsigned int
		unsigned int iNum = PLY::PropertyInstance::ConvertTo<unsigned int>(v,prop->eFirstType);

		// parse all list elements
		for (unsigned int i = 0; i < iNum;++i)
		{
			if (!SkipSpaces(p_szIn, &p_szIn))return false;

			PLY::PropertyInstance::ParseValue(p_szIn, &p_szIn,prop->eType,&v);
			p_pcOut->avList.push_back(v);
		}
	}
	else
	{
		// parse the property
		PLY::PropertyInstance::ValueUnion v;
		PLY::PropertyInstance::ParseValue(p_szIn, &p_szIn,prop->eType,&v);
		p_pcOut->avList.push_back(v);
	}
	SkipSpacesAndLineEnd(p_szIn, &p_szIn);
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::PropertyInstance::ParseInstanceBinary (const char* p_szIn,const char** p_szOut,
	const PLY::Property* prop, PLY::PropertyInstance* p_pcOut,bool p_bBE)
{
	*p_szOut = p_szIn;

	if (prop->bIsList)
	{
		// parse the number of elements in the list
		PLY::PropertyInstance::ValueUnion v;
		PLY::PropertyInstance::ParseValueBinary(p_szIn, &p_szIn,prop->eFirstType,&v,p_bBE);

		// convert to unsigned int
		unsigned int iNum = PLY::PropertyInstance::ConvertTo<unsigned int>(v,prop->eFirstType);

		// parse all list elements
		for (unsigned int i = 0; i < iNum;++i)
		{
			PLY::PropertyInstance::ParseValueBinary(p_szIn, &p_szIn,prop->eType,&v,p_bBE);
			p_pcOut->avList.push_back(v);
		}
	}
	else
	{
		// parse the property
		PLY::PropertyInstance::ValueUnion v;
		PLY::PropertyInstance::ParseValueBinary(p_szIn, &p_szIn,prop->eType,&v,p_bBE);
		p_pcOut->avList.push_back(v);
	}
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
PLY::PropertyInstance::ValueUnion PLY::PropertyInstance::DefaultValue(
	PLY::EDataType eType)
{
	PLY::PropertyInstance::ValueUnion out;

	switch (eType)
	{
	case EDT_Float:
		out.fFloat = 0.0f;
		return out;

	case EDT_Double:
		out.fDouble = 0.0;
		return out;
	};
	out.iUInt = 0;
	return out;
}
// ------------------------------------------------------------------------------------------------
bool PLY::PropertyInstance::ParseValue(const char* p_szIn,const char** p_szOut,
	PLY::EDataType eType,PLY::PropertyInstance::ValueUnion* out)
{
	*p_szOut = p_szIn;

	switch (eType)
	{
	case EDT_UInt:
	case EDT_UShort:
	case EDT_UChar:

		// simply parse in a full uint
		out->iUInt = (uint32_t)strtol10(p_szIn, &p_szIn);

		break;

	case EDT_Int:
	case EDT_Short:
	case EDT_Char:

		{
		// simply parse in a full int
		// Take care of the sign at the beginning
		bool bMinus = false;
		if (*p_szIn == '-')
		{
			p_szIn++;
			bMinus = true;
		}
		out->iInt = (int32_t)strtol10(p_szIn, &p_szIn);
		if (bMinus)out->iInt *= -1;

		break;
		}

	case EDT_Float:

		// parse a simple float
		p_szIn = fast_atof_move(p_szIn,out->fFloat);
		break;

	case EDT_Double:

		// Parse a double float. .. TODO: support this
		float f;
		p_szIn = fast_atof_move(p_szIn,f);
		out->fDouble = (double)f;

	default:
		return false;
	}
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::PropertyInstance::ParseValueBinary(const char* p_szIn,const char** p_szOut,
	PLY::EDataType eType,PLY::PropertyInstance::ValueUnion* out, bool p_bBE)
{
	*p_szOut = p_szIn;

	switch (eType)
	{
	case EDT_UInt:
		out->iUInt = (uint32_t)*((uint32_t*)p_szIn);
		p_szIn += 4;
		
		if (p_bBE)
		{
			std::swap(((unsigned char*)(&out->iUInt))[0],((unsigned char*)(&out->iUInt))[3]);
			std::swap(((unsigned char*)(&out->iUInt))[1],((unsigned char*)(&out->iUInt))[2]);
		}
		break;

	case EDT_UShort:
		{
		uint16_t i = *((uint16_t*)p_szIn);
		if (p_bBE)
			{
			std::swap(((unsigned char*)(&i))[0],((unsigned char*)(&i))[1]);
			}
		out->iUInt = (uint32_t)i;
		p_szIn += 2;
		break;
		}

	case EDT_UChar:
		{
		uint8_t i = *((uint8_t*)p_szIn);
		out->iUInt = (uint32_t)i;
		p_szIn += 2;
		break;
		}

	case EDT_Int:
		out->iInt = *((int32_t*)p_szIn);
		p_szIn += 4;
		
		if (p_bBE)
		{
			std::swap(((unsigned char*)(&out->iInt))[0],((unsigned char*)(&out->iInt))[3]);
			std::swap(((unsigned char*)(&out->iInt))[1],((unsigned char*)(&out->iInt))[2]);
		}
		break;

	case EDT_Short:
		{
		int16_t i = *((int16_t*)p_szIn);
		if (p_bBE)
			{
			std::swap(((unsigned char*)(&i))[0],((unsigned char*)(&i))[1]);
			}
		out->iInt = (int32_t)i;
		p_szIn += 2;
		break;
		}

	case EDT_Char:
		out->iInt = (int32_t)*((int8_t*)p_szIn);
		p_szIn += 1;
		break;

	case EDT_Float:
		if (p_bBE)
		{
			union {char szArray[4]; float fValue; } _X;

			_X.szArray[0] = ((unsigned char*)p_szIn)[3];
			_X.szArray[1] = ((unsigned char*)p_szIn)[2];
			_X.szArray[2] = ((unsigned char*)p_szIn)[1];
			_X.szArray[3] = ((unsigned char*)p_szIn)[0];
			out->fFloat = _X.fValue;
		}
		else out->fFloat = *((float*)p_szIn);
		p_szIn += 4;
		break;

	case EDT_Double:
		if (p_bBE)
		{
			union {char szArray[8]; double fValue; } _X;

			_X.szArray[0] = ((unsigned char*)p_szIn)[7];
			_X.szArray[1] = ((unsigned char*)p_szIn)[6];
			_X.szArray[2] = ((unsigned char*)p_szIn)[5];
			_X.szArray[3] = ((unsigned char*)p_szIn)[4];
			_X.szArray[4] = ((unsigned char*)p_szIn)[3];
			_X.szArray[5] = ((unsigned char*)p_szIn)[2];
			_X.szArray[6] = ((unsigned char*)p_szIn)[1];
			_X.szArray[7] = ((unsigned char*)p_szIn)[0];
			out->fDouble = _X.fValue;
		}
		else out->fDouble = *((double*)p_szIn);
		p_szIn += 8;
		break;

	default:
		return false;
	}
	*p_szOut = p_szIn;
	return true;
}