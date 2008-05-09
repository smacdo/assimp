/** @file Implementation of the post processing step to join identical vertices
 * for all imported meshes
 */

#include <vector>
#include <assert.h>
#include "JoinVerticesProcess.h"
#include "SpatialSort.h"
#include "../include/aiPostProcess.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
JoinVerticesProcess::JoinVerticesProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
JoinVerticesProcess::~JoinVerticesProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool JoinVerticesProcess::IsActive( unsigned int pFlags) const
{
	return (pFlags & aiProcess_JoinIdenticalVertices) != 0;
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void JoinVerticesProcess::Execute( aiScene* pScene)
{
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
		ProcessMesh( pScene->mMeshes[a]);
}

// ------------------------------------------------------------------------------------------------
// Unites identical vertices in the given mesh
void JoinVerticesProcess::ProcessMesh( aiMesh* pMesh)
{
	// helper structure to hold all the data a single vertex can possibly have
	typedef struct Vertex vertex;
	
	struct Vertex
	{
		aiVector3D mPosition;
		aiVector3D mNormal;
		aiVector3D mTangent, mBitangent;
		aiColor4D mColors[AI_MAX_NUMBER_OF_COLOR_SETS];
		aiVector3D mTexCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];
	};
	std::vector<Vertex> uniqueVertices;
	uniqueVertices.reserve( pMesh->mNumVertices);

	// For each vertex the index of the vertex it was replaced by. 
	std::vector<unsigned int> replaceIndex( pMesh->mNumVertices, 0xffffffff);
	// for each vertex whether it was replaced by an existing unique vertex (true) or a new vertex was created for it (false)
	std::vector<bool> isVertexUnique( pMesh->mNumVertices, false);

	// calculate the position bounds so we have a reliable epsilon to check position differences against 
	aiVector3D minVec( 1e10f, 1e10f, 1e10f), maxVec( -1e10f, -1e10f, -1e10f);
	for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
	{
		minVec.x = std::min( minVec.x, pMesh->mVertices[a].x);
		minVec.y = std::min( minVec.y, pMesh->mVertices[a].y);
		minVec.z = std::min( minVec.z, pMesh->mVertices[a].z);
		maxVec.x = std::max( maxVec.x, pMesh->mVertices[a].x);
		maxVec.y = std::max( maxVec.y, pMesh->mVertices[a].y);
		maxVec.z = std::max( maxVec.z, pMesh->mVertices[a].z);
	}

	// squared because we check against squared length of the vector difference	
	const float epsilon = 1e-5f; 
	const float posEpsilon = (maxVec - minVec).Length() * epsilon;
	const float squareEpsilon = epsilon * epsilon;

	// a little helper to find locally close vertices faster
	SpatialSort vertexFinder( pMesh->mVertices, pMesh->mNumVertices, sizeof( aiVector3D));
	std::vector<unsigned int> verticesFound;

	// now check each vertex if it brings something new to the table
	for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
	{
		// collect the vertex data
		Vertex v;
		v.mPosition = pMesh->mVertices[a];
		v.mNormal = (pMesh->mNormals != NULL) ? pMesh->mNormals[a] : aiVector3D( 0, 0, 0);
		v.mTangent = (pMesh->mTangents != NULL) ? pMesh->mTangents[a] : aiVector3D( 0, 0, 0);
		v.mBitangent = (pMesh->mBitangents != NULL) ? pMesh->mBitangents[a] : aiVector3D( 0, 0, 0);
		for( unsigned int b = 0; b < AI_MAX_NUMBER_OF_COLOR_SETS; b++)
			v.mColors[b] = (pMesh->mColors[b] != NULL) ? pMesh->mColors[b][a] : aiColor4D( 0, 0, 0, 0);
		for( unsigned int b = 0; b < AI_MAX_NUMBER_OF_TEXTURECOORDS; b++)
			v.mTexCoords[b] = (pMesh->mTextureCoords[b] != NULL) ? pMesh->mTextureCoords[b][a] : aiVector3D( 0, 0, 0);

		// collect all vertices that are close enough to the given position
		vertexFinder.FindPositions( v.mPosition, posEpsilon, verticesFound);

		unsigned int matchIndex = 0xffffffff;
		// check all unique vertices close to the position if this vertex is already present among them
		for( unsigned int b = 0; b < verticesFound.size(); b++)
		{
			unsigned int vidx = verticesFound[b];
			unsigned int uidx = replaceIndex[ vidx];
			if( uidx == 0xffffffff || !isVertexUnique[ vidx])
				continue;

			const Vertex& uv = uniqueVertices[ uidx];
			// Position mismatch is impossible - the vertex finder already discarded all non-matching positions

			// We just test the other attributes even if they're not present in the mesh.
			// In this case they're initialized to 0 so the comparision succeeds. 
			// By this method the non-present attributes are effectively ignored in the comparision.
			
			if( (uv.mNormal - v.mNormal).SquareLength() > squareEpsilon)
				continue;
			if( (uv.mTangent - v.mTangent).SquareLength() > squareEpsilon)
				continue;
			if( (uv.mBitangent - v.mBitangent).SquareLength() > squareEpsilon)
				continue;
			// manually unrolled because continue wouldn't work as desired in an inner loop
			assert( AI_MAX_NUMBER_OF_COLOR_SETS == 4);
			if( GetColorDifference( uv.mColors[0], v.mColors[0]) > squareEpsilon)
				continue;
			if( GetColorDifference( uv.mColors[1], v.mColors[1]) > squareEpsilon)
				continue;
			if( GetColorDifference( uv.mColors[2], v.mColors[2]) > squareEpsilon)
				continue;
			if( GetColorDifference( uv.mColors[3], v.mColors[3]) > squareEpsilon)
				continue;
			// texture coord matching manually unrolled as well
			assert( AI_MAX_NUMBER_OF_TEXTURECOORDS == 4);
			if( (uv.mTexCoords[0] - v.mTexCoords[0]).SquareLength() > squareEpsilon)
				continue;
			if( (uv.mTexCoords[1] - v.mTexCoords[1]).SquareLength() > squareEpsilon)
				continue;
			if( (uv.mTexCoords[2] - v.mTexCoords[2]).SquareLength() > squareEpsilon)
				continue;
			if( (uv.mTexCoords[3] - v.mTexCoords[3]).SquareLength() > squareEpsilon)
				continue;

			// we're still here -> this vertex perfectly matches our given vertex
			matchIndex = uidx;
			break;
		}

		// found a replacement vertex among the uniques?
		if( matchIndex != 0xffffffff)
		{
			// store where to found the matching unique vertex
			replaceIndex[a] = matchIndex;
			isVertexUnique[a] = false;
		} else
		{
			// no unique vertex matches it upto now -> so add it
			replaceIndex[a] = uniqueVertices.size();
			uniqueVertices.push_back( v);
			isVertexUnique[a] = true;
		}
	}

	// replace vertex data with the unique data sets
	pMesh->mNumVertices = uniqueVertices.size();
	// Position
	delete [] pMesh->mVertices;
	pMesh->mVertices = new aiVector3D[pMesh->mNumVertices];
	for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
		pMesh->mVertices[a] = uniqueVertices[a].mPosition;
	// Normals, if present
	if( pMesh->mNormals)
	{
		delete [] pMesh->mNormals;
		pMesh->mNormals = new aiVector3D[pMesh->mNumVertices];
		for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
			pMesh->mNormals[a] = uniqueVertices[a].mNormal;
	}
	// Tangents, if present
	if( pMesh->mTangents)
	{
		delete [] pMesh->mTangents;
		pMesh->mTangents = new aiVector3D[pMesh->mNumVertices];
		for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
			pMesh->mTangents[a] = uniqueVertices[a].mTangent;
	}
	// Bitangents as well
	if( pMesh->mBitangents)
	{
		delete [] pMesh->mBitangents;
		pMesh->mBitangents = new aiVector3D[pMesh->mNumVertices];
		for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
			pMesh->mBitangents[a] = uniqueVertices[a].mBitangent;
	}
	// Vertex colors
	for( unsigned int a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; a++)
	{
		if( !pMesh->mColors[a])
			continue;

		delete [] pMesh->mColors[a];
		pMesh->mColors[a] = new aiColor4D[pMesh->mNumVertices];
		for( unsigned int b = 0; b < pMesh->mNumVertices; b++)
			pMesh->mColors[a][b] = uniqueVertices[b].mColors[a];
	}
	// Texture coords
	for( unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++)
	{
		if( !pMesh->mTextureCoords[a])
			continue;

		delete [] pMesh->mTextureCoords[a];
		pMesh->mTextureCoords[a] = new aiVector3D[pMesh->mNumVertices];
		for( unsigned int b = 0; b < pMesh->mNumVertices; b++)
			pMesh->mTextureCoords[a][b] = uniqueVertices[b].mTexCoords[a];
	}

	// adjust the indices in all faces
	for( unsigned int a = 0; a < pMesh->mNumFaces; a++)
	{
		aiFace& face = pMesh->mFaces[a];
		for( unsigned int b = 0; b < face.mNumIndices; b++)
		{
			const size_t index = face.mIndices[b];
			face.mIndices[b] = replaceIndex[face.mIndices[b]];
		}
	}

	// adjust bone vertex weights.
	for( unsigned int a = 0; a < pMesh->mNumBones; a++)
	{
		aiBone* bone = pMesh->mBones[a];
		std::vector<aiVertexWeight> newWeights;
		newWeights.reserve( bone->mNumWeights);

		for( unsigned int b = 0; b < bone->mNumWeights; b++)
		{
			const aiVertexWeight& ow = bone->mWeights[b];
			// if the vertex is a unique one, translate it
			if( isVertexUnique[ow.mVertexId])
			{
				aiVertexWeight nw;
				nw.mVertexId = replaceIndex[ow.mVertexId];
				nw.mWeight = ow.mWeight;
				newWeights.push_back( nw);
			}
		}

		// there should be some. At least I think there should be some
		assert( newWeights.size() > 0);

		// kill the old and replace them with the translated weights
		delete [] bone->mWeights;
		bone->mNumWeights = newWeights.size();
		bone->mWeights = new aiVertexWeight[bone->mNumWeights];
		memcpy( bone->mWeights, &newWeights[0], bone->mNumWeights * sizeof( aiVertexWeight));
	}
}