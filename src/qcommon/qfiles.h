/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#ifndef __QFILES_H__
#define __QFILES_H__

//
// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories
//

// surface geometry should not exceed these limits
#define SHADER_MAX_VERTEXES 1025 // Arnout: 1024+1 (1 buffer for RB_EndSurface overflow check) // JPW NERVE was 4000, 1000 in q3ta
#define SHADER_MAX_INDEXES  ( 6 * SHADER_MAX_VERTEXES )


// the maximum size of game reletive pathnames
#define MAX_QPATH       64

/*
========================================================================

QVM files

========================================================================
*/

#define VM_MAGIC    0x12721444
typedef struct {
	int vmMagic;

	int instructionCount;

	int codeOffset;
	int codeLength;

	int dataOffset;
	int dataLength;
	int litLength;              // ( dataLength - litLength ) should be byteswapped on load
	int bssLength;              // zero filled memory appended to datalength
} vmHeader_t;

/*
========================================================================

.MD3 triangle model file format

========================================================================
*/

#define MD3_IDENT			(('3'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD3_VERSION			15

// limits
#define MD3_MAX_LODS		4
#define	MD3_MAX_TRIANGLES	8192	// per surface
#define MD3_MAX_VERTS		4096	// per surface
#define MD3_MAX_SHADERS		256		// per surface
#define MD3_MAX_FRAMES		1024	// per model
#define	MD3_MAX_SURFACES	32		// per model
#define MD3_MAX_TAGS		16		// per frame

// vertex scales
#define	MD3_XYZ_SCALE		(1.0/64)

typedef struct md3Frame_s {
	vec3_t		bounds[2];
	vec3_t		localOrigin;
	float		radius;
	char		name[16];
} md3Frame_t;

typedef struct md3Tag_s {
	char		name[MAX_QPATH];	// tag name
	vec3_t		origin;
	vec3_t		axis[3];
} md3Tag_t;

/*
** md3Surface_t
**
** CHUNK			SIZE
** header			sizeof( md3Surface_t )
** shaders			sizeof( md3Shader_t ) * numShaders
** triangles[0]		sizeof( md3Triangle_t ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numFrames
*/
typedef struct {
	int32_t ident;				//

	char	name[MAX_QPATH];	// polyset name

	int32_t flags;
	int32_t numFrames;			// all surfaces in a model should have the same

	int32_t numShaders;			// all surfaces in a model should have the same
	int32_t numVerts;

	int32_t numTriangles;
	uint32_t ofsTriangles;

	uint32_t ofsShaders;			// offset from start of md3Surface_t
	uint32_t ofsSt;				// texture coords are common for all frames
	uint32_t ofsXyzNormals;		// numVerts * numFrames

	uint32_t ofsEnd;				// next surface follows
} md3Surface_t;

typedef struct {
	char			name[MAX_QPATH];
	int				shaderIndex;	// for in-game use
} md3Shader_t;

typedef struct {
	int			indexes[3];
} md3Triangle_t;

typedef struct {
	float		st[2];
} md3St_t;

typedef struct {
	short		xyz[3];
	short		normal;
} md3XyzNormal_t;

typedef struct {
	int32_t		ident;
	int32_t		version;

	char		name[MAX_QPATH];	// model name

	uint32_t	flags;

	int32_t		numFrames;
	int32_t		numTags;
	int32_t		numSurfaces;

	int32_t		numSkins;

	uint32_t	ofsFrames;			// offset for first frame
	uint32_t	ofsTags;			// numFrames * numTags
	uint32_t	ofsSurfaces;		// first surface, others follow

	uint32_t	ofsEnd;				// end of file
} md3Header_t;

/*
========================================================================

.tag tag file format

========================================================================
*/

#define TAG_IDENT           ( ( '3' << 24 ) + ( 'G' << 16 ) + ( 'A' << 8 ) + 'T' )
#define TAG_VERSION         1

typedef struct {
	int32_t ident;
	int32_t version;

	int32_t numTags;

	uint32_t ofsEnd;
} tagHeader_t;

typedef struct {
	char filename[MAX_QPATH];
	int start;
	int count;
} tagHeaderExt_t;

// Ridah, mesh compression
/*
==============================================================================

MDC file format

==============================================================================
*/

#define MDC_IDENT           ( ( 'C' << 24 ) + ( 'P' << 16 ) + ( 'D' << 8 ) + 'I' )
#define MDC_VERSION         2

// version history:
// 1 - original
// 2 - changed tag structure so it only lists the names once

typedef struct {
	uint32_t ofsVec;                    // offset direction from the last base frame
//	uint16_t	ofsVec;
} mdcXyzCompressed_t;

