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

// tr_mesh.c: triangle model functions
//
// !!! NOTE: Any changes made here must be duplicated in tr_cmesh.c for MDC support

#include "tr_local.h"

static float ProjectRadius( float r, vec3_t location ) {
	float pr;
	float dist;
	float c;
	vec3_t p;
	float projected[4];

	c = DotProduct( tr.viewParms.orientation.axis[0], tr.viewParms.orientation.origin );
	dist = DotProduct( tr.viewParms.orientation.axis[0], location ) - c;

	if ( dist <= 0 ) {
		return 0;
	}

	p[0] = 0;
	p[1] = fabs( r );
	p[2] = -dist;

#if 0
	projected[0] = p[0] * tr.viewParms.projectionMatrix[0] +
				   p[1] * tr.viewParms.projectionMatrix[4] +
				   p[2] * tr.viewParms.projectionMatrix[8] +
				   tr.viewParms.projectionMatrix[12];
#endif
	projected[1] = p[0] * tr.viewParms.projectionMatrix[1] +
				   p[1] * tr.viewParms.projectionMatrix[5] +
				   p[2] * tr.viewParms.projectionMatrix[9] +
				   tr.viewParms.projectionMatrix[13];
#if 0
	projected[2] = p[0] * tr.viewParms.projectionMatrix[2] +
				   p[1] * tr.viewParms.projectionMatrix[6] +
				   p[2] * tr.viewParms.projectionMatrix[10] +
				   tr.viewParms.projectionMatrix[14];
#endif
	projected[3] = p[0] * tr.viewParms.projectionMatrix[3] +
				   p[1] * tr.viewParms.projectionMatrix[7] +
				   p[2] * tr.viewParms.projectionMatrix[11] +
				   tr.viewParms.projectionMatrix[15];

	pr = projected[1] / projected[3];

	if ( pr > 1.0f ) {
		pr = 1.0f;
	}

	return pr;
}

/*
=============
R_CullModel
=============
*/
static int R_CullModel( md3Header_t *header, trRefEntity_t *ent, vec3_t bounds[] ) {
	//vec3_t bounds[2];
	md3Frame_t  *oldFrame, *newFrame;
	int i;

	// compute frame pointers
	newFrame = ( md3Frame_t * )( ( byte * ) header + header->ofsFrames ) + ent->e.frame;
	oldFrame = ( md3Frame_t * )( ( byte * ) header + header->ofsFrames ) + ent->e.oldframe;

	// cull bounding sphere ONLY if this is not an upscaled entity
	if ( !ent->e.nonNormalizedAxes ) {
		if ( ent->e.frame == ent->e.oldframe ) {
			switch ( R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius ) )
			{
			case CULL_OUT:
				tr.pc.c_sphere_cull_md3_out++;
				return CULL_OUT;

			case CULL_IN:
				tr.pc.c_sphere_cull_md3_in++;
				return CULL_IN;

			case CULL_CLIP:
				tr.pc.c_sphere_cull_md3_clip++;
				break;
			}
		} else
		{
			int sphereCull, sphereCullB;

			sphereCull  = R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius );
			if ( newFrame == oldFrame ) {
				sphereCullB = sphereCull;
			} else {
				sphereCullB = R_CullLocalPointAndRadius( oldFrame->localOrigin, oldFrame->radius );
			}

			if ( sphereCull == sphereCullB ) {
				if ( sphereCull == CULL_OUT ) {
					tr.pc.c_sphere_cull_md3_out++;
					return CULL_OUT;
				} else if ( sphereCull == CULL_IN )   {
					tr.pc.c_sphere_cull_md3_in++;
					return CULL_IN;
				} else
				{
					tr.pc.c_sphere_cull_md3_clip++;
				}
			}
		}
	}

	// calculate a bounding box in the current coordinate system
	for ( i = 0 ; i < 3 ; i++ ) {
		bounds[0][i] = oldFrame->bounds[0][i] < newFrame->bounds[0][i] ? oldFrame->bounds[0][i] : newFrame->bounds[0][i];
		bounds[1][i] = oldFrame->bounds[1][i] > newFrame->bounds[1][i] ? oldFrame->bounds[1][i] : newFrame->bounds[1][i];
	}

	switch ( R_CullLocalBox( bounds ) )
	{
	case CULL_IN:
		tr.pc.c_box_cull_md3_in++;
		return CULL_IN;
	case CULL_CLIP:
		tr.pc.c_box_cull_md3_clip++;
		return CULL_CLIP;
	case CULL_OUT:
	default:
		tr.pc.c_box_cull_md3_out++;
		return CULL_OUT;
	}
}


