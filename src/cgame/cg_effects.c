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

// cg_effects.c -- these functions generate localentities, usually as a result
// of event processing

#include "cg_local.h"


/*
==================
CG_BubbleTrail

Bullets shot underwater
==================
*/
void CG_BubbleTrail(vec3_t start, vec3_t end, float size, float spacing) {
	vec3_t move;
	vec3_t vec;
	float  len;
	int    i;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	// advance a random amount first
	i = rand() % (int)spacing;
	VectorMA(move, i, vec, move);

	VectorScale(vec, spacing, vec);

	for ( ; i < len; i += spacing) {
		localEntity_t *le;
		refEntity_t   *re;

		le            = CG_AllocLocalEntity();
		le->leFlags   = LEF_PUFF_DONT_SCALE;
		le->leType    = LE_MOVE_SCALE_FADE;
		le->startTime = cg.time;
		le->endTime   = cg.time + 1000 + random() * 250;
		le->lifeRate  = 1.0 / (le->endTime - le->startTime);

		re             = &le->refEntity;
		re->shaderTime = cg.time / 1000.0f;

		re->reType        = RT_SPRITE;
		re->rotation      = 0;
		re->radius        = size; // (SA)
		re->customShader  = cgs.media.waterBubbleShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;

		le->color[3] = 1.0;

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.time;
		VectorCopy(move, le->pos.trBase);
		le->pos.trDelta[0] = crandom() * 3;
		le->pos.trDelta[1] = crandom() * 3;
		le->pos.trDelta[2] = crandom() * 5 + 20;  // (SA)

		VectorAdd(move, vec, move);
	}
}

/*
=====================
CG_SmokePuff

Adds a smoke puff or blood trail localEntity.

(SA) boy, it would be nice to have an acceleration vector for this as well.
        big velocity vector with a negative acceleration for deceleration, etc.
        (breath could then come out of a guys mouth at the rate he's walking/running and it
        would slow down once it's created)
=====================
*/

//----(SA)	modified
localEntity_t *CG_SmokePuff(const vec3_t p, const vec3_t vel,
                            float radius,
                            float r, float g, float b, float a,
                            float duration,
                            int startTime,
                            int fadeInTime,
                            int leFlags,
                            qhandle_t hShader) {
	static int    seed = 0x92;
	localEntity_t *le;
	refEntity_t   *re;

	le          = CG_AllocLocalEntity();
	le->leFlags = leFlags;
	le->radius  = radius;

	re             = &le->refEntity;
	re->rotation   = Q_random(&seed) * 360;
	re->radius     = radius;
	re->shaderTime = startTime / 1000.0f;

	le->leType     = LE_MOVE_SCALE_FADE;
	le->startTime  = startTime;
	le->endTime    = startTime + duration;
	le->fadeInTime = fadeInTime;
	if (fadeInTime > startTime) {
		le->lifeRate = 1.0 / (le->endTime - le->fadeInTime);
	} else {
		le->lifeRate = 1.0 / (le->endTime - le->startTime);
	}
	le->color[0] = r;
	le->color[1] = g;
	le->color[2] = b;
	le->color[3] = a;


	le->pos.trType = TR_LINEAR;
	le->pos.trTime = startTime;
	VectorCopy(vel, le->pos.trDelta);
	VectorCopy(p, le->pos.trBase);

	VectorCopy(p, re->origin);
	re->customShader = hShader;

	// rage pro can't alpha fade, so use a different shader
	if (cgs.glconfig.hardwareType == GLHW_RAGEPRO) {
		re->customShader  = cgs.media.smokePuffRageProShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;
	} else {
		re->shaderRGBA[0] = le->color[0] * 0xff;
		re->shaderRGBA[1] = le->color[1] * 0xff;
		re->shaderRGBA[2] = le->color[2] * 0xff;
		re->shaderRGBA[3] = 0xff;
	}
// JPW NERVE
	if (cg_fxflags & 1) {
		re->customShader = getTestShader();
		re->rotation     = 180;
	}
// jpw

	re->reType = RT_SPRITE;
	re->radius = le->radius;

	return le;
}

qhandle_t getTestShader(void) {
	switch (rand() % 2) {
	case 0:
		return cgs.media.nerveTestShader;

	default:
		return cgs.media.idTestShader;
	}
}

