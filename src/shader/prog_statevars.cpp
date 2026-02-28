/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file prog_statevars.c
 * Program state variable management.
 * \author Brian Paul
 */


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "prog_statevars.h"
#include "prog_parameter.h"
#include "nvvertparse.h"


/**
 * Use the list of tokens in the state[] array to find global GL state
 * and return it in <value>.  Usually, four values are returned in <value>
 * but matrix queries may return as many as 16 values.
 * This function is used for ARB vertex/fragment programs.
 * The program parser will produce the state[] values.
 */
static void
_mesa_fetch_state(GLcontext *ctx, const gl_state_index state[],
		  GLfloat *value)
{
    switch (state[0]) {
	case STATE_MATERIAL: {
	    /* state[1] is either 0=front or 1=back side */
	    const GLuint face = static_cast<GLuint>(state[1]);
	    const struct gl_material *mat = &ctx->Light.Material;
	    assert(face == 0 || face == 1);
	    /* we rely on tokens numbered so that _BACK_ == _FRONT_+ 1 */
	    assert(MAT_ATTRIB_FRONT_AMBIENT + 1 == MAT_ATTRIB_BACK_AMBIENT);
	    /* XXX we could get rid of this switch entirely with a little
	     * work in arbprogparse.c's parse_state_single_item().
	     */
	    /* state[2] is the material attribute */
	    switch (state[2]) {
		case STATE_AMBIENT:
		    mesa_copy4v(value, mat->Attrib[MAT_ATTRIB_FRONT_AMBIENT + face]);
		    return;
		case STATE_DIFFUSE:
		    mesa_copy4v(value, mat->Attrib[MAT_ATTRIB_FRONT_DIFFUSE + face]);
		    return;
		case STATE_SPECULAR:
		    mesa_copy4v(value, mat->Attrib[MAT_ATTRIB_FRONT_SPECULAR + face]);
		    return;
		case STATE_EMISSION:
		    mesa_copy4v(value, mat->Attrib[MAT_ATTRIB_FRONT_EMISSION + face]);
		    return;
		case STATE_SHININESS:
		    value[0] = mat->Attrib[MAT_ATTRIB_FRONT_SHININESS + face][0];
		    value[1] = 0.0F;
		    value[2] = 0.0F;
		    value[3] = 1.0F;
		    return;
		default:
		    _mesa_problem(ctx, "Invalid material state in fetch_state");
		    return;
	    }
	}
	case STATE_LIGHT: {
	    /* state[1] is the light number */
	    const GLuint ln = static_cast<GLuint>(state[1]);
	    /* state[2] is the light attribute */
	    switch (state[2]) {
		case STATE_AMBIENT:
		    mesa_copy4v(value, ctx->Light.Light[ln].Ambient);
		    return;
		case STATE_DIFFUSE:
		    mesa_copy4v(value, ctx->Light.Light[ln].Diffuse);
		    return;
		case STATE_SPECULAR:
		    mesa_copy4v(value, ctx->Light.Light[ln].Specular);
		    return;
		case STATE_POSITION:
		    mesa_copy4v(value, ctx->Light.Light[ln].EyePosition);
		    return;
		case STATE_ATTENUATION:
		    value[0] = ctx->Light.Light[ln].ConstantAttenuation;
		    value[1] = ctx->Light.Light[ln].LinearAttenuation;
		    value[2] = ctx->Light.Light[ln].QuadraticAttenuation;
		    value[3] = ctx->Light.Light[ln].SpotExponent;
		    return;
		case STATE_SPOT_DIRECTION:
		    mesa_copy3v(value, ctx->Light.Light[ln].EyeDirection);
		    value[3] = ctx->Light.Light[ln]._CosCutoff;
		    return;
		case STATE_SPOT_CUTOFF:
		    value[0] = ctx->Light.Light[ln].SpotCutoff;
		    return;
		case STATE_HALF_VECTOR: {
		    static const GLfloat eye_z[] = {0, 0, 1};
		    GLfloat p[3];
		    /* Compute infinite half angle vector:
		     *   halfVector = normalize(normalize(lightPos) + (0, 0, 1))
		    * light.EyePosition.w should be 0 for infinite lights.
		     */
		    mesa_copy3v(p, ctx->Light.Light[ln].EyePosition);
		    mesa_normalize3fv(p);
		    mesa_add3v(value, p, eye_z);
		    mesa_normalize3fv(value);
		    value[3] = 1.0;
		}
		return;
		case STATE_POSITION_NORMALIZED:
		    mesa_copy4v(value, ctx->Light.Light[ln].EyePosition);
		    mesa_normalize3fv(value);
		    return;
		default:
		    _mesa_problem(ctx, "Invalid light state in fetch_state");
		    return;
	    }
	}
	case STATE_LIGHTMODEL_AMBIENT:
	    mesa_copy4v(value, ctx->Light.Model.Ambient);
	    return;
	case STATE_LIGHTMODEL_SCENECOLOR:
	    if (state[1] == 0) {
		/* front */
		GLint i;
		for (i = 0; i < 3; i++) {
		    value[i] = ctx->Light.Model.Ambient[i]
			       * ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_AMBIENT][i]
			       + ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_EMISSION][i];
		}
		value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE][3];
	    } else {
		/* back */
		GLint i;
		for (i = 0; i < 3; i++) {
		    value[i] = ctx->Light.Model.Ambient[i]
			       * ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_AMBIENT][i]
			       + ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_EMISSION][i];
		}
		value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_DIFFUSE][3];
	    }
	    return;
	case STATE_LIGHTPROD: {
	    const GLuint ln = static_cast<GLuint>(state[1]);
	    const GLuint face = static_cast<GLuint>(state[2]);
	    GLint i;
	    assert(face == 0 || face == 1);
	    switch (state[3]) {
		case STATE_AMBIENT:
		    for (i = 0; i < 3; i++) {
			value[i] = ctx->Light.Light[ln].Ambient[i] *
				   ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_AMBIENT+face][i];
		    }
		    /* [3] = material alpha */
		    value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_AMBIENT+face][3];
		    return;
		case STATE_DIFFUSE:
		    for (i = 0; i < 3; i++) {
			value[i] = ctx->Light.Light[ln].Diffuse[i] *
				   ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE+face][i];
		    }
		    /* [3] = material alpha */
		    value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE+face][3];
		    return;
		case STATE_SPECULAR:
		    for (i = 0; i < 3; i++) {
			value[i] = ctx->Light.Light[ln].Specular[i] *
				   ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_SPECULAR+face][i];
		    }
		    /* [3] = material alpha */
		    value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_SPECULAR+face][3];
		    return;
		default:
		    _mesa_problem(ctx, "Invalid lightprod state in fetch_state");
		    return;
	    }
	}
	case STATE_TEXGEN: {
	    /* state[1] is the texture unit */
	    const GLuint unit = static_cast<GLuint>(state[1]);
	    /* state[2] is the texgen attribute */
	    switch (state[2]) {
		case STATE_TEXGEN_EYE_S:
		    mesa_copy4v(value, ctx->Texture.Unit[unit].EyePlaneS);
		    return;
		case STATE_TEXGEN_EYE_T:
		    mesa_copy4v(value, ctx->Texture.Unit[unit].EyePlaneT);
		    return;
		case STATE_TEXGEN_EYE_R:
		    mesa_copy4v(value, ctx->Texture.Unit[unit].EyePlaneR);
		    return;
		case STATE_TEXGEN_EYE_Q:
		    mesa_copy4v(value, ctx->Texture.Unit[unit].EyePlaneQ);
		    return;
		case STATE_TEXGEN_OBJECT_S:
		    mesa_copy4v(value, ctx->Texture.Unit[unit].ObjectPlaneS);
		    return;
		case STATE_TEXGEN_OBJECT_T:
		    mesa_copy4v(value, ctx->Texture.Unit[unit].ObjectPlaneT);
		    return;
		case STATE_TEXGEN_OBJECT_R:
		    mesa_copy4v(value, ctx->Texture.Unit[unit].ObjectPlaneR);
		    return;
		case STATE_TEXGEN_OBJECT_Q:
		    mesa_copy4v(value, ctx->Texture.Unit[unit].ObjectPlaneQ);
		    return;
		default:
		    _mesa_problem(ctx, "Invalid texgen state in fetch_state");
		    return;
	    }
	}
	case STATE_TEXENV_COLOR: {
	    /* state[1] is the texture unit */
	    const GLuint unit = static_cast<GLuint>(state[1]);
	    mesa_copy4v(value, ctx->Texture.Unit[unit].EnvColor);
	}
	return;
	case STATE_FOG_COLOR:
	    mesa_copy4v(value, ctx->Fog.Color);
	    return;
	case STATE_FOG_PARAMS:
	    value[0] = ctx->Fog.Density;
	    value[1] = ctx->Fog.Start;
	    value[2] = ctx->Fog.End;
	    value[3] = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
	    return;
	case STATE_CLIPPLANE: {
	    const GLuint plane = static_cast<GLuint>(state[1]);
	    mesa_copy4v(value, ctx->Transform.EyeUserPlane[plane]);
	}
	return;
	case STATE_POINT_SIZE:
	    value[0] = ctx->Point.Size;
	    value[1] = ctx->Point.MinSize;
	    value[2] = ctx->Point.MaxSize;
	    value[3] = ctx->Point.Threshold;
	    return;
	case STATE_POINT_ATTENUATION:
	    value[0] = ctx->Point.Params[0];
	    value[1] = ctx->Point.Params[1];
	    value[2] = ctx->Point.Params[2];
	    value[3] = 1.0F;
	    return;
	case STATE_MODELVIEW_MATRIX:
	case STATE_PROJECTION_MATRIX:
	case STATE_MVP_MATRIX:
	case STATE_TEXTURE_MATRIX:
	case STATE_PROGRAM_MATRIX: {
	    /* state[0] = modelview, projection, texture, etc. */
	    /* state[1] = which texture matrix or program matrix */
	    /* state[2] = first row to fetch */
	    /* state[3] = last row to fetch */
	    /* state[4] = transpose, inverse or invtrans */
	    const GLmatrix *matrix;
	    const gl_state_index mat = state[0];
	    const GLuint index = static_cast<GLuint>(state[1]);
	    const GLuint firstRow = static_cast<GLuint>(state[2]);
	    const GLuint lastRow = static_cast<GLuint>(state[3]);
	    const gl_state_index modifier = state[4];
	    const GLfloat *m;
	    GLuint row, i;
	    assert(firstRow >= 0);
	    assert(firstRow < 4);
	    assert(lastRow >= 0);
	    assert(lastRow < 4);
	    if (mat == STATE_MODELVIEW_MATRIX) {
		matrix = ctx->ModelviewMatrixStack.Top;
	    } else if (mat == STATE_PROJECTION_MATRIX) {
		matrix = ctx->ProjectionMatrixStack.Top;
	    } else if (mat == STATE_MVP_MATRIX) {
		matrix = &ctx->_ModelProjectMatrix;
	    } else if (mat == STATE_TEXTURE_MATRIX) {
		matrix = ctx->TextureMatrixStack[index].Top;
	    } else if (mat == STATE_PROGRAM_MATRIX) {
		matrix = ctx->ProgramMatrixStack[index].Top;
	    } else {
		_mesa_problem(ctx, "Bad matrix name in _mesa_fetch_state()");
		return;
	    }
	    if (modifier == STATE_MATRIX_INVERSE ||
		modifier == STATE_MATRIX_INVTRANS) {
		/* Be sure inverse is up to date:
		 */
		((GLmatrix *) matrix)->alloc_inv();
		((GLmatrix *) matrix)->analyse();
		m = matrix->inv;
	    } else {
		m = matrix->m;
	    }
	    if (modifier == STATE_MATRIX_TRANSPOSE ||
		modifier == STATE_MATRIX_INVTRANS) {
		for (i = 0, row = firstRow; row <= lastRow; row++) {
		    value[i++] = m[row * 4 + 0];
		    value[i++] = m[row * 4 + 1];
		    value[i++] = m[row * 4 + 2];
		    value[i++] = m[row * 4 + 3];
		}
	    } else {
		for (i = 0, row = firstRow; row <= lastRow; row++) {
		    value[i++] = m[row + 0];
		    value[i++] = m[row + 4];
		    value[i++] = m[row + 8];
		    value[i++] = m[row + 12];
		}
	    }
	}
	return;
	case STATE_DEPTH_RANGE:
	    value[0] = ctx->Viewport.Near;                     /* near       */
	    value[1] = ctx->Viewport.Far;                      /* far        */
	    value[2] = ctx->Viewport.Far - ctx->Viewport.Near; /* far - near */
	    value[3] = 1.0;
	    return;
	case STATE_FRAGMENT_PROGRAM: {
	    /* state[1] = {STATE_ENV, STATE_LOCAL} */
	    /* state[2] = parameter index          */
	    const int idx = static_cast<int>(state[2]);
	    switch (state[1]) {
		case STATE_ENV:
		    mesa_copy4v(value, ctx->FragmentProgram.Parameters[idx]);
		    break;
		case STATE_LOCAL:
		    mesa_copy4v(value, ctx->FragmentProgram.Current->LocalParams[idx]);
		    break;
		default:
		    _mesa_problem(ctx, "Bad state switch in _mesa_fetch_state()");
		    return;
	    }
	}
	return;

	case STATE_VERTEX_PROGRAM: {
	    /* state[1] = {STATE_ENV, STATE_LOCAL} */
	    /* state[2] = parameter index          */
	    const int idx = static_cast<int>(state[2]);
	    switch (state[1]) {
		case STATE_ENV:
		    mesa_copy4v(value, ctx->VertexProgram.Parameters[idx]);
		    break;
		case STATE_LOCAL:
		    mesa_copy4v(value, ctx->VertexProgram.Current->LocalParams[idx]);
		    break;
		default:
		    _mesa_problem(ctx, "Bad state switch in _mesa_fetch_state()");
		    return;
	    }
	}
	return;

	case STATE_NORMAL_SCALE:
	    mesa_assign4v(value, ctx->_ModelViewInvScale, 0, 0, 1);
	    return;

	case STATE_INTERNAL:
	    switch (state[1]) {
		case STATE_NORMAL_SCALE:
		    mesa_assign4v(value, ctx->_ModelViewInvScale, 0, 0, 1);
		    return;
		case STATE_TEXRECT_SCALE: {
		    const int unit = static_cast<int>(state[2]);
		    const struct gl_texture_object *texObj
			    = ctx->Texture.Unit[unit]._Current;
		    if (texObj) {
			struct gl_texture_image *texImage = texObj->Image[0][0];
			mesa_assign4v(value, 1.0 / texImage->Width,
				  1.0 / texImage->Height,
				  0.0, 1.0);
		    }
		}
		return;
		case STATE_FOG_PARAMS_OPTIMIZED:
		    /* for simpler per-vertex/pixel fog calcs. POW (for EXP/EXP2 fog)
		     * might be more expensive than EX2 on some hw, plus it needs
		     * another constant (e) anyway. Linear fog can now be done with a
		     * single MAD.
		     * linear: fogcoord * -1/(end-start) + end/(end-start)
		     * exp: 2^-(density/ln(2) * fogcoord)
		     * exp2: 2^-((density/(ln(2)^2) * fogcoord)^2)
		     */
		    value[0] = -1.0F / (ctx->Fog.End - ctx->Fog.Start);
		    value[1] = ctx->Fog.End / (ctx->Fog.End - ctx->Fog.Start);
		    value[2] = ctx->Fog.Density * ONE_DIV_LN2;
		    value[3] = ctx->Fog.Density * ONE_DIV_SQRT_LN2;
		    return;
		case STATE_SPOT_DIR_NORMALIZED: {
		    /* here, state[2] is the light number */
		    /* pre-normalize spot dir */
		    const GLuint ln = static_cast<GLuint>(state[2]);
		    mesa_copy3v(value, ctx->Light.Light[ln].EyeDirection);
		    mesa_normalize3fv(value);
		    value[3] = ctx->Light.Light[ln]._CosCutoff;
		    return;
		}
		default:
		    /* unknown state indexes are silently ignored
		     *  should be handled by the driver.
		     */
		    return;
	    }
	    return;

	default:
	    _mesa_problem(ctx, "Invalid state in _mesa_fetch_state");
	    return;
    }
}