typedef struct {
	char name[MAX_QPATH];           // tag name
} mdcTagName_t;

#define MDC_TAG_ANGLE_SCALE ( 360.0 / 32700.0 )

typedef struct {
	short xyz[3];
	short angles[3];
} mdcTag_t;

/*
** mdcSurface_t
**
** CHUNK			SIZE
** header			sizeof( md3Surface_t )
** shaders			sizeof( md3Shader_t ) * numShaders
** triangles[0]		sizeof( md3Triangle_t ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numBaseFrames
** XyzCompressed	sizeof( mdcXyzCompressed ) * numVerts * numCompFrames
** frameBaseFrames	sizeof( short ) * numFrames
** frameCompFrames	sizeof( short ) * numFrames (-1 if frame is a baseFrame)
*/
typedef struct {
	int32_t ident;                  //

	char name[MAX_QPATH];       // polyset name

	int32_t flags;
	int32_t numCompFrames;          // all surfaces in a model should have the same
	int32_t numBaseFrames;          // ditto

	int32_t numShaders;             // all surfaces in a model should have the same
	int32_t numVerts;

	int32_t numTriangles;
	uint32_t ofsTriangles;

	uint32_t ofsShaders;             // offset from start of md3Surface_t
	uint32_t ofsSt;                  // texture coords are common for all frames
	uint32_t ofsXyzNormals;          // numVerts * numBaseFrames
	uint32_t ofsXyzCompressed;       // numVerts * numCompFrames

	uint32_t ofsFrameBaseFrames;     // numFrames
	uint32_t ofsFrameCompFrames;     // numFrames

	uint32_t ofsEnd;                 // next surface follows
} mdcSurface_t;

typedef struct {
	int32_t ident;
	int32_t version;

	char name[MAX_QPATH];           // model name

	uint32_t flags;

	int32_t numFrames;
	int32_t numTags;
	int32_t numSurfaces;

	int32_t numSkins;

	uint32_t ofsFrames;                  // offset for first frame, stores the bounds and localOrigin
	uint32_t ofsTagNames;                // numTags
	uint32_t ofsTags;                    // numFrames * numTags
	uint32_t ofsSurfaces;                // first surface, others follow

	uint32_t ofsEnd;                     // end of file
} mdcHeader_t;
// done.


/*
==============================================================================

MDS file format (Wolfenstein Skeletal Format)

==============================================================================
*/

#define MDS_IDENT           ( ( 'W' << 24 ) + ( 'S' << 16 ) + ( 'D' << 8 ) + 'M' )
#define MDS_VERSION         4
#define MDS_MAX_VERTS       6000
#define MDS_MAX_TRIANGLES   8192
#define MDS_MAX_BONES       128
#define MDS_MAX_SURFACES    32
#define MDS_MAX_TAGS        128

#define MDS_TRANSLATION_SCALE   ( 1.0 / 64 )

typedef struct {
	int32_t boneIndex;              // these are indexes into the boneReferences,
	float boneWeight;           // not the global per-frame bone list
	vec3_t offset;
} mdsWeight_t;

typedef struct {
	vec3_t normal;
	vec2_t texCoords;
	int32_t numWeights;
	int32_t fixedParent;            // stay equi-distant from this parent
	float fixedDist;
	mdsWeight_t weights[1];     // variable sized
} mdsVertex_t;

typedef struct {
	int32_t indexes[3];
} mdsTriangle_t;

typedef struct {
	int32_t ident;

	char name[MAX_QPATH];           // polyset name
	char shader[MAX_QPATH];
	int32_t shaderIndex;                // for in-game use

	int32_t minLod;

	int32_t ofsHeader;                  // this will be a negative number

	int32_t numVerts;
	uint32_t ofsVerts;

	int32_t numTriangles;
	uint32_t ofsTriangles;

	uint32_t ofsCollapseMap;           // numVerts * int

	// Bone references are a set of ints representing all the bones
	// present in any vertex weights for this surface.  This is
	// needed because a model may have surfaces that need to be
	// drawn at different sort times, and we don't want to have
	// to re-interpolate all the bones for each surface.
	int32_t numBoneReferences;
	uint32_t ofsBoneReferences;

	uint32_t ofsEnd;                     // next surface follows
} mdsSurface_t;

typedef struct {
	//float		angles[3];
	//float		ofsAngles[2];
	short angles[4];            // to be converted to axis at run-time (this is also better for lerping)
	short ofsAngles[2];         // PITCH/YAW, head in this direction from parent to go to the offset position
} mdsBoneFrameCompressed_t;