/*
====================
CG_MakeExplosion
====================
*/
localEntity_t *CG_MakeExplosion(vec3_t origin, vec3_t dir,
                                qhandle_t hModel, qhandle_t shader,
                                int msec, qboolean isSprite) {
	float         ang;
	localEntity_t *ex;
	int           offset;
	vec3_t        tmpVec, newOrigin;

	if (msec <= 0) {
		CG_Error("CG_MakeExplosion: msec = %i", msec);
	}

	// skew the time a bit so they aren't all in sync
	offset = rand() & 63;

	ex = CG_AllocLocalEntity();
	if (isSprite) {
		ex->leType = LE_SPRITE_EXPLOSION;

		// randomly rotate sprite orientation
		ex->refEntity.rotation = rand() % 360;
		VectorScale(dir, 16, tmpVec);
		VectorAdd(tmpVec, origin, newOrigin);
	} else {
		ex->leType = LE_EXPLOSION;
		VectorCopy(origin, newOrigin);

		// set axis with random rotate
		if (!dir) {
			AxisClear(ex->refEntity.axis);
		} else {
			ang = rand() % 360;
			VectorCopy(dir, ex->refEntity.axis[0]);
			RotateAroundDirection(ex->refEntity.axis, ang);
		}
	}

	ex->startTime = cg.time - offset;
	ex->endTime   = ex->startTime + msec;

	// bias the time so all shader effects start correctly
	ex->refEntity.shaderTime = ex->startTime / 1000.0f;

	ex->refEntity.hModel       = hModel;
	ex->refEntity.customShader = shader;

	// set origin
	VectorCopy(newOrigin, ex->refEntity.origin);
	VectorCopy(newOrigin, ex->refEntity.oldorigin);

	// Ridah, move away from the wall as the sprite expands
	ex->pos.trType = TR_LINEAR;
	ex->pos.trTime = cg.time;
	VectorCopy(newOrigin, ex->pos.trBase);
	VectorScale(dir, 48, ex->pos.trDelta);
	// done.

	ex->color[0] = ex->color[1] = ex->color[2] = 1.0;

	return ex;
}

/*
======================
CG_GetOriginForTag

  places the position of the tag into "org"

  returns the index of the tag it used, so we can cycle through tag's with the same name
======================
*/
int CG_GetOriginForTag(refEntity_t *parent, char *tagName, int startIndex, vec3_t org, vec3_t axis[3]) {
	int           i;
	orientation_t lerped;
	int           retval;

	// lerp the tag
	retval = trap_R_LerpTag(&lerped, parent, tagName, startIndex);

	if (retval < 0) {
		return retval;
	}

	VectorCopy(parent->origin, org);

	for (i = 0 ; i < 3 ; i++) {
		VectorMA(org, lerped.origin[i], parent->axis[i], org);
	}

	if (axis) {
		// had to cast away the const to avoid compiler problems...
		MatrixMultiply(lerped.axis, ((refEntity_t *)parent)->axis, axis);
	}

	return retval;
}

#define MAX_SPOT_SEGS 20
#define MAX_SPOT_RANGE 2000
/*
==============
CG_Spotlight
    segs:	number of sides on tube. - 999 is a valid value and just means, 'cap to max' (MAX_SPOT_SEGS) or use lod scheme
    range:	effective range of beam
    startWidth: will be optimized for '0' as a value (true cone) to not use quads and not cap the start circle

    -- flags --
    SL_NOTRACE			- don't do a trace check for shortening the beam, always draw at full 'range' length
    SL_TRACEWORLDONLY	- go through everything but the world
    SL_NODLIGHT			- don't put a dlight at the end
    SL_NOSTARTCAP		- dont' cap the start circle
    SL_LOCKTRACETORANGE	- only trace out as far as the specified range (rather than to max spot range)
    SL_NOFLARE			- don't draw a flare when the light is pointing at the camera
    SL_NOIMPACT			- don't draw the impact mark on hit surfaces
    SL_LOCKUV			- lock the texture coordinates at the 'true' length of the requested beam.
    SL_NOCORE			- don't draw the center 'core' beam






  I know, this is a bit kooky right now.  It evolved big, but now that I know what it should do, it'll get
  crunched down to a bunch of table driven stuff.  once it works, I'll make it work well...

==============
*/