/*
=================
R_ComputeLOD

=================
*/
int R_ComputeLOD( trRefEntity_t *ent ) {
	float radius;
	float flod, lodscale;
	float projectedRadius;
	md3Frame_t *frame;
	int lod;

	if ( tr.currentModel->numLods < 2 ) {
		// model has only 1 LOD level, skip computations and bias
		lod = 0;
	} else
	{
		// multiple LODs exist, so compute projected bounding sphere
		// and use that as a criteria for selecting LOD

		// RF, checked for a forced lowest LOD
		if ( ent->e.reFlags & REFLAG_FORCE_LOD ) {
			return ( tr.currentModel->numLods - 1 );
		}

		frame = ( md3Frame_t * )( ( ( unsigned char * ) tr.currentModel->model.md3[0] ) + tr.currentModel->model.md3[0]->ofsFrames );

		frame += ent->e.frame;

		radius = RadiusFromBounds( frame->bounds[0], frame->bounds[1] );

		//----(SA)	testing
		if ( ent->e.reFlags & REFLAG_ORIENT_LOD ) {
			// right now this is for trees, and pushes the lod distance way in.
			// this is not the intended purpose, but is helpful for the new
			// terrain level that has loads of trees
//			radius = radius/2.0f;
		}
		//----(SA)	end

		if ( ( projectedRadius = ProjectRadius( radius, ent->e.origin ) ) != 0 ) {
			lodscale = r_lodscale->value;
			if ( lodscale > 20 ) {
				lodscale = 20;
			}
			flod = 1.0f - projectedRadius * lodscale;
		} else
		{
			// object intersects near view plane, e.g. view weapon
			flod = 0;
		}

		flod *= tr.currentModel->numLods;
		lod = Q_ftol( flod );

		if ( lod < 0 ) {
			lod = 0;
		} else if ( lod >= tr.currentModel->numLods )   {
			lod = tr.currentModel->numLods - 1;
		}
	}

	lod += r_lodbias->integer;

	if ( lod >= tr.currentModel->numLods ) {
		lod = tr.currentModel->numLods - 1;
	}
	if ( lod < 0 ) {
		lod = 0;
	}

	return lod;
}