/**
 * Return a bitmask of the Mesa state flags (_NEW_* values) which would
 * indicate that the given context state may have changed.
 * The bitmask is used during validation to determine if we need to update
 * vertex/fragment program parameters (like "state.material.color") when
 * some GL state has changed.
 */
GLbitfield
_mesa_program_state_flags(const gl_state_index state[STATE_LENGTH])
{
    switch (state[0]) {
	case STATE_MATERIAL:
	case STATE_LIGHT:
	case STATE_LIGHTMODEL_AMBIENT:
	case STATE_LIGHTMODEL_SCENECOLOR:
	case STATE_LIGHTPROD:
	    return _NEW_LIGHT;

	case STATE_TEXGEN:
	case STATE_TEXENV_COLOR:
	    return _NEW_TEXTURE;

	case STATE_FOG_COLOR:
	case STATE_FOG_PARAMS:
	    return _NEW_FOG;

	case STATE_CLIPPLANE:
	    return _NEW_TRANSFORM;

	case STATE_POINT_SIZE:
	case STATE_POINT_ATTENUATION:
	    return _NEW_POINT;

	case STATE_MODELVIEW_MATRIX:
	    return _NEW_MODELVIEW;
	case STATE_PROJECTION_MATRIX:
	    return _NEW_PROJECTION;
	case STATE_MVP_MATRIX:
	    return _NEW_MODELVIEW | _NEW_PROJECTION;
	case STATE_TEXTURE_MATRIX:
	    return _NEW_TEXTURE_MATRIX;
	case STATE_PROGRAM_MATRIX:
	    return _NEW_TRACK_MATRIX;

	case STATE_DEPTH_RANGE:
	    return _NEW_VIEWPORT;

	case STATE_FRAGMENT_PROGRAM:
	case STATE_VERTEX_PROGRAM:
	    return _NEW_PROGRAM;

	case STATE_NORMAL_SCALE:
	    return _NEW_MODELVIEW;

	case STATE_INTERNAL:
	    switch (state[1]) {
		case STATE_TEXRECT_SCALE:
		    return _NEW_TEXTURE;
		case STATE_FOG_PARAMS_OPTIMIZED:
		    return _NEW_FOG;
		default:
		    /* unknown state indexes are silently ignored and
		    *  no flag set, since it is handled by the driver.
		    */
		    return 0;
	    }

	default:
	    _mesa_problem(nullptr, "unexpected state[0] in make_state_flags()");
	    return 0;
    }
}


