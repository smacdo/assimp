/*
Hailstorm Model Exporter
Copyright (c) 2012 Scott MacDonald
======================================================================

Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_EXPORT
#ifndef ASSIMP_BUILD_NO_HAILSTORM_EXPORTER
#include "HailstormExporter.h"

using namespace Assimp;

namespace Assimp
{

// ------------------------------------------------------------------------------------------------
// Worker function for exporting a scene to Hailstorm.
// Prototyped and registered in Exporter.cpp
void ExportSceneHailstorm( const char* pFile,
                           IOSystem* pIOSystem,
                           const aiScene* pScene )
{
	// invoke the exporter 
	HailstormExporter iDoTheExportThing( pScene);

	// we're still here - export successfully completed. Write result to the given IOSYstem
	boost::scoped_ptr<IOStream> outfile (pIOSystem->Open(pFile,"wt"));

	// XXX maybe use a small wrapper around IOStream that behaves like std::stringstream in order to avoid the extra copy.
	outfile->Write( iDoTheExportThing.mOutput.str().c_str(), static_cast<size_t>(iDoTheExportThing.mOutput.tellp()),1);
}

} // end of namespace Assimp


// ------------------------------------------------------------------------------------------------
// Constructor for a specific scene to export
HailstormExporter::HailstormExporter( const aiScene* pScene)
{
	// make sure that all formatting happens using the standard, C locale and not the user's current locale
	mOutput.imbue( std::locale("C") );

	mScene = pScene;

	// set up strings
	endstr = "\n"; 

	// start writing
	WriteFile();
}

// ------------------------------------------------------------------------------------------------
// Starts writing the contents
void HailstormExporter::WriteFile()
{
	// write the DTD
	mOutput << "<?xml version=\"1.0\"?>" << endstr;
    mOutput << "<model version=\"1\">" << endstr;
	PushTag();

	WriteHeader( "", "", "" );
    WriteMaterials();
	WriteSceneGeometry();
    WriteSceneGraph();

	PopTag();
	mOutput << "</model>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Writes the asset header
void HailstormExporter::WriteHeader( const std::string& author,
                                     const std::string& url,
                                     const std::string& license )
{
	mOutput << startstr << "<details>" << endstr;
    PushTag();

    // Used to keep track of contributions to the game. Can be read in
    // but usually manually edited 
	mOutput << startstr << "<author>"  << author  << "</author>"  << endstr
            << startstr << "<url>"     << url     << "</url>"     << endstr
            << startstr << "<license>" << license << "</license>" << endstr;

	PopTag();
	mOutput << startstr << "</details>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Reads a single surface entry from the given material keys
void HailstormExporter::ReadMaterialSurface( Surface& surface,
                                             const aiMaterial* pSrcMat,
                                             aiTextureType texture,
                                             const char* pKey,
                                             size_t type,
                                             size_t index )
{
    assert( pSrcMat != NULL );

    if ( pSrcMat->GetTextureCount( texture ) > 0 )
    {
        aiString texfile;
        unsigned int uvChannel = 0;
        pSrcMat->GetTexture( texture, 0, &texfile, NULL, &uvChannel );

        surface.texture = texfile.C_Str();
        surface.channel = uvChannel;
        surface.enabled = true;
    }
    else if ( pKey != NULL )
    {
        if ( pSrcMat->Get( pKey, type, index, surface.color ) == aiReturn_SUCCESS )
        {
            surface.enabled = true;
        }
        else
        {
            surface.enabled = false;
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Writes a color-or-texture entry into an effect definition
void HailstormExporter::WriteShadingParam( const Surface& surface,
                                           const std::string& paramName,
                                           const std::string& matName )
{
    mOutput << startstr << "<" << paramName << " ";

    if ( surface.texture.empty() )
    {
        mOutput << Write( surface.color );
    }
    else
    {
        mOutput << "image=\"" << surface.texture << "\" ";
    }

    mOutput << "/>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Writes the material setup
void HailstormExporter::WriteMaterials()
{
    materials.resize( mScene->mNumMaterials );

    /// collect all materials from the scene
    size_t numTextures = 0;

    //
    // Go through all of the materials in the scene, and see if they have
    // any associated source images that need to be referenced
    //
    for( size_t a = 0; a < mScene->mNumMaterials; ++a )
    {
        // Get a pointer to the active material that we wish to write out
        const aiMaterial* pMat = mScene->mMaterials[a];
        assert( pMat != NULL );

        // Obtain the name of the material. If the material does not have
        // a name assigned, then this is probably an error
        //  (Especially since we can't connect the mesh material to us)
        aiString name;

        if( pMat->Get( AI_MATKEY_NAME, name ) != aiReturn_SUCCESS )
        {
            assert( false && "Materials must have names" );
        }

        materials[a].name = std::string( name.C_Str() );

        // Read in all of the possible surface information stored in
        // the material
        ReadMaterialSurface( materials[a].ambient,
                             pMat,
                             aiTextureType_AMBIENT,
                             AI_MATKEY_COLOR_AMBIENT );

        ReadMaterialSurface( materials[a].diffuse,
                             pMat,
                             aiTextureType_DIFFUSE,
                             AI_MATKEY_COLOR_DIFFUSE );

        ReadMaterialSurface( materials[a].specular,
                             pMat,
                             aiTextureType_SPECULAR,
                             AI_MATKEY_COLOR_SPECULAR );

        ReadMaterialSurface( materials[a].emissive,
                             pMat,
                             aiTextureType_EMISSIVE,
                             AI_MATKEY_COLOR_EMISSIVE );

        ReadMaterialSurface( materials[a].normal,
                             pMat,
                             aiTextureType_NORMALS,
                             NULL,
                             0,
                             0 );

        // Also get dat shiny key
        if ( pMat->Get( AI_MATKEY_SHININESS, materials[a].shininess) == aiReturn_SUCCESS )
        {
            materials[a].hasShininess = true;
        }

        pMat->Get( AI_MATKEY_SHADING_MODEL, materials[a].shadingMode );
    }

    if ( !materials.empty() )
    {
        mOutput << startstr << "<materials>" << endstr;
        PushTag();

        for ( std::vector<Material>::const_iterator it  = materials.begin();
                                                    it != materials.end();
                                                  ++it )
        {
            const Material& mat = *it;

            // Write the name of the material out first
            mOutput << startstr
                    << "<material "
                    << "name=\"" << mat.name << "\" "
                    << ">"
                    << endstr;
            PushTag();

            // Write out the lighting model
            mOutput << startstr
                    << "<shading "
                    << "model=\""  << GetShadingName( mat.shadingMode ) << "\" "
                    << "/>"
                    << endstr;
            PushTag();

            if ( mat.ambient.enabled )
            {
                WriteShadingParam( mat.ambient,  "ambient",  mat.name  );
            }

            if ( mat.diffuse.enabled )
            {
                WriteShadingParam( mat.diffuse,  "diffuse",  mat.name  );
            }

            if ( mat.emissive.enabled == false &&
                 mat.emissive.color.r == 0     && mat.emissive.color.g == 0 &&
                 mat.emissive.color.b == 0 )
            {
                WriteShadingParam( mat.emissive, "emissive", mat.name );
            }

            if ( mat.specular.enabled )
            {
                WriteShadingParam( mat.specular, "specular", mat.name );
            }

            if ( !mat.normal.texture.empty() )
            {
                WriteShadingParam( mat.normal, "normal", mat.name );
            }
            
            // TODO don't forget shinines

            PopTag();
            mOutput << startstr << "</shading>" << endstr;

            PopTag();
            mOutput << startstr << "</material>" << endstr;
        }

        PopTag();
        mOutput << startstr << "</materials>" << endstr;
    }
}

// ------------------------------------------------------------------------------------------------
// Writes the geometry library
void HailstormExporter::WriteSceneGeometry()
{
    mOutput << startstr << "<meshes>" << endstr;
	PushTag();

	for ( size_t a = 0; a < mScene->mNumMeshes; ++a )
    {
		WriteGeometry( a );
    }

	PopTag();
	mOutput << startstr << "</meshes>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Writes the given mesh
void HailstormExporter::WriteGeometry( size_t meshIndex )
{
    // Get a pointer to the mesh we wish to write out
	const aiMesh* pMesh = mScene->mMeshes[ meshIndex ];
    assert( pMesh != NULL && "Cannot have a null mesh" );

    // Generate a name for the mesh. Either use the one already provided,
    // or create a default name
	std::string idstr = GetMeshId( meshIndex );

    // Refuse to export empty meshes
    if( pMesh->mNumFaces == 0 || pMesh->mNumVertices == 0 )
    {
        return;
    }

    // Get the name of the material referenced by this mesh, if any
    aiString matname;

    if ( mScene->HasMaterials() )
    {
        // Ensure the material index is valid
        assert( pMesh->mMaterialIndex < mScene->mNumMaterials );

        // Look up the material, and get it's name. Double check that the
        // material actually has a name
        aiMaterial * pMat = mScene->mMaterials[ pMesh->mMaterialIndex ];
        assert( pMat != NULL );

        if ( AI_SUCCESS != pMat->Get( AI_MATKEY_NAME, matname ) )
        {
            assert( false && "Failed to locate material name" );
        }
    }

    //
    // Write out the mesh element tag, which is the container for this
    // mesh's vertex buffer and face array. WE'll need to write the format
    // and setup options out as well
    // 
    mOutput << startstr
            << "<mesh "
            << "name=\""    << idstr                        << "\" ";

    if ( matname.length > 0 )
    {
        mOutput << "mat=\"" << matname.C_Str()              << "\" ";
    }

    mOutput << "uv=\""      << pMesh->GetNumUVChannels()    << "\" "
            << "color=\""   << pMesh->GetNumColorChannels() << "\" "
            << "/>" << endstr;
    PushTag();

	// Open the vertices array
    mOutput << startstr
            << "<va count=\"" << pMesh->mNumVertices << "\" />"
            << endstr;
	PushTag();

    // Write out every vertex contained within
    for ( size_t i = 0; i < pMesh->mNumVertices; ++i )
    {
        mOutput << startstr;
        mOutput << "<v ";

        // Write position
        mOutput << Write( pMesh->mVertices[i] );

        // Write normals, if we have them
        if ( pMesh->HasNormals() )
        {
            mOutput << Write( pMesh->mNormals[i], "n" );
        }

        // Write tangent info
        if ( pMesh->HasTangentsAndBitangents() )
        {
            mOutput << Write( pMesh->mBitangents[i], "b" )
                    << Write( pMesh->mTangents[i], "t" );
        }

        // Write texture coordinates
        for ( size_t j = 0; j < pMesh->GetNumUVChannels(); ++j )
        {
            assert( pMesh->HasTextureCoords( j ) );
            mOutput << WriteUV( pMesh->mTextureCoords[j][i], j );
            
        }

        // Write vertex color
        if ( pMesh->HasVertexColors( 0 ) )
        {
            mOutput << Write( pMesh->mColors[0][i] );
        }

        // Wrap it up
        mOutput << "/>" << endstr;
    }

	PopTag();
	mOutput << startstr << "</va>" << endstr;

	// Let's start exporting faces. Start with the face container element
	mOutput << startstr
            << "<fa "
            << "count=\"" << pMesh->mNumFaces << "\" "
            << "/>" << endstr;
	PushTag();
	
    // Go through the mesh's list of faces, and write out each one as a
    // separate face element
    for ( size_t j = 0; j < pMesh->mNumFaces; ++j )
    {
        const aiFace& face = pMesh->mFaces[j];

        // Validate that there are only three points per face
        assert( face.mNumIndices == 3 );

        // Now write the faces out
        mOutput << startstr
                << "<f "
                << "a=\"" << face.mIndices[0] << "\" "
                << "b=\"" << face.mIndices[1] << "\" "
                << "c=\"" << face.mIndices[2] << "\" />"
                << endstr;
    }

    // Close it up
    PopTag();
	mOutput << startstr << "</fa>" << endstr;

	PopTag();
	mOutput << startstr << "</mesh>" << endstr;
}

void HailstormExporter::WriteSceneGraph()
{
    mOutput << startstr << "<scenegraph>" << endstr;
    PushTag();

    WriteNode( mScene->mRootNode );

    PopTag();
    mOutput << startstr << "</scenegraph>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Recursively writes the given node
void HailstormExporter::WriteNode( const aiNode* pNode )
{
    assert( pNode != NULL && "Scene node cannot be null" );

	mOutput << startstr
            << "<node "
            << "name=\""  << pNode->mName.C_Str() << "\""
            << ">"
            << endstr;
	PushTag();

	// write transformation - we can directly put the matrix there
    //  *** ROW MAJOR ***
	const aiMatrix4x4& mat = pNode->mTransformation;

    if (! mat.IsIdentity() )
    {
        // Write out the transformation aspect of the model
        aiVector3D scale, translation;
        aiQuaternion rotation;
        mat.Decompose( scale, rotation, translation );

        mOutput << startstr
                << "<scale " << Write( scale ) << "/>"
                << endstr;

        mOutput << startstr
                << "<translation " << Write( translation ) << "/>"
                << endstr;

        mOutput << startstr
                << "<rotation " << Write( rotation ) << "/>"
                << endstr;
    }

	// instance every geometry
	for( size_t a = 0; a < pNode->mNumMeshes; ++a )
	{
		const aiMesh* mesh = mScene->mMeshes[pNode->mMeshes[a]];
        assert( mesh->mNumFaces > 0 && mesh->mNumVertices > 0 );

		mOutput << startstr
                << "<mesh "
                << "name=\""  << GetMeshId( pNode->mMeshes[a] )       << "\" "
                << "mat=\"" << materials[mesh->mMaterialIndex].name << "\" "
                << "/>" << endstr;
	}

	// recurse into subnodes
	for( size_t a = 0; a < pNode->mNumChildren; ++a )
    {
		WriteNode( pNode->mChildren[a] );
    }

	PopTag();
	mOutput << startstr << "</node>" << endstr;
}

/**
 * Takes a shading mode enumeration value, and returns it in string form
 */