// NOTE: this only used at run-time
typedef struct {
	float matrix[3][3];             // 3x3 rotation
	vec3_t translation;             // translation vector
} mdsBoneFrame_t;

typedef struct {
	vec3_t bounds[2];               // bounds of all surfaces of all LOD's for this frame
	vec3_t localOrigin;             // midpoint of bounds, used for sphere cull
	float radius;                   // dist from localOrigin to corner
	vec3_t parentOffset;            // one bone is an ascendant of all other bones, it starts the hierachy at this position
	mdsBoneFrameCompressed_t bones[1];              // [numBones]
} mdsFrame_t;

typedef struct {
	int32_t numSurfaces;
	uint32_t ofsSurfaces;                // first surface, others follow
	uint32_t ofsEnd;                     // next lod follows
} mdsLOD_t;

typedef struct {
	char name[MAX_QPATH];           // name of tag
	float torsoWeight;
	int32_t boneIndex;                  // our index in the bones
} mdsTag_t;

#define BONEFLAG_TAG        1       // this bone is actually a tag

typedef struct {
	char name[MAX_QPATH];           // name of bone
	int32_t parent;                     // not sure if this is required, no harm throwing it in
	float torsoWeight;              // scale torso rotation about torsoParent by this
	float parentDist;
	int32_t flags;
} mdsBoneInfo_t;

typedef struct {
	int32_t ident;
	int32_t version;

	char name[MAX_QPATH];           // model name

	float lodScale;
	float lodBias;

	// frames and bones are shared by all levels of detail
	int32_t numFrames;
	int32_t numBones;
	uint32_t ofsFrames;                  // md4Frame_t[numFrames]
	uint32_t ofsBones;                   // mdsBoneInfo_t[numBones]
	int32_t torsoParent;                // index of bone that is the parent of the torso

	int32_t numSurfaces;
	uint32_t ofsSurfaces;

	// tag data
	int32_t numTags;
	uint32_t ofsTags;                    // mdsTag_t[numTags]

	uint32_t ofsEnd;                     // end of file
} mdsHeader_t;

/*
==============================================================================

MDM file format (Wolfenstein Skeletal Mesh)

version history:
	2 - initial version
	3 - removed all frame data, this format is pure mesh and bone references now

==============================================================================
*/

#define MDM_IDENT           ( ( 'W' << 24 ) + ( 'M' << 16 ) + ( 'D' << 8 ) + 'M' )
#define MDM_VERSION         3
#define MDM_MAX_VERTS       6000
#define MDM_MAX_TRIANGLES   8192
#define MDM_MAX_SURFACES    32
#define MDM_MAX_TAGS        128

#define MDM_TRANSLATION_SCALE   ( 1.0 / 64 )

typedef struct {
	int32_t boneIndex;              // these are indexes into the boneReferences,
	float boneWeight;           // not the global per-frame bone list
	vec3_t offset;
} mdmWeight_t;

typedef struct {
	vec3_t normal;
	vec2_t texCoords;
	int32_t numWeights;
	mdmWeight_t weights[1];     // variable sized
} mdmVertex_t;

typedef struct {
	int32_t indexes[3];
} mdmTriangle_t;

typedef struct {
	int32_t ident;

	char name[MAX_QPATH];           // polyset name
	char shader[MAX_QPATH];
	int32_t shaderIndex;                // for in-game use

	int32_t minLod;

	int32_t ofsHeader;                  // this will be a negative number

	int32_t numVerts;
	uint32_t ofsVerts;

	int32_t numTriangles;
	uint32_t ofsTriangles;

	uint32_t ofsCollapseMap;           // numVerts * int

	// Bone references are a set of ints representing all the bones
	// present in any vertex weights for this surface.  This is
	// needed because a model may have surfaces that need to be
	// drawn at different sort times, and we don't want to have
	// to re-interpolate all the bones for each surface.
	int32_t numBoneReferences;
	uint32_t ofsBoneReferences;

	uint32_t ofsEnd;                     // next surface follows
} mdmSurface_t;

/*typedef struct {
	vec3_t		bounds[2];			// bounds of all surfaces of all LOD's for this frame
	vec3_t		localOrigin;		// midpoint of bounds, used for sphere cull
	float		radius;				// dist from localOrigin to corner
	vec3_t		parentOffset;		// one bone is an ascendant of all other bones, it starts the hierachy at this position
} mdmFrame_t;*/

typedef struct {
	int32_t numSurfaces;
	uint32_t ofsSurfaces;                // first surface, others follow
	uint32_t ofsEnd;                     // next lod follows
} mdmLOD_t;

