//-------------------------------------------------------------------------------
/**
*	This program is distributed under the terms of the GNU Lesser General
*	Public License (LGPL). 
*
*	ASSIMP Viewer Utility
*
*/
//-------------------------------------------------------------------------------

#include "stdafx.h"
#include "assimp_view.h"


namespace AssimpView {

//
// Specifies the number of different shaders generated for
// the current asset. This number is incremented by CreateMaterial()
// each time a shader isn't found in cache and needs to be created
//
unsigned int g_iShaderCount			 = 0 ;

//-------------------------------------------------------------------------------
// Compiler idependent stricmp() function.
//
// Used for case insensitive string comparison
//-------------------------------------------------------------------------------
inline int ASSIMP_stricmp(const char *s1, const char *s2)
	{
	const char *a1, *a2;
	a1 = s1;
	a2 = s2;

	while (true)
		{
		char c1 = (char)tolower(*a1); 
		char c2 = (char)tolower(*a2);
		if ((0 == c1) && (0 == c2)) return 0;
		if (c1 < c2) return-1;
		if (c1 > c2) return 1;
		++a1; 
		++a2;
		}
	}

//-------------------------------------------------------------------------------
// D3DX callback function to fill a texture with a checkers pattern
//
// This pattern is used to mark textures which could not be loaded
//-------------------------------------------------------------------------------
VOID WINAPI FillFunc(D3DXVECTOR4* pOut, 
					 CONST D3DXVECTOR2* pTexCoord, 
					 CONST D3DXVECTOR2* pTexelSize, 
					 LPVOID pData)
	{
	UNREFERENCED_PARAMETER(pData);
	UNREFERENCED_PARAMETER(pTexelSize);

	// generate a nice checker pattern (yellow/black)
	// size of a square: 32 * 32 px
	unsigned int iX = (unsigned int)(pTexCoord->x * 256.0f);
	unsigned int iY = (unsigned int)(pTexCoord->y * 256.0f);

	bool bBlack = false;
	if ((iX / 32) % 2 == 0)
		{
		if ((iY / 32) % 2 == 0)bBlack = true;
		}
	else 
		{
		if ((iY / 32) % 2 != 0)bBlack = true;
		}
	pOut->w = 1.0f;
	if (bBlack)
		{
		pOut->x = pOut->y = pOut->z = 0.0f;
		}
	else
		{
		pOut->x = pOut->y = 1.0f;
		pOut->z = 0.0f;
		}
	return;
	}

//-------------------------------------------------------------------------------
// Setup the default texture for a texture channel
//
// Generates a default checker pattern for a texture
//-------------------------------------------------------------------------------
int SetDefaultTexture(IDirect3DTexture9** p_ppiOut)
	{
	if(FAILED(g_piDevice->CreateTexture(
		256,
		256,
		0,
		0,
		D3DFMT_A8R8G8B8,
		D3DPOOL_MANAGED,
		p_ppiOut,
		NULL)))
		{
		CLogDisplay::Instance().AddEntry("[ERROR] Unable to create default texture",
			D3DCOLOR_ARGB(0xFF,0xFF,0,0));
		}
	D3DXFillTexture(*p_ppiOut,&FillFunc,NULL);
	return 1;
	}


//-------------------------------------------------------------------------------
// find a valid path to a texture file
//
// Handle 8.3 syntax correctly, search the environment of the
// executable and the asset for a texture with a name very similar to a given one
//-------------------------------------------------------------------------------
bool TryLongerPath(char* szTemp,aiString* p_szString)
	{
	char szTempB[MAX_PATH];

	strcpy(szTempB,szTemp);

	// go to the beginning of the file name
	char* szFile = strrchr(szTempB,'\\');
	if (!szFile)szFile = strrchr(szTempB,'/');
	
	char* szFile2 = szTemp + (szFile - szTempB)+1;
	szFile++;
	char* szExt = strrchr(szFile,'.')+1;
	*szFile = 0;

	strcat(szTempB,"*.*");
	const unsigned int iSize = (const unsigned int) ( szExt - 1 - szFile );

	HANDLE          h;
	WIN32_FIND_DATA info;

	// build a list of files
	h = FindFirstFile(szTempB, &info);
	if (h != INVALID_HANDLE_VALUE)
		{
		do
			{
			if (!(strcmp(info.cFileName, ".") == 0 || strcmp(info.cFileName, "..") == 0))
				{
				char* szExtFound = strrchr(info.cFileName, '.')+1;
				if ((char*)0x1 != szExtFound)
					{
					if (0 == ASSIMP_stricmp(szExtFound,szExt))
						{
						const unsigned int iSizeFound = (const unsigned int) ( 
							szExtFound - 1 - info.cFileName);

						for (unsigned int i = 0; i < iSizeFound;++i)
							info.cFileName[i] = (CHAR)tolower(info.cFileName[i]);

						if (0 == memcmp(info.cFileName,szFile2, std::min(iSizeFound,iSize)))
							{
							// we have it. Build the full path ...
							char* sz = strrchr(szTempB,'*');
							*(sz-2) = 0x0;

							strcat(szTempB,info.cFileName);

							// copy the result string back to the aiString
							const size_t iLen = strlen(szTempB);
							size_t iLen2 = iLen+1;
							iLen2 = iLen2 > MAXLEN ? MAXLEN : iLen2;
							memcpy(p_szString->data,szTempB,iLen2);
							p_szString->length = iLen;
							return true;
							}
						}
					// check whether the 8.3 DOS name is matching
					if (0 == ASSIMP_stricmp(info.cAlternateFileName,p_szString->data))
						{
						strcat(szTempB,info.cAlternateFileName);

						// copy the result string back to the aiString
						const size_t iLen = strlen(szTempB);
						size_t iLen2 = iLen+1;
						iLen2 = iLen2 > MAXLEN ? MAXLEN : iLen2;
						memcpy(p_szString->data,szTempB,iLen2);
						p_szString->length = iLen;
						return true;
						}
					}
				}
			} 
		while (FindNextFile(h, &info));

		FindClose(h);
		}
	return false;
	}

//-------------------------------------------------------------------------------
// find a valid path to a texture file
//
// Handle 8.3 syntax correctly, search the environment of the
// executable and the asset for a texture with a name very similar to a given one
//-------------------------------------------------------------------------------
int FindValidPath(aiString* p_szString)
	{
	// first check whether we can directly load the file
	FILE* pFile = fopen(p_szString->data,"rb");
	if (pFile)fclose(pFile);
	else
		{
		// check whether we can use the directory of 
		// the asset as relative base
		char szTemp[MAX_PATH*2];
		strcpy(szTemp, g_szFileName);

		char* szData = p_szString->data;
		if (*szData == '\\' || *szData == '/')++szData;

		char* szEnd = strrchr(szTemp,'\\');
		if (!szEnd)
			{
			szEnd = strrchr(szTemp,'/');
			if (!szEnd)szEnd = szTemp;
			}
		szEnd++;
		*szEnd = 0;
		strcat(szEnd,szData);

		pFile = fopen(szTemp,"rb");
		if (!pFile)
			{
			// convert the string to lower case
			for (unsigned int i = 0;;++i)
				{
				if ('\0' == szTemp[i])break;
				szTemp[i] = (char)tolower(szTemp[i]);
				}

			if(TryLongerPath(szTemp,p_szString))return 1;
			*szEnd = 0;

			// search common sub directories
			strcat(szEnd,"tex\\");
			strcat(szEnd,szData);

			pFile = fopen(szTemp,"rb");
			if (!pFile)
				{
				if(TryLongerPath(szTemp,p_szString))return 1;

				*szEnd = 0;

				strcat(szEnd,"textures\\");
				strcat(szEnd,szData);

				pFile = fopen(szTemp,"rb");
				if (!pFile)
					{
					if(TryLongerPath(szTemp, p_szString))return 1;
				
					// still unable to load ... however, don't spew
					// an error message here, simply let it and wait for
					// D3DXCreateTextureFromFileEx() to fail ;-)
					}
				return 0;
				}
			}
		fclose(pFile);

		// copy the result string back to the aiString
		const size_t iLen = strlen(szTemp);
		size_t iLen2 = iLen+1;
		iLen2 = iLen2 > MAXLEN ? MAXLEN : iLen2;
		memcpy(p_szString->data,szTemp,iLen2);
		p_szString->length = iLen;
		}
	return 1;
	}

//-------------------------------------------------------------------------------
// Load a texture into memory and create a native D3D texture resource
//
// The function tries to find a valid path for a texture
//-------------------------------------------------------------------------------
int LoadTexture(IDirect3DTexture9** p_ppiOut,aiString* szPath)
	{
	// first get a valid path to the texture
	FindValidPath(szPath);

	*p_ppiOut = NULL;

	// then call D3DX to load the texture
	if (FAILED(D3DXCreateTextureFromFileEx(
		g_piDevice,
		szPath->data,
		D3DX_DEFAULT,
		D3DX_DEFAULT,
		0,
		0,
		D3DFMT_A8R8G8B8,
		D3DPOOL_MANAGED,
		D3DX_DEFAULT,
		D3DX_DEFAULT,
		0,
		NULL,
		NULL,
		p_ppiOut)))
		{
		// error ... use the default texture instead
		std::string sz = "[ERROR] Unable to load texture: ";
		sz.append(szPath->data);
		CLogDisplay::Instance().AddEntry(sz,D3DCOLOR_ARGB(0xFF,0xFF,0x0,0x0));

		SetDefaultTexture(p_ppiOut);
		}
	return 1;
	}



//-------------------------------------------------------------------------------
// Delete all resources of a given material
//
// Must be called before CreateMaterial() to prevent memory leaking
//-------------------------------------------------------------------------------
void DeleteMaterial(AssetHelper::MeshHelper* pcIn)
	{
	if (!pcIn || !pcIn->piEffect)return;
	pcIn->piEffect->Release();

	// release all textures associated with the material
	if (pcIn->piDiffuseTexture)
		{
		pcIn->piDiffuseTexture->Release();
		pcIn->piDiffuseTexture = NULL;
		}
	if (pcIn->piSpecularTexture)
		{
		pcIn->piSpecularTexture->Release();
		pcIn->piSpecularTexture = NULL;
		}
	if (pcIn->piEmissiveTexture)
		{
		pcIn->piEmissiveTexture->Release();
		pcIn->piEmissiveTexture = NULL;
		}
	if (pcIn->piAmbientTexture)
		{
		pcIn->piAmbientTexture->Release();
		pcIn->piAmbientTexture = NULL;
		}
	if (pcIn->piOpacityTexture)
		{
		pcIn->piOpacityTexture->Release();
		pcIn->piOpacityTexture = NULL;
		}
	if (pcIn->piNormalTexture)
		{
		pcIn->piNormalTexture->Release();
		pcIn->piNormalTexture = NULL;
		}
	}


//-------------------------------------------------------------------------------
// Convert a height map to a normal map if necessary
//
// The function tries to detect the type of a texture automatically.
// However, this wont work in every case.
//-------------------------------------------------------------------------------
void HMtoNMIfNecessary(IDirect3DTexture9* piTexture,IDirect3DTexture9** piTextureOut,
					   bool bWasOriginallyHM = true)
	{
	bool bMustConvert = false;
	uintptr_t iElement = 3;

	*piTextureOut = piTexture;

	// Lock the input texture and try to determine its type.
	// Criterias:
	// - If r,g,b channel are identical it MUST be a height map
	// - If one of the rgb channels is used and the others are empty it
	//   must be a height map, too.
	// - If the average color of the whole image is something inside the
	//   purple range we can be sure it is a normal map
	//
	// - Otherwise we assume it is a normal map
	// To increase performance we take not every pixel

	D3DLOCKED_RECT sRect;
	D3DSURFACE_DESC sDesc;
	piTexture->GetLevelDesc(0,&sDesc);
	if (FAILED(piTexture->LockRect(0,&sRect,NULL,D3DLOCK_READONLY)))
		{
		return;
		}
	const int iPitchDiff = (int)sRect.Pitch - (int)(sDesc.Width * 4);

	struct SColor
		{
		union
			{
			struct {unsigned char b,g,r,a;};
			char _array[4];
			};
		};
	const SColor* pcData = (const SColor*)sRect.pBits;

	union
		{
		const SColor* pcPointer;
		const unsigned char* pcCharPointer;
		};
	pcPointer = pcData;

	// 1. If r,g,b channel are identical it MUST be a height map
	bool bIsEqual = true;
	for (unsigned int y = 0; y <  sDesc.Height;++y)
		{
		for (unsigned int x = 0; x <  sDesc.Width;++x)
			{
			if (pcPointer->b != pcPointer->r || pcPointer->b != pcPointer->g)
				{
				bIsEqual = false;
				break;
				}
			pcPointer++;
			}
		pcCharPointer += iPitchDiff;
		}
	if (bIsEqual)bMustConvert = true;
	else
		{
		// 2. If one of the rgb channels is used and the others are empty it
		//    must be a height map, too.
		pcPointer = pcData;
		while (*pcCharPointer == 0)pcCharPointer++;

		iElement = (uintptr_t)(pcCharPointer - (unsigned char*)pcData) % 4;
		unsigned int aiIndex[3] = {0,1,2};
		if (3 != iElement)aiIndex[iElement] = 3;

		pcPointer = pcData;

		bIsEqual = true;
		if (3 != iElement)
			{
			for (unsigned int y = 0; y <  sDesc.Height;++y)
				{
				for (unsigned int x = 0; x <  sDesc.Width;++x)
					{
					for (unsigned int ii = 0; ii < 3;++ii)
						{
						// don't take the alpha channel into account.
						// if the texture was stored n RGB888 format D3DX has
						// converted it to ARGB8888 format with a fixed alpha channel
						if (aiIndex[ii] != 3 && pcPointer->_array[aiIndex[ii]] != 0)
							{
							bIsEqual = false;
							break;
							}
						}
					pcPointer++;
					}
				pcCharPointer += iPitchDiff;
				}
			if (bIsEqual)bMustConvert = true;
			else
				{
				// If the average color of the whole image is something inside the
				// purple range we can be sure it is a normal map

				// (calculate the average color line per line to prevent overflows!)
				pcPointer = pcData;
				aiColor3D clrColor;
				for (unsigned int y = 0; y <  sDesc.Height;++y)
					{
					aiColor3D clrColorLine;
					for (unsigned int x = 0; x <  sDesc.Width;++x)
						{
						clrColorLine.r += pcPointer->r;
						clrColorLine.g += pcPointer->g;
						clrColorLine.b += pcPointer->b;
						pcPointer++;
						}
					clrColor.r += clrColorLine.r /= (float)sDesc.Width;
					clrColor.g += clrColorLine.g /= (float)sDesc.Width;
					clrColor.b += clrColorLine.b /= (float)sDesc.Width;
					pcCharPointer += iPitchDiff;
					}
				clrColor.r /= (float)sDesc.Height;
				clrColor.g /= (float)sDesc.Height;
				clrColor.b /= (float)sDesc.Height;

				if (!(clrColor.b > 215 && 
					clrColor.r > 100 && clrColor.r < 140 && 
					clrColor.g > 100 && clrColor.g < 140))
					{
					// Unable to detect. Believe the original value obtained from the loader
					if (bWasOriginallyHM)
						{
						bMustConvert = true;
						}
					}
				}
			}
		}

	piTexture->UnlockRect(0);

	// if the input data is assumed to be a height map we'll
	// need to convert it NOW
	if (bMustConvert)
		{
		D3DSURFACE_DESC sDesc;
		piTexture->GetLevelDesc(0, &sDesc);

		IDirect3DTexture9* piTempTexture;
		if(FAILED(g_piDevice->CreateTexture(
			sDesc.Width,
			sDesc.Height,
			piTexture->GetLevelCount(),
			sDesc.Usage,
			sDesc.Format,
			sDesc.Pool, &piTempTexture, NULL)))
			{
			CLogDisplay::Instance().AddEntry(
				"[ERROR] Unable to create normal map texture",
				D3DCOLOR_ARGB(0xFF,0xFF,0x0,0x0));
			return;
			}

		DWORD dwFlags;
		if (3 == iElement)dwFlags = D3DX_CHANNEL_LUMINANCE;
		else if (2 == iElement)dwFlags = D3DX_CHANNEL_RED;
		else if (1 == iElement)dwFlags = D3DX_CHANNEL_GREEN;
		else /*if (0 == iElement)*/dwFlags = D3DX_CHANNEL_BLUE;

		if(FAILED(D3DXComputeNormalMap(piTempTexture,
			piTexture,NULL,0,dwFlags,1.0f)))
			{
			CLogDisplay::Instance().AddEntry(
				"[ERROR] Unable to compute normal map from height map",
				D3DCOLOR_ARGB(0xFF,0xFF,0x0,0x0));

			piTempTexture->Release();
			return;
			}
		*piTextureOut = piTempTexture;
		piTexture->Release();
		}
	}


//-------------------------------------------------------------------------------
// Search for non-opaque pixels in a texture
//
// A pixel is considered to be non-opaque if its alpha value s less than 255
//-------------------------------------------------------------------------------
bool HasAlphaPixels(IDirect3DTexture9* piTexture)
	{
	D3DLOCKED_RECT sRect;
	D3DSURFACE_DESC sDesc;
	piTexture->GetLevelDesc(0,&sDesc);
	if (FAILED(piTexture->LockRect(0,&sRect,NULL,D3DLOCK_READONLY)))
		{
		return false;
		}
	const int iPitchDiff = (int)sRect.Pitch - (int)(sDesc.Width * 4);

	struct SColor
		{
		unsigned char b,g,r,a;;
		};
	const SColor* pcData = (const SColor*)sRect.pBits;

	union
		{
		const SColor* pcPointer;
		const unsigned char* pcCharPointer;
		};
	pcPointer = pcData;
	for (unsigned int y = 0; y <  sDesc.Height;++y)
		{
		for (unsigned int x = 0; x <  sDesc.Width;++x)
			{
			if (pcPointer->a != 0xFF)
				{
				piTexture->UnlockRect(0);
				return true;
				}
			pcPointer++;
			}
		pcCharPointer += iPitchDiff;
		}
	piTexture->UnlockRect(0);
	return false;
	}


//-------------------------------------------------------------------------------
// Create the material for a mesh.
//
// The function checks whether an identical shader is already in use.
// A shader is considered to be identical if it has the same input signature
// and takes the same number of texture channels.
//-------------------------------------------------------------------------------
int CreateMaterial(AssetHelper::MeshHelper* pcMesh,const aiMesh* pcSource)
	{
	ID3DXBuffer* piBuffer;

	D3DXMACRO sMacro[32];

	// extract all properties from the ASSIMP material structure
	const aiMaterial* pcMat = g_pcAsset->pcScene->mMaterials[pcSource->mMaterialIndex];

	//
	// DIFFUSE COLOR --------------------------------------------------
	//
	if(AI_SUCCESS != aiGetMaterialColor(pcMat,AI_MATKEY_COLOR_DIFFUSE,
		(aiColor4D*)&pcMesh->vDiffuseColor))
		{
		pcMesh->vDiffuseColor.x = 1.0f;
		pcMesh->vDiffuseColor.y = 1.0f;
		pcMesh->vDiffuseColor.z = 1.0f;
		pcMesh->vDiffuseColor.w = 1.0f;
		}
	//
	// SPECULAR COLOR --------------------------------------------------
	//
	if(AI_SUCCESS != aiGetMaterialColor(pcMat,AI_MATKEY_COLOR_SPECULAR,
		(aiColor4D*)&pcMesh->vSpecularColor))
		{
		pcMesh->vSpecularColor.x = 1.0f;
		pcMesh->vSpecularColor.y = 1.0f;
		pcMesh->vSpecularColor.z = 1.0f;
		pcMesh->vSpecularColor.w = 1.0f;
		}
	//
	// AMBIENT COLOR --------------------------------------------------
	//
	if(AI_SUCCESS != aiGetMaterialColor(pcMat,AI_MATKEY_COLOR_AMBIENT,
		(aiColor4D*)&pcMesh->vAmbientColor))
		{
		pcMesh->vAmbientColor.x = 0.0f;
		pcMesh->vAmbientColor.y = 0.0f;
		pcMesh->vAmbientColor.z = 0.0f;
		pcMesh->vAmbientColor.w = 1.0f;
		}
	//
	// EMISSIVE COLOR -------------------------------------------------
	//
	if(AI_SUCCESS != aiGetMaterialColor(pcMat,AI_MATKEY_COLOR_EMISSIVE,
		(aiColor4D*)&pcMesh->vEmissiveColor))
		{
		pcMesh->vEmissiveColor.x = 0.0f;
		pcMesh->vEmissiveColor.y = 0.0f;
		pcMesh->vEmissiveColor.z = 0.0f;
		pcMesh->vEmissiveColor.w = 1.0f;
		}

	//
	// Opacity --------------------------------------------------------
	//
	if(AI_SUCCESS != aiGetMaterialFloat(pcMat,AI_MATKEY_OPACITY,&pcMesh->fOpacity))
		{
		pcMesh->fOpacity = 1.0f;
		}

	//
	// Shading Model --------------------------------------------------
	//
	bool bDefault = false;
	if(AI_SUCCESS != aiGetMaterialInteger(pcMat,AI_MATKEY_SHADING_MODEL,(int*)&pcMesh->eShadingMode ))
		{
		bDefault = true;
		pcMesh->eShadingMode = aiShadingMode_Gouraud;
		}


	//
	// Shininess ------------------------------------------------------
	//
	if(AI_SUCCESS != aiGetMaterialFloat(pcMat,AI_MATKEY_SHININESS,&pcMesh->fShininess))
		{
		// assume 15 as default shininess
		pcMesh->fShininess = 15.0f;
		}
	else if (bDefault)pcMesh->eShadingMode  = aiShadingMode_Phong;

	aiString szPath;

	//
	// DIFFUSE TEXTURE ------------------------------------------------
	//
	if(AI_SUCCESS == aiGetMaterialString(pcMat,AI_MATKEY_TEXTURE_DIFFUSE(0),&szPath))
		{
		LoadTexture(&pcMesh->piDiffuseTexture,&szPath);
		}

	//
	// SPECULAR TEXTURE ------------------------------------------------
	//
	if(AI_SUCCESS == aiGetMaterialString(pcMat,AI_MATKEY_TEXTURE_SPECULAR(0),&szPath))
		{
		LoadTexture(&pcMesh->piSpecularTexture,&szPath);
		}

	//
	// OPACITY TEXTURE ------------------------------------------------
	//
	if(AI_SUCCESS == aiGetMaterialString(pcMat,AI_MATKEY_TEXTURE_OPACITY(0),&szPath))
		{
		LoadTexture(&pcMesh->piOpacityTexture,&szPath);
		}
	else
		{
		// try to find out whether the diffuse texture has any
		// non-opaque pixels. If we find a few use it as opacity texture
		if (pcMesh->piDiffuseTexture && HasAlphaPixels(pcMesh->piDiffuseTexture))
			{
			pcMesh->piOpacityTexture = pcMesh->piDiffuseTexture;
			pcMesh->piOpacityTexture->AddRef();
			}
		}

	//
	// AMBIENT TEXTURE ------------------------------------------------
	//
	if(AI_SUCCESS == aiGetMaterialString(pcMat,AI_MATKEY_TEXTURE_AMBIENT(0),&szPath))
		{
		LoadTexture(&pcMesh->piAmbientTexture,&szPath);
		}

	//
	// EMISSIVE TEXTURE ------------------------------------------------
	//
	if(AI_SUCCESS == aiGetMaterialString(pcMat,AI_MATKEY_TEXTURE_EMISSIVE(0),&szPath))
		{
		LoadTexture(&pcMesh->piEmissiveTexture,&szPath);
		}

	//
	// NORMAL/HEIGHT MAP ------------------------------------------------
	//
	bool bHM = false;
	if(AI_SUCCESS == aiGetMaterialString(pcMat,AI_MATKEY_TEXTURE_NORMALS(0),&szPath))
		{
		LoadTexture(&pcMesh->piNormalTexture,&szPath);
		}
	else
		{
		if(AI_SUCCESS == aiGetMaterialString(pcMat,AI_MATKEY_TEXTURE_BUMP(0),&szPath))
			{
			LoadTexture(&pcMesh->piNormalTexture,&szPath);
			}
		bHM = true;
		}
	// normal/height maps are sometimes mixed up. Try to detect the type
	// of the texture automatically
	if (pcMesh->piNormalTexture)
		{
		HMtoNMIfNecessary(pcMesh->piNormalTexture, &pcMesh->piNormalTexture,bHM);
		}

	// check whether a global background texture is contained
	// in this material. Some loaders set this value ...
	if(AI_SUCCESS == aiGetMaterialString(pcMat,AI_MATKEY_GLOBAL_BACKGROUND_IMAGE,&szPath))
		{
		CBackgroundPainter::Instance().SetTextureBG(szPath.data);
		}

	// BUGFIX: If the shininess is 0.0f disable phong lighting
	// This is a workaround for some meshes in the DX SDK (e.g. tiny.x)
	if (0.0f == pcMesh->fShininess)
		{
		pcMesh->eShadingMode = aiShadingMode_Gouraud;
		}

	// check whether we have already a material using the same
	// shader. This will decrease loading time rapidly ...
	for (unsigned int i = 0; i < g_pcAsset->pcScene->mNumMeshes;++i)
		{
		if (g_pcAsset->pcScene->mMeshes[i] == pcSource)
			{
			break;
			}
		AssetHelper::MeshHelper* pc = g_pcAsset->apcMeshes[i];

		if  ((pcMesh->piDiffuseTexture != NULL ? true : false) != 
			(pc->piDiffuseTexture != NULL ? true : false))
			continue;
		if  ((pcMesh->piSpecularTexture != NULL ? true : false) != 
			(pc->piSpecularTexture != NULL ? true : false))
			continue;
		if  ((pcMesh->piAmbientTexture != NULL ? true : false) != 
			(pc->piAmbientTexture != NULL ? true : false))
			continue;
		if  ((pcMesh->piEmissiveTexture != NULL ? true : false) != 
			(pc->piEmissiveTexture != NULL ? true : false))
			continue;
		if  ((pcMesh->piNormalTexture != NULL ? true : false) != 
			(pc->piNormalTexture != NULL ? true : false))
			continue;
		if  ((pcMesh->piOpacityTexture != NULL ? true : false) != 
			(pc->piOpacityTexture != NULL ? true : false))
			continue;
		if ((pcMesh->eShadingMode != aiShadingMode_Gouraud ? true : false) != 
			(pc->eShadingMode != aiShadingMode_Gouraud ? true : false))
			continue;

		if ((pcMesh->fOpacity != 1.0f ? true : false) != (pc->fOpacity != 1.0f ? true : false))
			continue;

		// we can reuse this material
		if (pc->piEffect)
			{
			pcMesh->piEffect = pc->piEffect;
			pc->bSharedFX = pcMesh->bSharedFX = true;
			pcMesh->piEffect->AddRef();
			return 2;
			}
		}
	g_iShaderCount++;

	// build macros for the HLSL compiler
	unsigned int iCurrent = 0;
	if (pcMesh->piDiffuseTexture)
		{
		sMacro[iCurrent].Name = "AV_DIFFUSE_TEXTURE";
		sMacro[iCurrent].Definition = "1";
		++iCurrent;
		}
	if (pcMesh->piSpecularTexture)
		{
		sMacro[iCurrent].Name = "AV_SPECULAR_TEXTURE";
		sMacro[iCurrent].Definition = "1";
		++iCurrent;
		}
	if (pcMesh->piAmbientTexture)
		{
		sMacro[iCurrent].Name = "AV_AMBIENT_TEXTURE";
		sMacro[iCurrent].Definition = "1";
		++iCurrent;
		}
	if (pcMesh->piEmissiveTexture)
		{
		sMacro[iCurrent].Name = "AV_EMISSIVE_TEXTURE";
		sMacro[iCurrent].Definition = "1";
		++iCurrent;
		}
	if (pcMesh->piNormalTexture)
		{
		sMacro[iCurrent].Name = "AV_NORMAL_TEXTURE";
		sMacro[iCurrent].Definition = "1";
		++iCurrent;
		}
	if (pcMesh->piOpacityTexture)
		{
		sMacro[iCurrent].Name = "AV_OPACITY_TEXTURE";
		sMacro[iCurrent].Definition = "1";
		++iCurrent;

		if (pcMesh->piOpacityTexture == pcMesh->piDiffuseTexture)
			{
			sMacro[iCurrent].Name = "AV_OPACITY_TEXTURE_REGISTER_MASK";
			sMacro[iCurrent].Definition = "a";
			++iCurrent;
			}
		else
			{
			sMacro[iCurrent].Name = "AV_OPACITY_TEXTURE_REGISTER_MASK";
			sMacro[iCurrent].Definition = "r";
			++iCurrent;
			}
		}


	if (pcMesh->eShadingMode  != aiShadingMode_Gouraud  && !g_sOptions.bNoSpecular)
		{
		sMacro[iCurrent].Name = "AV_SPECULAR_COMPONENT";
		sMacro[iCurrent].Definition = "1";
		++iCurrent;
		}
	if (1.0f != pcMesh->fOpacity)
		{
		sMacro[iCurrent].Name = "AV_OPACITY";
		sMacro[iCurrent].Definition = "1";
		++iCurrent;
		}



	// If a cubemap is active, we'll need to lookup it for calculating
	// a physically correct reflection
	if (CBackgroundPainter::TEXTURE_CUBE == CBackgroundPainter::Instance().GetMode())
		{
		sMacro[iCurrent].Name = "AV_SKYBOX_LOOKUP";
		sMacro[iCurrent].Definition = "1";
		++iCurrent;
		}
	sMacro[iCurrent].Name = NULL;
	sMacro[iCurrent].Definition = NULL;

	// compile the shader
	if(FAILED( D3DXCreateEffect(g_piDevice,
		g_szMaterialShader.c_str(),(UINT)g_szMaterialShader.length(),
		(const D3DXMACRO*)sMacro,NULL,0,NULL,&pcMesh->piEffect,&piBuffer)))
		{
		// failed to compile the shader
		if( piBuffer) 
			{
			MessageBox(g_hDlg,(LPCSTR)piBuffer->GetBufferPointer(),"HLSL",MB_OK);
			piBuffer->Release();
			}
		// use the default material instead
		if (g_piDefaultEffect)
			{
			pcMesh->piEffect = g_piDefaultEffect;
			g_piDefaultEffect->AddRef();
			}

		// get the name of the material and use it in the log message
		if(AI_SUCCESS == aiGetMaterialString(pcMat,AI_MATKEY_NAME,&szPath) &&
			'\0' != szPath.data[0])
			{
			std::string sz = "[ERROR] Unable to load material: ";
			sz.append(szPath.data);
			CLogDisplay::Instance().AddEntry(sz);
			}
		else
			{
			CLogDisplay::Instance().AddEntry("Unable to load material: UNNAMED");
			}
		return 0;
		}
	if( piBuffer) piBuffer->Release();


	// now commit all constants to the shader
	//
	// This is not necessary for shared shader. Shader constants for
	// shared shaders are automatically recommited before the shader
	// is being used for a particular mesh

	if (1.0f != pcMesh->fOpacity)
		pcMesh->piEffect->SetFloat("TRANSPARENCY",pcMesh->fOpacity);
	if (pcMesh->eShadingMode  != aiShadingMode_Gouraud && !g_sOptions.bNoSpecular)
		pcMesh->piEffect->SetFloat("SPECULARITY",pcMesh->fShininess);

	pcMesh->piEffect->SetVector("DIFFUSE_COLOR",&pcMesh->vDiffuseColor);
	pcMesh->piEffect->SetVector("SPECULAR_COLOR",&pcMesh->vSpecularColor);
	pcMesh->piEffect->SetVector("AMBIENT_COLOR",&pcMesh->vAmbientColor);
	pcMesh->piEffect->SetVector("EMISSIVE_COLOR",&pcMesh->vEmissiveColor);

	if (pcMesh->piDiffuseTexture)
		pcMesh->piEffect->SetTexture("DIFFUSE_TEXTURE",pcMesh->piDiffuseTexture);
	if (pcMesh->piOpacityTexture)
		pcMesh->piEffect->SetTexture("OPACITY_TEXTURE",pcMesh->piOpacityTexture);
	if (pcMesh->piSpecularTexture)
		pcMesh->piEffect->SetTexture("SPECULAR_TEXTURE",pcMesh->piSpecularTexture);
	if (pcMesh->piAmbientTexture)
		pcMesh->piEffect->SetTexture("AMBIENT_TEXTURE",pcMesh->piAmbientTexture);
	if (pcMesh->piEmissiveTexture)
		pcMesh->piEffect->SetTexture("EMISSIVE_TEXTURE",pcMesh->piEmissiveTexture);
	if (pcMesh->piNormalTexture)
		pcMesh->piEffect->SetTexture("NORMAL_TEXTURE",pcMesh->piNormalTexture);

	if (CBackgroundPainter::TEXTURE_CUBE == CBackgroundPainter::Instance().GetMode())
		{
		pcMesh->piEffect->SetTexture("lw_tex_envmap",CBackgroundPainter::Instance().GetTexture());
		}
	return 1;
	}
};