void CG_Spotlight(centity_t *cent, float *color, vec3_t realstart, vec3_t lightDir, int segs, float range, int startWidth, float coneAngle, int flags) {
	int         i, j;
	vec3_t      start, traceEnd;
	vec3_t      right, up;
	vec3_t      v1, v2;
	vec3_t      startvec, endvec;   // the vectors to rotate around lightDir to create the circles
	vec3_t      conevec;
	vec3_t      start_points[MAX_SPOT_SEGS], end_points[MAX_SPOT_SEGS];
	vec3_t      coreright;
	polyVert_t  verts[MAX_SPOT_SEGS * 4]; // x4 for 4 verts per poly
	polyVert_t  plugVerts[MAX_SPOT_SEGS];
	vec3_t      endCenter;
	polyVert_t  coreverts[4];
	trace_t     tr;
	float       radius = 0.0; // TTimo might be used uninitialized
	float       coreEndRadius;
	qboolean    capStart = qtrue;
	float       hitDist;    // the actual distance of the trace impact	(0 is no hit)
	float       beamLen;    // actual distance of the drawn beam
	float       endAlpha = 0.0;
	vec4_t      colorNorm;  // normalized color vector
	refEntity_t ent;
	vec3_t      angles;
	vec4_t      projection;

	// Nico, init vars
	tr.endpos[0] = 0;
	tr.endpos[1] = 0;
	tr.endpos[2] = 0;

	VectorCopy(realstart, start);

	// normalize color
	colorNorm[3] = 0;   // store normalize multiplier in alpha index
	for (i = 0; i < 3; i++) {
		if (color[i] > colorNorm[3]) {
			colorNorm[3] = color[i];    // find largest color value in RGB
		}
	}

	if (colorNorm[3] != 1) {       // it needs to be boosted
		VectorMA(color, 1.0 / colorNorm[3], color, colorNorm);      // FIXME: div by 0
	} else {
		VectorCopy(color, colorNorm);
	}
	colorNorm[3] = color[3];


	if (flags & SL_NOSTARTCAP) {
		capStart = qfalse;
	}

	if (startWidth == 0) {     // cone, not cylinder
		capStart = qfalse;
	}

	if (flags & SL_LOCKTRACETORANGE) {
		VectorMA(start, range, lightDir, traceEnd);             // trace out to 'range'
	} else {
		VectorMA(start, MAX_SPOT_RANGE, lightDir, traceEnd);    // trace all the way out to max dist
	}

	// first trace to see if anything is hit
	if (flags & SL_NOTRACE) {
		tr.fraction = 1.0;  // force no hit
	} else {
		if (flags & SL_TRACEWORLDONLY) {
			CG_Trace(&tr, start, NULL, NULL, traceEnd, -1, CONTENTS_SOLID);
		} else {
			CG_Trace(&tr, start, NULL, NULL, traceEnd, -1, MASK_SHOT);
		}
	}


	if (tr.fraction < 1.0) {
		hitDist = beamLen = MAX_SPOT_RANGE * tr.fraction;
		if (beamLen > range) {
			beamLen = range;
		}
	} else {
		hitDist = 0;
		beamLen = range;
	}


	if (flags & SL_LOCKUV && beamLen < range) {
		endAlpha = 255.0f * (color[3] - (color[3] * beamLen / range));
	}


	if (segs >= MAX_SPOT_SEGS) {
		segs = MAX_SPOT_SEGS - 1;
	}

	// TODO: adjust segs based on r_lodbias
	// TODO: move much of this to renderer


// model at base
	if (cent->currentState.modelindex) {
		memset(&ent, 0, sizeof (ent));
		ent.frame    = 0;
		ent.oldframe = 0;
		ent.backlerp = 0;
		VectorCopy(cent->lerpOrigin, ent.origin);
		VectorCopy(cent->lerpOrigin, ent.oldorigin);
		ent.hModel = cgs.gameModels[cent->currentState.modelindex];
		vectoangles(lightDir, angles);
		AnglesToAxis(angles, ent.axis);
		trap_R_AddRefEntityToScene(&ent);
		memcpy(&cent->refEnt, &ent, sizeof (refEntity_t));

		// push start out a bit so the beam fits to the front of the base model
		VectorMA(start, 14, lightDir, start);
	}

//// BEAM

	PerpendicularVector(up, lightDir);
	CrossProduct(lightDir, up, right);

	// find first vert of the start
	VectorScale(right, startWidth, startvec);
	// find the first vert of the end
	RotatePointAroundVector(conevec, up, lightDir, -coneAngle);
	VectorMA(startvec, beamLen, conevec, endvec);     // this applies the offset of the start diameter before finding the end points

	VectorScale(lightDir, beamLen, endCenter);
	VectorSubtract(endCenter, endvec, coreverts[3].xyz);      // get a vector of the radius out at the end for the core to use
	coreEndRadius = VectorLength(coreverts[3].xyz);
#define CORESCALE 0.6f

//
//	generate the flat beam 'core'
//
	if (!(flags & SL_NOCORE)) {
		VectorSubtract(start, cg.refdef_current->vieworg, v1);
		VectorNormalize(v1);
		VectorSubtract(traceEnd, cg.refdef_current->vieworg, v2);
		VectorNormalize(v2);
		CrossProduct(v1, v2, coreright);
		VectorNormalize(coreright);

		memset(&coreverts[0], 0, 4 * sizeof (polyVert_t));
		VectorMA(start, startWidth * 0.5f, coreright, coreverts[0].xyz);
		VectorMA(start, -startWidth * 0.5f, coreright, coreverts[1].xyz);
		VectorMA(endCenter, -coreEndRadius * CORESCALE, coreright, coreverts[2].xyz);
		VectorAdd(start, coreverts[2].xyz, coreverts[2].xyz);
		VectorMA(endCenter, coreEndRadius * CORESCALE, coreright, coreverts[3].xyz);
		VectorAdd(start, coreverts[3].xyz, coreverts[3].xyz);

		for (i = 0; i < 4; i++) {
			coreverts[i].modulate[0] = color[0] * 200.0f;
			coreverts[i].modulate[1] = color[1] * 200.0f;
			coreverts[i].modulate[2] = color[2] * 200.0f;
			coreverts[i].modulate[3] = color[3] * 200.0f;
			if (i > 1) {
				coreverts[i].modulate[3] = 0;
			}
		}

		trap_R_AddPolyToScene(cgs.media.spotLightBeamShader, 4, &coreverts[0]);
	}


//
// generate the beam cylinder
//

	for (i = 0; i <= segs; i++) {
		RotatePointAroundVector(start_points[i], lightDir, startvec, (360.0f / (float)segs) * i);
		VectorAdd(start_points[i], start, start_points[i]);

		RotatePointAroundVector(end_points[i], lightDir, endvec, (360.0f / (float)segs) * i);
		VectorAdd(end_points[i], start, end_points[i]);
	}

	for (i = 0; i < segs; i++) {

		j = (i * 4);

		VectorCopy(start_points[i], verts[(i * 4)].xyz);
		verts[j].st[0]       = 0;
		verts[j].st[1]       = 1;
		verts[j].modulate[0] = color[0] * 255.0f;
		verts[j].modulate[1] = color[1] * 255.0f;
		verts[j].modulate[2] = color[2] * 255.0f;
		verts[j].modulate[3] = color[3] * 255.0f;
		j++;

		VectorCopy(end_points[i], verts[j].xyz);
		verts[j].st[0]       = 0;
		verts[j].st[1]       = 0;
		verts[j].modulate[0] = color[0] * 255.0f;
		verts[j].modulate[1] = color[1] * 255.0f;
		verts[j].modulate[2] = color[2] * 255.0f;
		verts[j].modulate[3] = endAlpha;
		j++;

		VectorCopy(end_points[i + 1], verts[j].xyz);
		verts[j].st[0]       = 1;
		verts[j].st[1]       = 0;
		verts[j].modulate[0] = color[0] * 255.0f;
		verts[j].modulate[1] = color[1] * 255.0f;
		verts[j].modulate[2] = color[2] * 255.0f;
		verts[j].modulate[3] = endAlpha;
		j++;

		VectorCopy(start_points[i + 1], verts[j].xyz);
		verts[j].st[0]       = 1;
		verts[j].st[1]       = 1;
		verts[j].modulate[0] = color[0] * 255.0f;
		verts[j].modulate[1] = color[1] * 255.0f;
		verts[j].modulate[2] = color[2] * 255.0f;
		verts[j].modulate[3] = color[3] * 255.0f;

		if (capStart) {
			VectorCopy(start_points[i], plugVerts[i].xyz);
			plugVerts[i].st[0]       = 0;
			plugVerts[i].st[1]       = 0;
			plugVerts[i].modulate[0] = color[0] * 255.0f;
			plugVerts[i].modulate[1] = color[1] * 255.0f;
			plugVerts[i].modulate[2] = color[2] * 255.0f;
			plugVerts[i].modulate[3] = color[3] * 255.0f;
		}
	}

	trap_R_AddPolysToScene(cgs.media.spotLightBeamShader, 4, &verts[0], segs);

	// plug up the start circle
	if (capStart) {
		trap_R_AddPolyToScene(cgs.media.spotLightBeamShader, segs, &plugVerts[0]);
	}

	// show the endpoint

	if (!(flags & SL_NOIMPACT) && hitDist) {
		VectorMA(startvec, hitDist, conevec, endvec);

		radius = coreEndRadius * (hitDist / beamLen);

		VectorCopy(lightDir, projection);
		projection[3] = radius * 2.0f;
		CG_ImpactMark(cgs.media.spotLightShader, tr.endpos, projection, radius, colorNorm[0], colorNorm[1], colorNorm[2], 1.0f, 1.0f, -1);
	}

	// add d light at end
	if (!(flags & SL_NODLIGHT)) {
		vec3_t dlightLoc;
		VectorMA(tr.endpos, 0, lightDir, dlightLoc);      // back away from the hit
		trap_R_AddLightToScene(dlightLoc, radius * 2, 0.3, 1.0, 1.0, 1.0, 0, 0);
	}

	// draw flare at source
	if (!(flags & SL_NOFLARE)) {
		qboolean lightInEyes = qfalse;
		vec3_t   camloc, dirtolight;
		float    dot, deg, dist;
		float    flarescale = 0.0;    // TTimo: might be used uninitialized

		// get camera position and direction to lightsource
		VectorCopy(cg.snap->ps.origin, camloc);
		camloc[2] += cg.snap->ps.viewheight;
		VectorSubtract(start, camloc, dirtolight);
		dist = VectorNormalize(dirtolight);

		// first use dot to determine if it's facing the camera
		dot = DotProduct(lightDir, dirtolight);

		// it's facing the camera, find out how closely and trace to see if the source can be seen

		deg = RAD2DEG(M_PI - acos(dot));
		if (deg <= 35) {   // start flare a bit before the camera gets inside the cylinder
			lightInEyes = qtrue;
			flarescale  = 1 - (deg / 35);
		}

		if (lightInEyes) {     // the dot check succeeded, now do a trace
			CG_Trace(&tr, start, NULL, NULL, camloc, -1, MASK_ALL & ~(CONTENTS_MONSTERCLIP | CONTENTS_AREAPORTAL | CONTENTS_CLUSTERPORTAL));
			if (tr.fraction != 1) {
				lightInEyes = qfalse;
			}

		}

		if (lightInEyes) {
			float coronasize = flarescale;
			if (dist < 512) {   // make even bigger if you're close enough
				coronasize *= (512.0f / dist);
			}

			trap_R_AddCoronaToScene(start, colorNorm[0], colorNorm[1], colorNorm[2], coronasize, cent->currentState.number, qtrue);
		} else {
			// even though it's off, still need to add it, but turned off so it can fade in/out properly
			trap_R_AddCoronaToScene(start, colorNorm[0], colorNorm[1], colorNorm[2], 0, cent->currentState.number, qfalse);
		}
	}

}