/*typedef struct {
	char		name[MAX_QPATH];	// name of tag
	float		torsoWeight;
	int			boneIndex;			// our index in the bones

	int			numBoneReferences;
	int			ofsBoneReferences;

	int			ofsEnd;				// next tag follows
} mdmTag_t;*/

// Tags always only have one parent bone
typedef struct {
	char name[MAX_QPATH];           // name of tag
	vec3_t axis[3];

	int32_t boneIndex;
	vec3_t offset;

	int32_t numBoneReferences;
	uint32_t ofsBoneReferences;

	uint32_t ofsEnd;                     // next tag follows
} mdmTag_t;

typedef struct {
	int32_t ident;
	int32_t version;

	char name[MAX_QPATH];           // model name
/*	char		bonesfile[MAX_QPATH];	// bone file

#ifdef UTILS
	int			skel;
#else
	// dummy in file, set on load to link to MDX
	qhandle_t	skel;
#endif // UTILS
*/
	float lodScale;
	float lodBias;

	// frames and bones are shared by all levels of detail
/*	int			numFrames;
	int			ofsFrames;			// mdmFrame_t[numFrames]
*/
	int32_t numSurfaces;
	uint32_t ofsSurfaces;

	// tag data
	int32_t numTags;
	uint32_t ofsTags;

	uint32_t ofsEnd;                     // end of file
} mdmHeader_t;

/*
==============================================================================

MDX file format (Wolfenstein Skeletal Data)

version history:
	1 - initial version
	2 - moved parentOffset from the mesh to the skeletal data file

==============================================================================
*/

#define MDX_IDENT           ( ( 'W' << 24 ) + ( 'X' << 16 ) + ( 'D' << 8 ) + 'M' )
#define MDX_VERSION         2
#define MDX_MAX_BONES       128

typedef struct {
	vec3_t bounds[2];               // bounds of this frame
	vec3_t localOrigin;             // midpoint of bounds, used for sphere cull
	float radius;                   // dist from localOrigin to corner
	vec3_t parentOffset;            // one bone is an ascendant of all other bones, it starts the hierachy at this position
} mdxFrame_t;

typedef struct {
	//float		angles[3];
	//float		ofsAngles[2];
	short angles[4];                // to be converted to axis at run-time (this is also better for lerping)
	short ofsAngles[2];             // PITCH/YAW, head in this direction from parent to go to the offset position
} mdxBoneFrameCompressed_t;

// NOTE: this only used at run-time
// FIXME: do we really need this?
typedef struct {
	float matrix[3][3];             // 3x3 rotation
	vec3_t translation;             // translation vector
} mdxBoneFrame_t;

typedef struct {
	char name[MAX_QPATH];           // name of bone
	int32_t parent;                     // not sure if this is required, no harm throwing it in
	float torsoWeight;              // scale torso rotation about torsoParent by this
	float parentDist;
	int32_t flags;
} mdxBoneInfo_t;

typedef struct {
	int32_t ident;
	int32_t version;

	char name[MAX_QPATH];           // model name

	// bones are shared by all levels of detail
	int32_t numFrames;
	int32_t numBones;
	uint32_t ofsFrames;                  // (mdxFrame_t + mdxBoneFrameCompressed_t[numBones]) * numframes
	uint32_t ofsBones;                   // mdxBoneInfo_t[numBones]
	int32_t torsoParent;                // index of bone that is the parent of the torso

	uint32_t ofsEnd;                     // end of file
} mdxHeader_t;

/*
==============================================================================

  .BSP file format

==============================================================================
*/


#define BSP_IDENT   ( ( 'P' << 24 ) + ( 'S' << 16 ) + ( 'B' << 8 ) + 'I' )
// little-endian "IBSP"

#define BSP_VERSION         47


// there shouldn't be any problem with increasing these values at the
// expense of more memory allocation in the utilities
//#define	MAX_MAP_MODELS		0x400
#define MAX_MAP_MODELS      0x800
#define MAX_MAP_BRUSHES     16384
#define MAX_MAP_ENTITIES    4096
#define MAX_MAP_ENTSTRING   0x40000
#define MAX_MAP_SHADERS     0x400

#define MAX_MAP_AREAS       0x100   // MAX_MAP_AREA_BYTES in q_shared must match!
#define MAX_MAP_FOGS        0x100
#define MAX_MAP_PLANES      0x40000
#define MAX_MAP_NODES       0x20000
#define MAX_MAP_BRUSHSIDES  0x100000
#define MAX_MAP_LEAFS       0x20000
#define MAX_MAP_LEAFFACES   0x20000
#define MAX_MAP_LEAFBRUSHES 0x40000
#define MAX_MAP_PORTALS     0x20000
#define MAX_MAP_LIGHTING    0x800000
#define MAX_MAP_LIGHTGRID   0x800000
#define MAX_MAP_VISIBILITY  0x200000