std::string HailstormExporter::GetShadingName( aiShadingMode shadingMode ) const
{
    std::string shadingName;

    switch ( shadingMode )
    {
        case aiShadingMode_Flat:
            shadingName = "flat";
            break;
        case aiShadingMode_Gouraud:
            shadingName = "gouraud";
            break;
        case aiShadingMode_Phong:
            shadingName = "phong";
            break;
        case aiShadingMode_Blinn:
            shadingName = "blinn";
            break;
        case aiShadingMode_Toon:
            shadingName = "toon";
            break;
        case aiShadingMode_OrenNayar:
        case aiShadingMode_Minnaert:
        case aiShadingMode_CookTorrance:
        case aiShadingMode_Fresnel:
            assert( false && "Shading mode not supported yet" );
            break;
        case aiShadingMode_NoShading:
            shadingName = "none";
            break;
        default:
            shadingName = "phong";
            break;
    }

    return shadingName;
}

/**
 * Returns a Vec3 suitable for export
 */
std::string HailstormExporter::Write( const aiVector3D& vec,
                                      const std::string& prefix ) const
{
    std::ostringstream ss;

    ss << prefix << "x=\"" << vec[0] << "\" "
       << prefix << "y=\"" << vec[1] << "\" "
       << prefix << "z=\"" << vec[2] << "\" ";

    return ss.str();
}