/*
==============
CG_RumbleEfx
==============
*/
void CG_RumbleEfx(float pitch, float yaw) {
	float  pitchRecoilAdd = 0;
	float  pitchAdd       = 0;
	float  yawRandom      = 0;
	vec3_t recoil;

	if (pitch < 1) {
		pitch = 1;
	}

	pitchRecoilAdd = 0.5 * pow(random(), 8) * (10 + VectorLength(cg.snap->ps.velocity) / 5);
	pitchAdd       = 0.5 * (rand() % (int)pitch) - (pitch * 0.5); //5
	yawRandom      = 0.5 * yaw; //2

	// calc the recoil

	// xkan, 11/04/2002 - the following used to be "recoil[YAW] = crandom()*yawRandom()"
	// but that seems to skew the effect either to the left or to the right for long streches
	// of time. The idea here is to keep it skewed for short period of time and then switches
	// to the other direction - switch the sign of recoil[YAW] when random() < 0.05 and keep the
	// sign otherwise. This seems better at balancing out the effect.
	if (cg.kickAVel[YAW] > 0) {
		if (random() < 0.05) {
			recoil[YAW] = -random() * yawRandom;
		} else {
			recoil[YAW] = random() * yawRandom;
		}
	} else if (cg.kickAVel[YAW] < 0) {
		if (random() < 0.05) {
			recoil[YAW] = random() * yawRandom;
		} else {
			recoil[YAW] = -random() * yawRandom;
		}
	} else {
		if (random() < 0.5) {
			recoil[YAW] = random() * yawRandom;
		} else {
			recoil[YAW] = -random() * yawRandom;
		}
	}

	recoil[ROLL]  = -recoil[YAW];   // why not
	recoil[PITCH] = -pitchAdd;
	// scale it up a bit (easier to modify this while tweaking)
	VectorScale(recoil, 30, recoil);
	// set the recoil
	VectorCopy(recoil, cg.kickAVel);
	// set the recoil
	cg.recoilPitch -= pitchRecoilAdd;
}