/** Return the literal string for a gl_state_index token, used in state names. */
static std::string
token_string(gl_state_index k)
{
    switch (k) {
	case STATE_MATERIAL:            return "material";
	case STATE_LIGHT:               return "light";
	case STATE_LIGHTMODEL_AMBIENT:  return "lightmodel.ambient";
	case STATE_LIGHTMODEL_SCENECOLOR: return {};
	case STATE_LIGHTPROD:           return "lightprod";
	case STATE_TEXGEN:              return "texgen";
	case STATE_FOG_COLOR:           return "fog.color";
	case STATE_FOG_PARAMS:          return "fog.params";
	case STATE_CLIPPLANE:           return "clip";
	case STATE_POINT_SIZE:          return "point.size";
	case STATE_POINT_ATTENUATION:   return "point.attenuation";
	case STATE_MODELVIEW_MATRIX:    return "matrix.modelview";
	case STATE_PROJECTION_MATRIX:   return "matrix.projection";
	case STATE_MVP_MATRIX:          return "matrix.mvp";
	case STATE_TEXTURE_MATRIX:      return "matrix.texture";
	case STATE_PROGRAM_MATRIX:      return "matrix.program";
	case STATE_MATRIX_INVERSE:      return ".inverse";
	case STATE_MATRIX_TRANSPOSE:    return ".transpose";
	case STATE_MATRIX_INVTRANS:     return ".invtrans";
	case STATE_AMBIENT:             return ".ambient";
	case STATE_DIFFUSE:             return ".diffuse";
	case STATE_SPECULAR:            return ".specular";
	case STATE_EMISSION:            return ".emission";
	case STATE_SHININESS:           return "lshininess";
	case STATE_HALF_VECTOR:         return ".half";
	case STATE_POSITION:            return ".position";
	case STATE_ATTENUATION:         return ".attenuation";
	case STATE_SPOT_DIRECTION:      return ".spot.direction";
	case STATE_SPOT_CUTOFF:         return ".spot.cutoff";
	case STATE_TEXGEN_EYE_S:        return "eye.s";
	case STATE_TEXGEN_EYE_T:        return "eye.t";
	case STATE_TEXGEN_EYE_R:        return "eye.r";
	case STATE_TEXGEN_EYE_Q:        return "eye.q";
	case STATE_TEXGEN_OBJECT_S:     return "object.s";
	case STATE_TEXGEN_OBJECT_T:     return "object.t";
	case STATE_TEXGEN_OBJECT_R:     return "object.r";
	case STATE_TEXGEN_OBJECT_Q:     return "object.q";
	case STATE_TEXENV_COLOR:        return "texenv";
	case STATE_DEPTH_RANGE:         return "depth.range";
	case STATE_VERTEX_PROGRAM:
	case STATE_FRAGMENT_PROGRAM:    return {};
	case STATE_ENV:                 return "env";
	case STATE_LOCAL:               return "local";
	case STATE_NORMAL_SCALE:        return "normalScale";
	case STATE_INTERNAL:
	case STATE_POSITION_NORMALIZED: return "(internal)";
	default:                        return {};
    }
}

