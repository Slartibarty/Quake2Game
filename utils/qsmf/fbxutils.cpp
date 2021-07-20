
#include "../../core/core.h"

#include "fbxsdk.h"

FbxScene *FBX_ImportScene( FbxManager *pManager, const char *filename )
{
	FbxImporter *pImporter = FbxImporter::Create( pManager, "" );

	Com_Printf( "Importing scene: %s\n", filename );

	if ( !pImporter->Initialize( filename, -1, pManager->GetIOSettings() ) )
	{
		Com_Print( "FbxImporter::Initialize failed\n" );
		Com_Printf( "Error returned: %s\n", pImporter->GetStatus().GetErrorString() );
		return nullptr;
	}

	FbxScene *pScene = FbxScene::Create( pManager, "" );
	if ( pScene )
	{
		pImporter->Import( pScene );
	}
	pImporter->Destroy();

	return pScene;
}

FbxVector2 FBX_GetUV( FbxMesh *pMesh, int polyIter, int vertIter )
{
	const int controlPointIndex = pMesh->GetPolygonVertex( polyIter, vertIter );
	const int elementUVCount = pMesh->GetElementUVCount();

	for ( int uvIter = 0; uvIter < elementUVCount; ++uvIter )
	{
		FbxGeometryElementUV *pUV = pMesh->GetElementUV( uvIter );

		int id;

		switch ( pUV->GetMappingMode() )
		{
		case FbxGeometryElement::eByPolygonVertex:
		{
			int textureUVIndex = pMesh->GetTextureUVIndex( polyIter, vertIter );
			switch ( pUV->GetReferenceMode() )
			{
			case FbxGeometryElement::eDirect:
			case FbxGeometryElement::eIndexToDirect:
				return pUV->GetDirectArray().GetAt( textureUVIndex );
				break;

				// other reference modes not shown here!
			default:
				break;
			}
		}
		break;

		case FbxGeometryElement::eByControlPoint:
			switch ( pUV->GetReferenceMode() )
			{
			case FbxGeometryElement::eDirect:
				return pUV->GetDirectArray().GetAt( controlPointIndex );
				break;

			case FbxGeometryElement::eIndexToDirect:
				id = pUV->GetIndexArray().GetAt( controlPointIndex );
				return pUV->GetDirectArray().GetAt( id );
				break;

				// other reference modes not shown here!
			default:
				break;
			}
			break;

		default:
			break;
		}
	}

	return FbxVector2( 0.0, 0.0 );
}

FbxVector4 FBX_GetNormal( FbxMesh *pMesh, int vertexID )
{
	const int elementNormalCount = pMesh->GetElementNormalCount();

	for ( int normIter = 0; normIter < elementNormalCount; ++normIter )
	{
		FbxGeometryElementNormal *pNormal = pMesh->GetElementNormal( normIter );

		if ( pNormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex )
		{
			int id;

			switch ( pNormal->GetReferenceMode() )
			{
			case FbxGeometryElement::eDirect:
				return pNormal->GetDirectArray().GetAt( vertexID );
				break;

			case FbxGeometryElement::eIndexToDirect:
				id = pNormal->GetIndexArray().GetAt( vertexID );
				return pNormal->GetDirectArray().GetAt( id );
				break;

				// other reference modes not shown here!
			default:
				break;
			}
		}
	}

	return FbxVector4( 0.0, 0.0, 0.0, 0.0 );
}

FbxVector4 FBX_GetTangent( FbxMesh *pMesh, int vertexID )
{
	const int elementTangentCount = pMesh->GetElementTangentCount();

	for ( int tanIter = 0; tanIter < elementTangentCount; ++tanIter )
	{
		FbxGeometryElementTangent *pTangent = pMesh->GetElementTangent( tanIter );

		if ( pTangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex )
		{
			int id;

			switch ( pTangent->GetReferenceMode() )
			{
			case FbxGeometryElement::eDirect:
				return pTangent->GetDirectArray().GetAt( vertexID );
				break;

			case FbxGeometryElement::eIndexToDirect:
				id = pTangent->GetIndexArray().GetAt( vertexID );
				return pTangent->GetDirectArray().GetAt( id );
				break;

				// other reference modes not shown here!
			default:
				break;
			}
		}
	}

	return FbxVector4( 0.0, 0.0, 0.0, 0.0 );
}

FbxVector4 FBX_GetVertexWeights( FbxMesh *pMesh, int vertexID )
{
	const int elementTangentCount = pMesh->GetElementTangentCount();

	for ( int tanIter = 0; tanIter < elementTangentCount; ++tanIter )
	{
		FbxGeometryElementTangent *pTangent = pMesh->GetElementTangent( tanIter );

		if ( pTangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex )
		{
			int id;

			switch ( pTangent->GetReferenceMode() )
			{
			case FbxGeometryElement::eDirect:
				return pTangent->GetDirectArray().GetAt( vertexID );
				break;

			case FbxGeometryElement::eIndexToDirect:
				id = pTangent->GetIndexArray().GetAt( vertexID );
				return pTangent->GetDirectArray().GetAt( id );
				break;

				// other reference modes not shown here!
			default:
				break;
			}
		}
	}

	return FbxVector4( 0.0, 0.0, 0.0, 0.0 );
}