#define MAX_SMOKESPRITES 512
#define SMOKEBOMB_DISTANCEBETWEENSPRITES 16.f
#define SMOKEBOMB_SPAWNRATE 10
#define SMOKEBOMB_SMOKEVELOCITY ((640.f - 16.f) / 8) / 1000.f       // units per msec

typedef struct smokesprite_s {
	struct smokesprite_s *next;
	struct smokesprite_s *prev;             // this one is only valid for alloced smokesprites

	vec3_t pos;
	vec4_t colour;

	vec3_t dir;
	float dist;
	float size;

	centity_t *smokebomb;
} smokesprite_t;

static smokesprite_t SmokeSprites[MAX_SMOKESPRITES];
static int           SmokeSpriteCount = 0;
static smokesprite_t *firstfreesmokesprite;         // pointer to the first free smokepuff in the SmokeSprites pool
static smokesprite_t *lastusedsmokesprite;          // pointer to the last used smokepuff

void InitSmokeSprites(void) {
	int i;

	memset(&SmokeSprites, 0, sizeof (SmokeSprites));
	for (i = 0; i < MAX_SMOKESPRITES - 1; i++) {
		SmokeSprites[i].next = &SmokeSprites[i + 1];
	}

	firstfreesmokesprite = &SmokeSprites[0];
	lastusedsmokesprite  = NULL;
	SmokeSpriteCount     = 0;
}