/**
 * Make a string from the given state vector.
 * For example, return "state.matrix.texture[2].inverse".
 */
std::string
_mesa_program_state_string(const gl_state_index state[STATE_LENGTH])
{
    std::string str = "state.";
    str += token_string((gl_state_index) state[0]);

    auto face = [](GLint f) -> std::string { return f == 0 ? "front." : "back."; };
    auto index = [](GLint i) { return '[' + std::to_string(i) + ']'; };

    switch (state[0]) {
	case STATE_MATERIAL:
	    str += face(state[1]);
	    str += token_string((gl_state_index) state[2]);
	    break;
	case STATE_LIGHT:
	    str += index(state[1]); /* light number [i]. */
	    str += token_string((gl_state_index) state[2]); /* coefficients */
	    break;
	case STATE_LIGHTMODEL_AMBIENT:
	    str += "lightmodel.ambient";
	    break;
	case STATE_LIGHTMODEL_SCENECOLOR:
	    str += (state[1] == 0) ? "lightmodel.front.scenecolor"
	                           : "lightmodel.back.scenecolor";
	    break;
	case STATE_LIGHTPROD:
	    str += index(state[1]); /* light number [i]. */
	    str += face(state[2]);
	    str += token_string((gl_state_index) state[3]);
	    break;
	case STATE_TEXGEN:
	    str += index(state[1]); /* tex unit [i] */
	    str += token_string((gl_state_index) state[2]); /* plane coef */
	    break;
	case STATE_TEXENV_COLOR:
	    str += index(state[1]); /* tex unit [i] */
	    str += "color";
	    break;
	case STATE_CLIPPLANE:
	    str += index(state[1]); /* plane [i] */
	    str += ".plane";
	    break;
	case STATE_MODELVIEW_MATRIX:
	case STATE_PROJECTION_MATRIX:
	case STATE_MVP_MATRIX:
	case STATE_TEXTURE_MATRIX:
	case STATE_PROGRAM_MATRIX: {
	    /* state[0] = modelview, projection, texture, etc. */
	    /* state[1] = which texture matrix or program matrix */
	    /* state[2] = first row to fetch */
	    /* state[3] = last row to fetch */
	    /* state[4] = transpose, inverse or invtrans */
	    const gl_state_index mat = (gl_state_index) state[0];
	    const GLuint idx = static_cast<GLuint>(state[1]);
	    const GLuint firstRow = static_cast<GLuint>(state[2]);
	    const GLuint lastRow = static_cast<GLuint>(state[3]);
	    const gl_state_index modifier = (gl_state_index) state[4];
	    if (idx || mat == STATE_TEXTURE_MATRIX || mat == STATE_PROGRAM_MATRIX)
		str += index(idx);
	    if (modifier)
		str += token_string(modifier);
	    if (firstRow == lastRow)
		str += ".row[" + std::to_string(firstRow) + ']';
	    else
		str += ".row[" + std::to_string(firstRow) + ".." + std::to_string(lastRow) + ']';
	}
	break;
	case STATE_POINT_SIZE:
	case STATE_POINT_ATTENUATION:
	case STATE_FOG_PARAMS:
	case STATE_FOG_COLOR:
	case STATE_DEPTH_RANGE:
	    break;
	case STATE_FRAGMENT_PROGRAM:
	case STATE_VERTEX_PROGRAM:
	    /* state[1] = {STATE_ENV, STATE_LOCAL} */
	    /* state[2] = parameter index          */
	    str += token_string((gl_state_index) state[1]);
	    str += index(state[2]);
	    break;
	case STATE_INTERNAL:
	    break;
	default:
	    _mesa_problem(nullptr, "Invalid state in _mesa_program_state_string");
	    break;
    }

    return str;
}