/*
=================
R_ComputeFogNum

=================
*/
static int R_ComputeFogNum( md3Header_t *header, trRefEntity_t *ent ) {
	int i, j;
	fog_t           *fog;
	md3Frame_t      *md3Frame;
	vec3_t localOrigin;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	// FIXME: non-normalized axis issues
	md3Frame = ( md3Frame_t * )( ( byte * ) header + header->ofsFrames ) + ent->e.frame;
	VectorAdd( ent->e.origin, md3Frame->localOrigin, localOrigin );
	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( localOrigin[j] - md3Frame->radius >= fog->bounds[1][j] ) {
				break;
			}
			if ( localOrigin[j] + md3Frame->radius <= fog->bounds[0][j] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}

/*
=================
R_AddMD3Surfaces

=================
*/
void R_AddMD3Surfaces( trRefEntity_t *ent ) {
	vec3_t			bounds[2];
	int				i;
	md3Header_t		*header = NULL;
	md3Surface_t	*surface = NULL;
	md3Shader_t		*md3Shader = NULL;
	shader_t		*shader = NULL;
	int				cull;
	int				lod;
	int				fogNum;
	qboolean		personalModel;
#ifdef USE_PMLIGHT
	dlight_t		*dl;
	int				n;
	dlight_t		*dlights[ ARRAY_LEN( backEndData->dlights ) ];
	int				numDlights;
#endif

	// don't add third_person objects if not in a portal
	personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && (tr.viewParms.portalView == PV_NONE);

	if ( ent->e.renderfx & RF_WRAP_FRAMES ) {
		ent->e.frame %= tr.currentModel->model.md3[0]->numFrames;
		ent->e.oldframe %= tr.currentModel->model.md3[0]->numFrames;
	}

	//
	// compute LOD
	//
	if ( ent->e.renderfx & RF_FORCENOLOD ) {
		lod = 0;
	} else {
		lod = R_ComputeLOD( ent );
	}

	//
	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, so
	// when the surfaces are rendered, they don't need to be
	// range checked again.
	//
	if ( ( ent->e.frame >= tr.currentModel->model.md3[lod]->numFrames )
		 || ( ent->e.frame < 0 )
		 || ( ent->e.oldframe >= tr.currentModel->model.md3[lod]->numFrames )
		 || ( ent->e.oldframe < 0 ) ) {

		// low level of detail may be restricted to 1 frame (i.e player head)
		// in this case, we assume the frame number differences is wanted
		// because we don't want animation at far distance
		// skipped warning message

		if( lod == 0 || tr.currentModel->model.md3[lod]->numFrames != 1 ) {
			ri.Printf( PRINT_DEVELOPER, "R_AddMD3Surfaces: no such frame %d to %d for '%s' (%d)\n",
					ent->e.oldframe, ent->e.frame,
					tr.currentModel->name,
					tr.currentModel->model.md3[ lod ]->numFrames );
		}
		ent->e.frame = 0;
		ent->e.oldframe = 0;
	}

	header = tr.currentModel->model.md3[lod];

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	cull = R_CullModel( header, ent, bounds );
	if ( cull == CULL_OUT ) {
		return;
	}

	//
	// set up lighting now that we know we aren't culled
	//
	if ( !personalModel || r_shadows->integer > 1 ) {
		R_SetupEntityLighting( &tr.refdef, ent );
	}

#ifdef USE_PMLIGHT
	numDlights = 0;
	if ( r_dlightMode->integer >= 2 && ( !personalModel || tr.viewParms.portalView != PV_NONE ) ) {
		R_TransformDlights( tr.viewParms.num_dlights, tr.viewParms.dlights, &tr.orientation );
		for ( n = 0; n < tr.viewParms.num_dlights; n++ ) {
			dl = &tr.viewParms.dlights[ n ];
			if ( !R_LightCullBounds( dl, bounds[0], bounds[1] ) ) 
				dlights[ numDlights++ ] = dl;
		}
	}
#endif

	//
	// see if we are in a fog volume
	//
	fogNum = R_ComputeFogNum( header, ent );

	//
	// draw all surfaces
	//
	surface = ( md3Surface_t * )( (byte *)header + header->ofsSurfaces );
	for ( i = 0 ; i < header->numSurfaces ; i++ ) {

		if ( ent->e.customShader ) {
			shader = R_GetShaderByHandle( ent->e.customShader );
		} else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins ) {
			skin_t *skin;
			int hash;
			int j;

			skin = R_GetSkinByHandle( ent->e.customSkin );

			// match the surface name to something in the skin file
			shader = tr.defaultShader;

//----(SA)	added blink
			if ( ent->e.renderfx & RF_BLINK ) {
				const char *s = va( "%s_b", surface->name ); // append '_b' for 'blink'
				hash = ri.MSG_HashKey( s, strlen( s ) );
				for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
					if ( hash != skin->surfaces[j].hash ) {
						continue;
					}
					if ( !strcmp( skin->surfaces[j].name, s ) ) {
						shader = skin->surfaces[j].shader;
						break;
					}
				}
			}

			if ( shader == tr.defaultShader ) {    // blink reference in skin was not found
				hash = ri.MSG_HashKey( surface->name, sizeof( surface->name ) );
				for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
					// the names have both been lowercased
					if ( hash != skin->surfaces[j].hash ) {
						continue;
					}
					if ( !strcmp( skin->surfaces[j].name, surface->name ) ) {
						shader = skin->surfaces[j].shader;
						break;
					}
				}
			}
//----(SA)	end

			if ( shader == tr.defaultShader ) {
				ri.Printf( PRINT_DEVELOPER, "WARNING: no shader for surface %s in skin %s\n", surface->name, skin->name );
			} else if ( shader->defaultShader )     {
				ri.Printf( PRINT_DEVELOPER, "WARNING: shader %s in skin %s not found\n", shader->name, skin->name );
			}
		} else if ( surface->numShaders <= 0 ) {
			shader = tr.defaultShader;
		} else {
			md3Shader = ( md3Shader_t * )( (byte *)surface + surface->ofsShaders );
			md3Shader += ent->e.skinNum % surface->numShaders;
			shader = tr.shaders[ md3Shader->shaderIndex ];
		}


		// we will add shadows even if the main object isn't visible in the view

		// stencil shadows can't do personal models unless I polyhedron clip
		if ( !personalModel
			 && r_shadows->integer == 2
			 && fogNum == 0
			 && !( ent->e.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) )
			 && shader->sort == SS_OPAQUE ) {
			R_AddDrawSurf( (void *)surface, tr.shadowShader, 0, 0 );
		}

		// projection shadows work fine with personal models
		if ( r_shadows->integer == 3
			 && fogNum == 0
			 && ( ent->e.renderfx & RF_SHADOW_PLANE )
			 && shader->sort == SS_OPAQUE ) {
			R_AddDrawSurf( (void *)surface, tr.projectionShadowShader, 0, 0 );
		}


		// for testing polygon shadows (on /all/ models)
		if ( r_shadows->integer == 4 ) {
			R_AddDrawSurf( (void *)surface, tr.projectionShadowShader, 0, 0 );
		}


		// don't add third_person objects if not viewing through a portal
		if ( !personalModel ) {
			R_AddDrawSurf( (void *)surface, shader, fogNum, 0 );
		}

#ifdef USE_PMLIGHT
		if ( numDlights && shader->lightingStage >= 0 ) {
			for ( n = 0; n < numDlights; n++ ) {
				dl = dlights[ n ];
				tr.light = dl;
				R_AddLitSurf( (void *)surface, shader, fogNum );
			}
		}
#endif

		surface = ( md3Surface_t * )( (byte *)surface + surface->ofsEnd );
	}
}