static smokesprite_t *AllocSmokeSprite(void) {
	smokesprite_t *alloc;

	if (SmokeSpriteCount >= MAX_SMOKESPRITES) {
		return NULL;
	}

	alloc = firstfreesmokesprite;

	firstfreesmokesprite = alloc->next;

	if (lastusedsmokesprite) {
		lastusedsmokesprite->next = alloc;
	}

	alloc->next         = NULL;
	alloc->prev         = lastusedsmokesprite;
	lastusedsmokesprite = alloc;

	SmokeSpriteCount++;
	return alloc;
}

// Returns previous alloced smokesprite in list (or NULL when there are no more alloced smokesprites left)
static smokesprite_t *DeAllocSmokeSprite(smokesprite_t *dealloc) {
	smokesprite_t *ret_smokesprite;

	if (dealloc->prev) {
		dealloc->prev->next = dealloc->next;
	}

	if (dealloc->next) {
		dealloc->next->prev = dealloc->prev;
	} else {   // no next particle, so this particle was 'lastusedsmokesprite'
		lastusedsmokesprite = dealloc->prev;
		if (lastusedsmokesprite) {   // incase that there was no previous particle (happens when there is only one particle and this one gets dealloced)
			lastusedsmokesprite->next = NULL;
		}
	}

	ret_smokesprite = dealloc->prev;

	memset(dealloc, 0, sizeof (smokesprite_t));
	dealloc->next        = firstfreesmokesprite;
	firstfreesmokesprite = dealloc;

	SmokeSpriteCount--;
	return ret_smokesprite;
}

