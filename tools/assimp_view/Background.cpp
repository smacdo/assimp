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


// From: U3D build 1256 (src\kernel\graphic\scenegraph\SkyBox.cpp)
// ------------------------------------------------------------------------------
/**	\brief Vertex structure for the skybox
 */
// ------------------------------------------------------------------------------
struct SkyBoxVertex
	{
    float x,y,z;
    float u,v,w;
	};


// ------------------------------------------------------------------------------
/**	\brief Vertices for the skybox 
 */
// ------------------------------------------------------------------------------
SkyBoxVertex g_cubeVertices_indexed[] =
	{
	{ -1.0f, 1.0f, -1.0f,	-1.0f,1.0f,-1.0f },		// 0
	{ 1.0f, 1.0f,  -1.0f,	1.0f,1.0f,-1.0f },		// 1
	{ -1.0f, -1.0f, -1.0f,	-1.0f,-1.0f,-1.0f },	// 2
	{ 1.0f,-1.0f,-1.0f,		 1.0f,-1.0f,-1.0f },	// 3
	{-1.0f, 1.0f, 1.0f,		-1.0f,1.0f,1.0f },		// 4
	{-1.0f,-1.0f, 1.0f,		-1.0f,-1.0f,1.0f },		// 5
	{ 1.0f, 1.0f, 1.0f,		1.0f,1.0f,1.0f },		// 6
	{ 1.0f,-1.0f, 1.0f,		1.0f,-1.0f,1.0f }		// 7
	};


// ------------------------------------------------------------------------------
/**	\brief Indices for the skybox
 */
// ------------------------------------------------------------------------------
unsigned short g_cubeIndices[] =
	{
	0, 1, 2, 3, 2, 1,4, 5, 6,
	7, 6, 5, 4, 6,  0, 1, 6, 0, 
	5, 2, 7,3, 2, 7, 1, 6, 3,
	7, 3, 6, 0, 2, 4, 5, 4, 2,  
	};

CBackgroundPainter CBackgroundPainter::s_cInstance;

//-------------------------------------------------------------------------------
void CBackgroundPainter::SetColor (D3DCOLOR p_clrNew)
	{
	if (TEXTURE_CUBE == this->eMode)this->RemoveSBDeps();

	this->clrColor = p_clrNew;
	this->eMode = SIMPLE_COLOR;

	if (this->pcTexture)
		{
		this->pcTexture->Release();
		this->pcTexture = NULL;
		}
	}
//-------------------------------------------------------------------------------
void CBackgroundPainter::RemoveSBDeps()
	{
	MODE e = this->eMode;
	this->eMode = SIMPLE_COLOR;
	if (g_pcAsset && g_pcAsset->pcScene)
		{
		for (unsigned int i = 0; i < g_pcAsset->pcScene->mNumMeshes;++i)
			{
			if (aiShadingMode_Gouraud != g_pcAsset->apcMeshes[i]->eShadingMode)
				{
				DeleteMaterial(g_pcAsset->apcMeshes[i]);
				CreateMaterial(g_pcAsset->apcMeshes[i],g_pcAsset->pcScene->mMeshes[i]);
				}
			}
		}
	this->eMode = e;
	}
//-------------------------------------------------------------------------------
void CBackgroundPainter::ResetSB()
	{
	this->mMatrix = aiMatrix4x4();
	}
//-------------------------------------------------------------------------------
void CBackgroundPainter::SetCubeMapBG (const char* p_szPath)
	{
	bool bHad = false;
	if (this->pcTexture)
		{
		this->pcTexture->Release();
		this->pcTexture = NULL;
		if(TEXTURE_CUBE ==this->eMode)bHad = true;
		}

	this->eMode = TEXTURE_CUBE;

	this->szPath = std::string( p_szPath );

	// ARRRGHH... ugly. TODO: Rewrite this!
	aiString sz;
	sz.Set(this->szPath);
	FindValidPath(&sz);
	this->szPath = std::string( sz.data );

	// now recreate all native resources
	this->RecreateNativeResource();

	if (SIMPLE_COLOR != this->eMode)
		{
		// this influences all material with specular components
		if (!bHad)
			{
			if (g_pcAsset && g_pcAsset->pcScene)
				{
				for (unsigned int i = 0; i < g_pcAsset->pcScene->mNumMeshes;++i)
					{
					if (aiShadingMode_Phong == g_pcAsset->apcMeshes[i]->eShadingMode)
						{
						DeleteMaterial(g_pcAsset->apcMeshes[i]);
						CreateMaterial(g_pcAsset->apcMeshes[i],g_pcAsset->pcScene->mMeshes[i]);
						}
					}
				}
			}
		else
			{
			if (g_pcAsset && g_pcAsset->pcScene)
				{
				for (unsigned int i = 0; i < g_pcAsset->pcScene->mNumMeshes;++i)
					{
					if (aiShadingMode_Phong == g_pcAsset->apcMeshes[i]->eShadingMode)
						{
						g_pcAsset->apcMeshes[i]->piEffect->SetTexture(
							"lw_tex_envmap",CBackgroundPainter::Instance().GetTexture());
						}
					}
				}
			}
		}
	}