#define MAX_MAP_DRAW_SURFS  0x20000
#define MAX_MAP_DRAW_VERTS  0x80000
#define MAX_MAP_DRAW_INDEXES    0x80000


// key / value pair sizes in the entities lump
#define MAX_KEY             32
#define MAX_VALUE           1024

// the editor uses these predefined yaw angles to orient entities up or down
#define ANGLE_UP            -1
#define ANGLE_DOWN          -2

#define LIGHTMAP_WIDTH      128
#define LIGHTMAP_HEIGHT     128

#define MAX_WORLD_COORD     ( 128 * 1024 )
#define MIN_WORLD_COORD     ( -128 * 1024 )
#define WORLD_SIZE          ( MAX_WORLD_COORD - MIN_WORLD_COORD )

//=============================================================================


typedef struct {
	int32_t fileofs;
	int32_t filelen;
} lump_t;

#define LUMP_ENTITIES       0
#define LUMP_SHADERS        1
#define LUMP_PLANES         2
#define LUMP_NODES          3
#define LUMP_LEAFS          4
#define LUMP_LEAFSURFACES   5
#define LUMP_LEAFBRUSHES    6
#define LUMP_MODELS         7
#define LUMP_BRUSHES        8
#define LUMP_BRUSHSIDES     9
#define LUMP_DRAWVERTS      10
#define LUMP_DRAWINDEXES    11
#define LUMP_FOGS           12
#define LUMP_SURFACES       13
#define LUMP_LIGHTMAPS      14
#define LUMP_LIGHTGRID      15
#define LUMP_VISIBILITY     16
#define HEADER_LUMPS        17

typedef struct {
	int ident;
	int version;

	lump_t lumps[HEADER_LUMPS];
} dheader_t;

typedef struct {
	float mins[3], maxs[3];
	int firstSurface, numSurfaces;
	int firstBrush, numBrushes;
} dmodel_t;

typedef struct {
	char shader[MAX_QPATH];
	int surfaceFlags;
	int contentFlags;
} dshader_t;

// planes x^1 is always the opposite of plane x

typedef struct {
	float normal[3];
	float dist;
} dplane_t;

typedef struct {
	int planeNum;
	int children[2];            // negative numbers are -(leafs+1), not nodes
	int mins[3];                // for frustom culling
	int maxs[3];
} dnode_t;

typedef struct {
	int cluster;                    // -1 = opaque cluster (do I still store these?)
	int area;

	int mins[3];                    // for frustum culling
	int maxs[3];

	int firstLeafSurface;
	int numLeafSurfaces;

	int firstLeafBrush;
	int numLeafBrushes;
} dleaf_t;

typedef struct {
	int planeNum;                   // positive plane side faces out of the leaf
	int shaderNum;
} dbrushside_t;

typedef struct {
	int firstSide;
	int numSides;
	int shaderNum;              // the shader that determines the contents flags
} dbrush_t;

typedef struct {
	char shader[MAX_QPATH];
	int brushNum;
	int visibleSide;            // the brush side that ray tests need to clip against (-1 == none)
} dfog_t;

typedef struct {
	vec3_t xyz;
	float st[2];
	float lightmap[2];
	vec3_t normal;
#ifdef USE_VULKAN
	color4ub_t	color;
#else
	byte color[4];
#endif
} drawVert_t;

typedef enum {
	MST_BAD,
	MST_PLANAR,
	MST_PATCH,
	MST_TRIANGLE_SOUP,
	MST_FLARE,
	MST_FOLIAGE
} mapSurfaceType_t;

typedef struct {
	int shaderNum;
	int fogNum;
	int surfaceType;

	int firstVert;
	int numVerts;                   // ydnar: num verts + foliage origins (for cleaner lighting code in q3map)

	int firstIndex;
	int numIndexes;

	int lightmapNum;
	int lightmapX, lightmapY;
	int lightmapWidth, lightmapHeight;

	vec3_t lightmapOrigin;
	vec3_t lightmapVecs[3];         // for patches, [0] and [1] are lodbounds

	int patchWidth;                 // ydnar: num foliage instances
	int patchHeight;                // ydnar: num foliage mesh verts
} dsurface_t;

//----(SA) added so I didn't change the dsurface_t struct (and thereby the bsp format) for something that doesn't need to be stored in the bsp
typedef struct {
	char        *lighttarg;
} drsurfaceInternal_t;
//----(SA) end

#endif