/**
 * Loop over all the parameters in a parameter list.  If the parameter
 * is a GL state reference, look up the current value of that state
 * variable and put it into the parameter's Value[4] array.
 * This would be called at glBegin time when using a fragment program.
 */
void
_mesa_load_state_parameters(GLcontext *ctx,
			    struct gl_program_parameter_list *paramList)
{
    GLuint i;

    if (!paramList)
	return;

    for (i = 0; i < paramList->NumParameters(); i++) {
	if (paramList->Parameters[i].Type == PROGRAM_STATE_VAR) {
	    _mesa_fetch_state(ctx,
			      (gl_state_index *) paramList->Parameters[i].StateIndexes,
			      paramList->ParameterValues[i].data());
	}
    }
}


/**
 * Copy the 16 elements of a matrix into four consecutive program
 * registers starting at 'pos'.
 */
static void
load_matrix(GLfloat registers[][4], GLuint pos, const GLfloat mat[16])
{
    GLuint i;
    for (i = 0; i < 4; i++) {
	registers[pos + i][0] = mat[0 + i];
	registers[pos + i][1] = mat[4 + i];
	registers[pos + i][2] = mat[8 + i];
	registers[pos + i][3] = mat[12 + i];
    }
}


/**
 * As above, but transpose the matrix.
 */