//-------------------------------------------------------------------------------
void CBackgroundPainter::RotateSB(const aiMatrix4x4* pm)
	{
	this->mMatrix = this->mMatrix * (*pm);
	}
//-------------------------------------------------------------------------------
void CBackgroundPainter::SetTextureBG (const char* p_szPath)
	{
	if (TEXTURE_CUBE == this->eMode)this->RemoveSBDeps();

	if (this->pcTexture)
		{
		this->pcTexture->Release();
		this->pcTexture = NULL;
		}
	
	this->eMode = TEXTURE_2D;
	this->szPath = std::string( p_szPath );

	// ARRRGHH... ugly. TODO: Rewrite this!
	aiString sz;
	sz.Set(this->szPath);
	FindValidPath(&sz);
	this->szPath = std::string( sz.data );

	// now recreate all native resources
	this->RecreateNativeResource();
	}
//-------------------------------------------------------------------------------
void CBackgroundPainter::OnPreRender()
	{
	if (SIMPLE_COLOR != this->eMode)
		{
		// clear the z-buffer only (in wireframe mode we must also clear
		// the color buffer )
		if (g_sOptions.eDrawMode == RenderOptions::WIREFRAME)
			{
			g_piDevice->Clear(0,NULL,D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET,
				D3DCOLOR_ARGB(0xff,100,100,100),1.0f,0);
			}
		else
			{
			g_piDevice->Clear(0,NULL,D3DCLEAR_ZBUFFER,0,1.0f,0);
			}

		if (TEXTURE_2D == this->eMode)
			{
			RECT sRect;
			GetWindowRect(GetDlgItem(g_hDlg,IDC_RT),&sRect);
			sRect.right -= sRect.left;
			sRect.bottom -= sRect.top;

			struct SVertex
				{
				float x,y,z,w,u,v;
				};

			UINT dw;
			this->piSkyBoxEffect->Begin(&dw,0);
			this->piSkyBoxEffect->BeginPass(0);

			SVertex as[4];
			as[1].x = 0.0f;
			as[1].y = 0.0f;
			as[1].z = 0.2f;
			as[1].w = 1.0f;
			as[1].u = 0.0f;
			as[1].v = 0.0f;

			as[3].x = (float)sRect.right;
			as[3].y = 0.0f;
			as[3].z = 0.2f;
			as[3].w = 1.0f;
			as[3].u = 1.0f;
			as[3].v = 0.0f;

			as[0].x = 0.0f;
			as[0].y = (float)sRect.bottom;
			as[0].z = 0.2f;
			as[0].w = 1.0f;
			as[0].u = 0.0f;
			as[0].v = 1.0f;

			as[2].x = (float)sRect.right;
			as[2].y = (float)sRect.bottom;
			as[2].z = 0.2f;
			as[2].w = 1.0f;
			as[2].u = 1.0f;
			as[2].v = 1.0f;

			as[0].x -= 0.5f;as[1].x -= 0.5f;as[2].x -= 0.5f;as[3].x -= 0.5f;
			as[0].y -= 0.5f;as[1].y -= 0.5f;as[2].y -= 0.5f;as[3].y -= 0.5f;

			DWORD dw2;g_piDevice->GetFVF(&dw2);
			g_piDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

			g_piDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,
				&as,sizeof(SVertex));

			this->piSkyBoxEffect->EndPass();
			this->piSkyBoxEffect->End();

			g_piDevice->SetFVF(dw2);
			}
		return;
		}
	// clear both the render target and the z-buffer
	g_piDevice->Clear(0,NULL,D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		this->clrColor,1.0f,0);
	}
//-------------------------------------------------------------------------------
void CBackgroundPainter::OnPostRender()
	{
	if (TEXTURE_CUBE == this->eMode)
		{
		aiMatrix4x4 pcProj;
		GetProjectionMatrix(pcProj);

		aiMatrix4x4 pcCam;
		aiVector3D vPos = GetCameraMatrix(pcCam);

		aiMatrix4x4 aiMe;
	    aiMe[3][0] = vPos.x;
		aiMe[3][1] = vPos.y;
		aiMe[3][2] = vPos.z;
		aiMe = this->mMatrix * aiMe;

		pcProj = (aiMe * pcCam) * pcProj;

		this->piSkyBoxEffect->SetMatrix("WorldViewProjection",
			(const D3DXMATRIX*)&pcProj);

		UINT dwPasses;
		this->piSkyBoxEffect->Begin(&dwPasses,0);
		this->piSkyBoxEffect->BeginPass(0);

		DWORD dw2;
		g_piDevice->GetFVF(&dw2);
		g_piDevice->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0));

		g_piDevice->DrawIndexedPrimitiveUP(
			D3DPT_TRIANGLELIST,0,8,12,g_cubeIndices,D3DFMT_INDEX16,
			g_cubeVertices_indexed,sizeof(SkyBoxVertex));

		g_piDevice->SetFVF(dw2);

		this->piSkyBoxEffect->EndPass();
		this->piSkyBoxEffect->End();
		}
	}