static qboolean CG_SmokeSpritePhysics(smokesprite_t *smokesprite, const float dist) {
	trace_t tr;
	vec3_t  oldpos;

	VectorCopy(smokesprite->pos, oldpos);
	VectorMA(oldpos, dist, smokesprite->dir, smokesprite->pos);

	smokesprite->dist += dist;

	smokesprite->size += 1.25f * dist;

	CG_Trace(&tr, oldpos, NULL, NULL, smokesprite->pos, -1, CONTENTS_SOLID);

	if (tr.fraction != 1.f) {
		if (smokesprite->dist < 24.f) {
			return qfalse;
		}
		VectorCopy(tr.endpos, smokesprite->pos);
	}
	return qtrue;
}

qboolean CG_SpawnSmokeSprite(centity_t *cent, float dist) {
	smokesprite_t *smokesprite = AllocSmokeSprite();

	if (smokesprite) {
		smokesprite->smokebomb = cent;
		VectorCopy(cent->origin2, smokesprite->pos);
		VectorCopy(bytedirs[rand() % NUMVERTEXNORMALS], smokesprite->dir);
		smokesprite->dir[2]   *= .5f;
		smokesprite->size      = 16.f;
		smokesprite->colour[0] = .35f;
		smokesprite->colour[1] = smokesprite->colour[0];
		smokesprite->colour[2] = smokesprite->colour[0];
		smokesprite->colour[3] = .8f;

		// Advance sprite
		if (!CG_SmokeSpritePhysics(smokesprite, dist)) {
			DeAllocSmokeSprite(smokesprite);
			return qfalse;
		}
		cent->miscTime++;
	}

	return qtrue;
}

void CG_RenderSmokeGrenadeSmoke(centity_t *cent, const weaponInfo_t *weapon) {
	int           spritesNeeded = 0;
	smokesprite_t *smokesprite;
	float         spawnrate = (1.f / SMOKEBOMB_SPAWNRATE) * 1000.f;

	if (cent->currentState.effect1Time == 16) {
		cent->miscTime          = 0;
		cent->lastFuseSparkTime = 0;    // last spawn time
		cent->muzzleFlashTime   = 0;    // delta time
		cent->dl_atten          = 0;
		return;
	}

	if (cent->currentState.effect1Time > 16) {
		int volume = 16 + ((cent->currentState.effect1Time / 640.f) * (100 - 16));

		if (!cent->dl_atten ||
		    cent->currentState.pos.trType != TR_STATIONARY ||
		    (cent->currentState.groundEntityNum != ENTITYNUM_WORLD && !VectorCompare(cent->lastLerpOrigin, cent->lerpOrigin))) {
			trace_t tr;

			VectorCopy(cent->lerpOrigin, cent->origin2);
			cent->origin2[2] += 32;
			CG_Trace(&tr, cent->currentState.pos.trBase, NULL, NULL, cent->origin2, -1, CONTENTS_SOLID);

			if (tr.startsolid) {
				cent->dl_atten = 2;
			} else {
				VectorCopy(tr.endpos, cent->origin2);
				cent->dl_atten = 1;
			}
		}

		trap_S_AddLoopingSound(cent->lerpOrigin, vec3_origin, weapon->overheatSound, volume, 0);

		// emitter is stuck in solid
		if (cent->dl_atten == 2) {
			return;
		}

		if (cg.oldTime && cent->lastFuseSparkTime != cg.time) {
			cent->muzzleFlashTime  += cg.frametime;
			spritesNeeded           = cent->muzzleFlashTime / spawnrate;
			cent->muzzleFlashTime  -= (spawnrate * spritesNeeded);
			cent->lastFuseSparkTime = cg.time;
		}

		if (!spritesNeeded) {
			return;
		} else if (spritesNeeded == 1) {
			// this is better
			if (!CG_SpawnSmokeSprite(cent, 0.f)) {
				// try again, just in case, so we don't get lots of gaps and remain quite constant
				CG_SpawnSmokeSprite(cent, 0.f);
			}
		} else {
			float lerp = 1.0f;
			float dtime;

			for (dtime = spritesNeeded * spawnrate; dtime > 0; dtime -= spawnrate) {
				if (!CG_SpawnSmokeSprite(cent, lerp * cg.frametime * SMOKEBOMB_SMOKEVELOCITY)) {
					// try again, just in case, so we don't get lots of gaps and remain quite constant
					CG_SpawnSmokeSprite(cent, lerp * cg.frametime * SMOKEBOMB_SMOKEVELOCITY);
				}
			}
		}
	} else if (cent->currentState.effect1Time == -1 && cent->miscTime > 0) {
		// unlink smokesprites from smokebomb
		smokesprite = lastusedsmokesprite;
		while (smokesprite) {
			if (smokesprite->smokebomb == cent) {
				smokesprite->smokebomb = NULL;
				cent->miscTime--;
			}
			smokesprite = smokesprite->prev;
		}
	}
}