/**
 * Returns a Matrix4x4 suitable for export
 */
std::string HailstormExporter::Write( const aiQuaternion& quat,
                                      const std::string& prefix ) const
{
    std::ostringstream ss;

    ss << prefix << "w=\"" << quat.w << "\" "
       << prefix << "x=\"" << quat.x << "\" "
       << prefix << "y=\"" << quat.y << "\" "
       << prefix << "z=\"" << quat.z << "\" ";

    return ss.str();
}

/**
 * Returns a Matrix4x4 suitable for export
 */
std::string HailstormExporter::Write( const aiMatrix4x4& mat,
                                      const std::string& prefix ) const
{
    std::ostringstream ss;

    ss << prefix << "m00=\"" << mat.a1 << "\" "
       << prefix << "m01=\"" << mat.a2 << "\" "
       << prefix << "m02=\"" << mat.a3 << "\" "
       << prefix << "m03=\"" << mat.a4 << "\" "
       << prefix << "m10=\"" << mat.b1 << "\" "
       << prefix << "m11=\"" << mat.b2 << "\" "
       << prefix << "m12=\"" << mat.b3 << "\" "
       << prefix << "m13=\"" << mat.b4 << "\" "
       << prefix << "m20=\"" << mat.c1 << "\" "
       << prefix << "m21=\"" << mat.c2 << "\" "
       << prefix << "m22=\"" << mat.c3 << "\" "
       << prefix << "m23=\"" << mat.c4 << "\" "
       << prefix << "m30=\"" << mat.d1 << "\" "
       << prefix << "m31=\"" << mat.d2 << "\" "
       << prefix << "m32=\"" << mat.d3 << "\" "
       << prefix << "m33=\"" << mat.d4 << "\" ";

    return ss.str();
}

/**
 * Returns a Color4 suitable for export
 */
std::string HailstormExporter::Write( const aiColor4D& c,
                                      const std::string& prefix ) const
{
    std::ostringstream ss;

    ss << prefix << "r=\"" << c.r << "\" "
       << prefix << "g=\"" << c.g << "\" "
       << prefix << "b=\"" << c.b << "\" ";

    if ( c.a < 1.0f )
    {
        ss << prefix << "a=\"" << c.a << "\" ";
    }

    return ss.str();
}

/**
 * Returns a texture coordinate suitable for export
 */
std::string HailstormExporter::WriteUV ( const aiVector3D& v,
                                         unsigned int index ) const
{
    std::ostringstream ss;

    ss << "u" << boost::lexical_cast<std::string>( index )
              << "=\"" << v[0] << "\" "
       << "v" << boost::lexical_cast<std::string>( index )
              << "=\"" << v[1] << "\" ";
    return ss.str();
}

#endif
#endif