//-------------------------------------------------------------------------------
void CBackgroundPainter::ReleaseNativeResource()
	{
	if (this->piSkyBoxEffect)
		{
		this->piSkyBoxEffect->Release();
		this->piSkyBoxEffect = NULL;
		}
	if (this->pcTexture)
		{
		this->pcTexture->Release();
		this->pcTexture = NULL;
		}
	}
//-------------------------------------------------------------------------------
void CBackgroundPainter::RecreateNativeResource()
	{
	if (SIMPLE_COLOR == this->eMode)return;
	if (TEXTURE_CUBE == this->eMode)
		{

		// many skyboxes are 16bit FP format which isn't supported
		// with bilinear filtering on older cards
		D3DFORMAT eFmt = D3DFMT_UNKNOWN;
		if(FAILED(g_piD3D->CheckDeviceFormat(0,D3DDEVTYPE_HAL,
			D3DFMT_X8R8G8B8,D3DUSAGE_QUERY_FILTER,D3DRTYPE_CUBETEXTURE,D3DFMT_A16B16G16R16F)))
		{
			eFmt = D3DFMT_A8R8G8B8;
		}

		if (FAILED(D3DXCreateCubeTextureFromFileEx(
			g_piDevice,
			this->szPath.c_str(),
			D3DX_DEFAULT,
			0,
			0,
			eFmt,
			D3DPOOL_MANAGED,
			D3DX_DEFAULT,
			D3DX_DEFAULT,
			0,
			NULL,
			NULL,
			(IDirect3DCubeTexture9**)&this->pcTexture)))
			{
			const char* szEnd = strrchr(this->szPath.c_str(),'\\');
			if (!szEnd)szEnd = strrchr(this->szPath.c_str(),'/');
			if (!szEnd)szEnd = this->szPath.c_str()-1;

			char szTemp[1024];
			sprintf(szTemp,"[ERROR] Unable to load background cubemap %s",szEnd+1);
			
			CLogDisplay::Instance().AddEntry(szTemp,
				D3DCOLOR_ARGB(0xFF,0xFF,0,0));

			this->eMode = SIMPLE_COLOR;
			return;
			}
		else CLogDisplay::Instance().AddEntry("[OK] The skybox has been imported successfully",
			D3DCOLOR_ARGB(0xFF,0,0xFF,0));
		}
	else
		{
		if (FAILED(D3DXCreateTextureFromFileEx(
			g_piDevice,
			this->szPath.c_str(),
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
			(IDirect3DTexture9**)&this->pcTexture)))
			{
			const char* szEnd = strrchr(this->szPath.c_str(),'\\');
			if (!szEnd)szEnd = strrchr(this->szPath.c_str(),'/');
			if (!szEnd)szEnd = this->szPath.c_str()-1;

			char szTemp[1024];
			sprintf(szTemp,"[ERROR] Unable to load background texture %s",szEnd+1);
			
			CLogDisplay::Instance().AddEntry(szTemp,
				D3DCOLOR_ARGB(0xFF,0xFF,0,0));

			this->eMode = SIMPLE_COLOR;
			return;
			}
		else CLogDisplay::Instance().AddEntry("[OK] The background texture has been imported successfully",
			D3DCOLOR_ARGB(0xFF,0,0xFF,0));
		}
	if (!piSkyBoxEffect)
		{
		if(FAILED( D3DXCreateEffect(
			g_piDevice,
			g_szSkyboxShader.c_str(),
			(UINT)g_szSkyboxShader.length(),
			NULL,
			NULL,
			D3DXSHADER_USE_LEGACY_D3DX9_31_DLL,
			NULL,
			&this->piSkyBoxEffect,NULL)))
			{
			CLogDisplay::Instance().AddEntry("[ERROR] Unable to compile skybox shader",
				D3DCOLOR_ARGB(0xFF,0xFF,0,0));
			this->eMode = SIMPLE_COLOR;
			return ;
			}
		}
	// commit the correct textures to the shader
	if (TEXTURE_CUBE == this->eMode)
		{
		this->piSkyBoxEffect->SetTexture("lw_tex_envmap",this->pcTexture);
		this->piSkyBoxEffect->SetTechnique("RenderSkyBox");
		}
	else if (TEXTURE_2D == this->eMode)
		{
		this->piSkyBoxEffect->SetTexture("TEXTURE_2D",this->pcTexture);
		this->piSkyBoxEffect->SetTechnique("RenderImage2D");
		}
	}
};