static void
load_transpose_matrix(GLfloat registers[][4], GLuint pos,
		      const GLfloat mat[16])
{
    memcpy(registers[pos], mat, 16 * sizeof(GLfloat));
}


/**
 * Load current vertex program's parameter registers with tracked
 * matrices (if NV program).  This only needs to be done per
 * glBegin/glEnd, not per-vertex.
 */
void
_mesa_load_tracked_matrices(GLcontext *ctx)
{
    GLuint i;

    for (i = 0; i < MAX_NV_VERTEX_PROGRAM_PARAMS / 4; i++) {
	/* point 'mat' at source matrix */
	GLmatrix *mat;
	if (ctx->VertexProgram.TrackMatrix[i] == GL_MODELVIEW) {
	    mat = ctx->ModelviewMatrixStack.Top;
	} else if (ctx->VertexProgram.TrackMatrix[i] == GL_PROJECTION) {
	    mat = ctx->ProjectionMatrixStack.Top;
	} else if (ctx->VertexProgram.TrackMatrix[i] == GL_TEXTURE) {
	    mat = ctx->TextureMatrixStack[ctx->Texture.CurrentUnit].Top;
	} else if (ctx->VertexProgram.TrackMatrix[i] == GL_COLOR) {
	    mat = ctx->ColorMatrixStack.Top;
	} else if (ctx->VertexProgram.TrackMatrix[i]==GL_MODELVIEW_PROJECTION_NV) {
	    /* XXX verify the combined matrix is up to date */
	    mat = &ctx->_ModelProjectMatrix;
	} else if (ctx->VertexProgram.TrackMatrix[i] >= GL_MATRIX0_NV &&
		   ctx->VertexProgram.TrackMatrix[i] <= GL_MATRIX7_NV) {
	    GLuint n = ctx->VertexProgram.TrackMatrix[i] - GL_MATRIX0_NV;
	    assert(n < MAX_PROGRAM_MATRICES);
	    mat = ctx->ProgramMatrixStack[n].Top;
	} else {
	    /* no matrix is tracked, but we leave the register values as-is */
	    assert(ctx->VertexProgram.TrackMatrix[i] == GL_NONE);
	    continue;
	}

	/* load the matrix values into sequential registers */
	if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_IDENTITY_NV) {
	    load_matrix(ctx->VertexProgram.Parameters, i*4, mat->m);
	} else if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_INVERSE_NV) {
	    mat->analyse(); /* update the inverse */
	    assert(!mat->is_dirty());
	    load_matrix(ctx->VertexProgram.Parameters, i*4, mat->inv);
	} else if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_TRANSPOSE_NV) {
	    load_transpose_matrix(ctx->VertexProgram.Parameters, i*4, mat->m);
	} else {
	    assert(ctx->VertexProgram.TrackMatrixTransform[i]
		   == GL_INVERSE_TRANSPOSE_NV);
	    mat->analyse(); /* update the inverse */
	    assert(!mat->is_dirty());
	    load_transpose_matrix(ctx->VertexProgram.Parameters, i*4, mat->inv);
	}
    }
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
