#ifndef BIND_SHADER_PARAMS_H
#define BIND_SHADER_PARAMS_H

/////////////////////////////  GPL LICENSE NOTICE  /////////////////////////////

//  crt-royale: A full-featured CRT shader, with cheese.
//  Copyright (C) 2014 TroggleMonkey <trogglemonkey@gmx.com>
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation; either version 2 of the License, or any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program; if not, write to the Free Software Foundation, Inc., 59 Temple
//  Place, Suite 330, Boston, MA 02111-1307 USA


/////////////////////////////  SETTINGS MANAGEMENT  ////////////////////////////


#include "CRT_Royale_derived-settings-and-constants.hlsli"

//  Override some parameters for gamma-management.h and tex2Dantialias.h:
#define OVERRIDE_DEVICE_GAMMA
static const float gba_gamma = 3.5; //  Irrelevant but necessary to define.
#define ANTIALIAS_OVERRIDE_BASICS
#define ANTIALIAS_OVERRIDE_PARAMETERS

//  Disable runtime shader params if the user doesn't explicitly want them.
//  Static constants will be defined in place of uniforms of the same name.
#ifndef RUNTIME_SHADER_PARAMS_ENABLE
#undef PARAMETER_UNIFORM
#endif


//  Provide accessors for vector constants that pack scalar uniforms:
float2 get_aspect_vector(const float geom_aspect_ratio) {
	//  Get an aspect ratio vector.  Enforce geom_max_aspect_ratio, and prevent
	//  the absolute scale from affecting the uv-mapping for curvature:
	const float geom_clamped_aspect_ratio =
		min(geom_aspect_ratio, geom_max_aspect_ratio);
	const float2 geom_aspect =
		normalize(float2(geom_clamped_aspect_ratio, 1.0));
	return geom_aspect;
}

float2 get_geom_overscan_vector() {
	return float2(geom_overscan_x, geom_overscan_y);
}

float2 get_geom_tilt_angle_vector() {
	return float2(geom_tilt_angle_x, geom_tilt_angle_y);
}

float3 get_convergence_offsets_x_vector() {
	return float3(convergence_offset_x_r, convergence_offset_x_g,
		convergence_offset_x_b);
}

float3 get_convergence_offsets_y_vector() {
	return float3(convergence_offset_y_r, convergence_offset_y_g,
		convergence_offset_y_b);
}

float2 get_convergence_offsets_r_vector() {
	return float2(convergence_offset_x_r, convergence_offset_y_r);
}

float2 get_convergence_offsets_g_vector() {
	return float2(convergence_offset_x_g, convergence_offset_y_g);
}

float2 get_convergence_offsets_b_vector() {
	return float2(convergence_offset_x_b, convergence_offset_y_b);
}

float2 get_aa_subpixel_r_offset() {
#ifdef RUNTIME_ANTIALIAS_WEIGHTS
#ifdef RUNTIME_ANTIALIAS_SUBPIXEL_OFFSETS
	//  WARNING: THIS IS EXTREMELY EXPENSIVE.
	return float2(aa_subpixel_r_offset_x_runtime,
		aa_subpixel_r_offset_y_runtime);
#else
	return aa_subpixel_r_offset_static;
#endif
#else
	return aa_subpixel_r_offset_static;
#endif
}

//  Provide accessors settings which still need "cooking:"
float get_mask_amplify() {
	static const float mask_grille_amplify = 1.0 / mask_grille_avg_color;
	static const float mask_slot_amplify = 1.0 / mask_slot_avg_color;
	static const float mask_shadow_amplify = 1.0 / mask_shadow_avg_color;
	return mask_type < 0.5 ? mask_grille_amplify :
		mask_type < 1.5 ? mask_slot_amplify :
		mask_shadow_amplify;
}

float get_mask_sample_mode() {
#ifdef RUNTIME_PHOSPHOR_MASK_MODE_TYPE_SELECT
#ifdef PHOSPHOR_MASK_MANUALLY_RESIZE
	return mask_sample_mode_desired;
#else
	return clamp(mask_sample_mode_desired, 1.0, 2.0);
#endif
#else
#ifdef PHOSPHOR_MASK_MANUALLY_RESIZE
	return mask_sample_mode_static;
#else
	return clamp(mask_sample_mode_static, 1.0, 2.0);
#endif
#endif
}


#endif  //  BIND_SHADER_PARAMS_H
