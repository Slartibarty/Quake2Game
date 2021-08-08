
#pragma once

FbxScene *FBX_ImportScene( FbxManager *pManager, const char *filename );

FbxVector2 FBX_GetUV( FbxMesh *pMesh, int polyIter, int vertIter );
FbxVector4 FBX_GetNormal( FbxMesh *pMesh, int vertexID );
FbxVector4 FBX_GetTangent( FbxMesh *pMesh, int vertexID );

FbxVector4 FBX_SkinMesh( FbxMesh *pMesh );