void CG_AddSmokeSprites(void) {
	smokesprite_t *smokesprite;
	qhandle_t     shader;
	byte          color[4];
	polyVert_t    verts[4];
	vec3_t        top, bottom;
	vec3_t        right, up, tmp;
	float         radius;
	float         halfSmokeSpriteWidth, halfSmokeSpriteHeight;
	float         dist = SMOKEBOMB_SMOKEVELOCITY * cg.frametime;

	smokesprite = lastusedsmokesprite;
	while (smokesprite) {
		if (smokesprite->smokebomb && !smokesprite->smokebomb->currentValid) {
			smokesprite = smokesprite->prev;
			continue;
		}

		// Do physics
		if (!CG_SmokeSpritePhysics(smokesprite, dist)) {
			if (smokesprite->smokebomb) {
				smokesprite->smokebomb->miscTime--;
			}
			smokesprite = DeAllocSmokeSprite(smokesprite);
			continue;
		}

		if (smokesprite->smokebomb) {
			radius = smokesprite->smokebomb->currentState.effect1Time;
		} else {
			radius = -1.f;
		}

		if (radius < 0) {
			radius = 640.f; // max radius

		}
		// Expire sprites
		if (smokesprite->dist > radius * .5f) {
			if (smokesprite->smokebomb) {
				smokesprite->smokebomb->miscTime--;
			}

			smokesprite = DeAllocSmokeSprite(smokesprite);
			continue;
		}

		// Now render it
		halfSmokeSpriteWidth  = 0.5f * smokesprite->size;
		halfSmokeSpriteHeight = 0.5f * smokesprite->size;

		VectorCopy(cg.refdef_current->viewaxis[1], tmp);
		RotatePointAroundVector(right, cg.refdef_current->viewaxis[0], tmp, 0);
		CrossProduct(cg.refdef_current->viewaxis[0], right, up);

		VectorMA(smokesprite->pos, halfSmokeSpriteHeight, up, top);
		VectorMA(smokesprite->pos, -halfSmokeSpriteHeight, up, bottom);

		color[0] = smokesprite->colour[0] * 0xff;
		color[1] = smokesprite->colour[1] * 0xff;
		color[2] = smokesprite->colour[2] * 0xff;
		color[3] = smokesprite->colour[3] * 0xff;

		// fadeout
		if (smokesprite->dist > (radius * .5f * .8f)) {
			color[3] = (smokesprite->colour[3] -  smokesprite->colour[3] * ((smokesprite->dist - (radius * .5f * .8f)) / ((radius * .5f) - (radius * .5f * .8f)))) * 0xff;
		} else {
			color[3] = smokesprite->colour[3] * 0xff;
		}

		VectorMA(top, halfSmokeSpriteWidth, right, verts[0].xyz);
		verts[0].st[0] = 1;
		verts[0].st[1] = 0;
		memcpy(verts[0].modulate, color, 4);

		VectorMA(top, -halfSmokeSpriteWidth, right, verts[1].xyz);
		verts[1].st[0] = 0;
		verts[1].st[1] = 0;
		memcpy(verts[1].modulate, color, 4);

		VectorMA(bottom, -halfSmokeSpriteWidth, right, verts[2].xyz);
		verts[2].st[0] = 0;
		verts[2].st[1] = 1;
		memcpy(verts[2].modulate, color, 4);

		VectorMA(bottom, halfSmokeSpriteWidth, right, verts[3].xyz);
		verts[3].st[0] = 1;
		verts[3].st[1] = 1;
		memcpy(verts[3].modulate, color, 4);

		shader = cgs.media.smokePuffShader;

		trap_R_AddPolyToScene(shader, 4, verts);

		smokesprite = smokesprite->prev;
	}
}
