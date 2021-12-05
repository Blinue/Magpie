// CRT-Royale For Magpie
// 移植自 https://github.com/libretro/common-shaders/tree/master/crt/shaders/crt-royale
// 
// CRT-Royale 的许可声明：
// crt-royale: A full-featured CRT shader, with cheese.
// Copyright (C) 2014 TroggleMonkey <trogglemonkey@gmx.com>
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA 02111-1307 USA

//!MAGPIE EFFECT
//!VERSION 1


//!CONSTANT
//!VALUE INPUT_WIDTH
float inputWidth;

//!CONSTANT
//!VALUE INPUT_HEIGHT
float inputHeight;

//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!CONSTANT
//!VALUE OUTPUT_WIDTH
float outputWidth;

//!CONSTANT
//!VALUE OUTPUT_WIDTH
float outputHeight;

//!CONSTANT
//!VALUE 1 / SCALE_Y
float rcpScaleY;

//!CONSTANT
//!DEFAULT 2.5
//!MIN 1
//!MAX 5
float crt_gamma;

//!CONSTANT
//!DEFAULT 2.2
//!MIN 1
//!MAX 5
float lcd_gamma;

//!CONSTANT
//!DEFAULT 1
//!MIN 0
//!MAX 4
float levels_contrast;

//!CONSTANT
//!DEFAULT 0
//!MIN 0
//!MAX 1
float halation_weight;

//!CONSTANT
//!DEFAULT 0.075
//!MIN 0
//!MAX 1
float diffusion_weight;

//!CONSTANT
//!DEFAULT 0.8
//!MIN 0
//!MAX 5
float bloom_underestimate_levels;

//!CONSTANT
//!DEFAULT 0
//!MIN 0
//!MAX 1
float bloom_excess;

//!CONSTANT
//!DEFAULT 0.02
//!MIN 0.005
//!MAX 1
float beam_min_sigma;

//!CONSTANT
//!DEFAULT 0.3
//!MIN 0.005
//!MAX 1
float beam_max_sigma;

//!CONSTANT
//!DEFAULT 0.33
//!MIN 0.01
//!MAX 16
float beam_spot_power;

//!CONSTANT
//!DEFAULT 2
//!MIN 2
//!MAX 32
float beam_min_shape;

//!CONSTANT
//!DEFAULT 4
//!MIN 2
//!MAX 32
float beam_max_shape;

//!CONSTANT
//!DEFAULT 0.25
//!MIN 0.01
//!MAX 16
float beam_shape_power;

//!CONSTANT
//!DEFAULT 0
//!MIN 0
//!MAX 2
int beam_horiz_filter;

//!CONSTANT
//!DEFAULT 0.35
//!MIN 0
//!MAX 0.67
float beam_horiz_sigma;

//!CONSTANT
//!DEFAULT 1
//!MIN 0
//!MAX 1
float beam_horiz_linear_rgb_weight;

//!CONSTANT
//!DEFAULT 0
//!MIN -4
//!MAX 4
float convergence_offset_x_r;

//!CONSTANT
//!DEFAULT 0
//!MIN -4
//!MAX 4
float convergence_offset_x_g;

//!CONSTANT
//!DEFAULT 0
//!MIN -4
//!MAX 4
float convergence_offset_x_b;

//!CONSTANT
//!DEFAULT 0
//!MIN -2
//!MAX 2
float convergence_offset_y_r;

//!CONSTANT
//!DEFAULT 0
//!MIN -2
//!MAX 2
float convergence_offset_y_g;

//!CONSTANT
//!DEFAULT 0
//!MIN -2
//!MAX 2
float convergence_offset_y_b;

//!CONSTANT
//!DEFAULT 1
//!MIN 0
//!MAX 2
int mask_type;

//!CONSTANT
//!DEFAULT 0
//!MIN 0
//!MAX 2
int mask_sample_mode_desired;

//!CONSTANT
//!DEFAULT 0
//!MIN 0
//!MAX 1
int mask_specify_num_triads;

//!CONSTANT
//!DEFAULT 3
//!MIN 1
//!MAX 18
float mask_triad_size_desired;

//!CONSTANT
//!DEFAULT 480
//!MIN 342
//!MAX 1920
int mask_num_triads_desired;

//!CONSTANT
//!DEFAULT -0.333333333
//!MIN -0.333333333
//!MAX 0.333333333
float aa_subpixel_r_offset_x_runtime;

//!CONSTANT
//!DEFAULT 0
//!MIN -0.333333333
//!MAX 0.333333333
float aa_subpixel_r_offset_y_runtime;

//!CONSTANT
//!DEFAULT 0.5
//!MIN 0
//!MAX 4
float aa_cubic_c;

//!CONSTANT
//!DEFAULT 0.5
//!MIN 0.0625
//!MAX 1
float aa_gauss_sigma;

//!CONSTANT
//!DEFAULT 0
//!MIN 0
//!MAX 3
int geom_mode_runtime;

//!CONSTANT
//!DEFAULT 2
//!MIN 0.16
//!MAX 1024
float geom_radius;

//!CONSTANT
//!DEFAULT 2
//!MIN 0.5
//!MAX 1024
float geom_view_dist;

//!CONSTANT
//!DEFAULT 0
//!MIN -3.14159265
//!MAX 3.14159265
float geom_tilt_angle_x;

//!CONSTANT
//!DEFAULT 0
//!MIN -3.14159265
//!MAX 3.14159265
float geom_tilt_angle_y;

//!CONSTANT
//!DEFAULT 432
//!MIN 1
//!MAX 512
int geom_aspect_ratio_x;

//!CONSTANT
//!DEFAULT 329
//!MIN 1
//!MAX 512
int geom_aspect_ratio_y;

//!CONSTANT
//!DEFAULT 1
//!MIN 0.00390625
//!MAX 4
float geom_overscan_x;

//!CONSTANT
//!DEFAULT 1
//!MIN 0.00390625
//!MAX 4
float geom_overscan_y;

//!CONSTANT
//!DEFAULT 0.015
//!MIN 0.0000001
//!MAX 0.5
float border_size;

//!CONSTANT
//!DEFAULT 2
//!MIN 0
//!MAX 16
float border_darkness;

//!CONSTANT
//!DEFAULT 2.5
//!MIN 1
//!MAX 64
float border_compress;

//!CONSTANT
//!DEFAULT 0
//!MIN 0
//!MAX 1
int interlace_bff;

//!CONSTANT
//!DEFAULT 0
//!MIN 0
//!MAX 1
int interlace_1080i;


//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT B8G8R8A8_UNORM
Texture2D tex1;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT B8G8R8A8_UNORM
Texture2D tex2;

//!TEXTURE
//!WIDTH 320
//!HEIGHT 240
//!FORMAT B8G8R8A8_UNORM
Texture2D tex3;

//!TEXTURE
//!WIDTH 320
//!HEIGHT 240
//!FORMAT B8G8R8A8_UNORM
Texture2D tex4;

//!TEXTURE
//!WIDTH 320
//!HEIGHT 240
//!FORMAT B8G8R8A8_UNORM
Texture2D tex5;

//!TEXTURE
//!WIDTH 64
//!HEIGHT 0.0625 * OUTPUT_HEIGHT
//!FORMAT B8G8R8A8_UNORM
Texture2D tex6;

//!TEXTURE
//!WIDTH 0.0625 * OUTPUT_WIDTH
//!HEIGHT 0.0625 * OUTPUT_HEIGHT
//!FORMAT B8G8R8A8_UNORM
Texture2D tex7;

//!TEXTURE
//!WIDTH OUTPUT_WIDTH
//!HEIGHT OUTPUT_HEIGHT
//!FORMAT B8G8R8A8_UNORM
Texture2D tex8;

//!TEXTURE
//!SOURCE CRT_Royale_TileableLinearApertureGrille15Wide8And5d5SpacingResizeTo64.png
Texture2D mask_grille_texture_small;

//!TEXTURE
//!SOURCE CRT_Royale_TileableLinearSlotMaskTall15Wide9And4d5Horizontal9d14VerticalSpacingResizeTo64.png
Texture2D mask_slot_texture_small;

//!TEXTURE
//!SOURCE CRT_Royale_TileableLinearShadowMaskEDPResizeTo64.png
Texture2D mask_shadow_texture_small;


//!SAMPLER
//!FILTER POINT
SamplerState samPoint;

//!SAMPLER
//!FILTER LINEAR
SamplerState samLinear;


//!COMMON

#define frame_count 0

#define BLOOM_APPROX_WIDTH 320
#define BLOOM_APPROX_HEIGHT 240

/////////////////////////////  DRIVER CAPABILITIES  ////////////////////////////

//  The Cg compiler uses different "profiles" with different capabilities.
//  This shader requires a Cg compilation profile >= arbfp1, but a few options
//  require higher profiles like fp30 or fp40.  The shader can't detect profile
//  or driver capabilities, so instead you must comment or uncomment the lines
//  below with "//" before "#define."  Disable an option if you get compilation
//  errors resembling those listed.  Generally speaking, all of these options
//  will run on nVidia cards, but only DRIVERS_ALLOW_TEX2DBIAS (if that) is
//  likely to run on ATI/AMD, due to the Cg compiler's profile limitations.

//  Derivatives: Unsupported on fp20, ps_1_1, ps_1_2, ps_1_3, and arbfp1.
//  Among other things, derivatives help us fix anisotropic filtering artifacts
//  with curved manually tiled phosphor mask coords.  Related errors:
//  error C3004: function "float2 ddx(float2);" not supported in this profile
//  error C3004: function "float2 ddy(float2);" not supported in this profile
#define DRIVERS_ALLOW_DERIVATIVES

//  Fine derivatives: Unsupported on older ATI cards.
//  Fine derivatives enable 2x2 fragment block communication, letting us perform
//  fast single-pass blur operations.  If your card uses coarse derivatives and
//  these are enabled, blurs could look broken.  Derivatives are a prerequisite.
#ifdef DRIVERS_ALLOW_DERIVATIVES
#define DRIVERS_ALLOW_FINE_DERIVATIVES
#endif

//  Dynamic looping: Requires an fp30 or newer profile.
//  This makes phosphor mask resampling faster in some cases.  Related errors:
//  error C5013: profile does not support "for" statements and "for" could not
//  be unrolled
//#define DRIVERS_ALLOW_DYNAMIC_BRANCHES

//  Without DRIVERS_ALLOW_DYNAMIC_BRANCHES, we need to use unrollable loops.
//  Using one static loop avoids overhead if the user is right, but if the user
//  is wrong (loops are allowed), breaking a loop into if-blocked pieces with a
//  binary search can potentially save some iterations.  However, it may fail:
//  error C6001: Temporary register limit of 32 exceeded; 35 registers
//  needed to compile program
#define ACCOMODATE_POSSIBLE_DYNAMIC_LOOPS

//  tex2Dlod: Requires an fp40 or newer profile.  This can be used to disable
//  anisotropic filtering, thereby fixing related artifacts.  Related errors:
//  error C3004: function "float4 tex2Dlod(sampler2D, float4);" not supported in
//  this profile
#define DRIVERS_ALLOW_TEX2DLOD

//  tex2Dbias: Requires an fp30 or newer profile.  This can be used to alleviate
//  artifacts from anisotropic filtering and mipmapping.  Related errors:
//  error C3004: function "float4 tex2Dbias(sampler2D, float4);" not supported
//  in this profile
#define DRIVERS_ALLOW_TEX2DBIAS

//  Integrated graphics compatibility: Integrated graphics like Intel HD 4000
//  impose stricter limitations on register counts and instructions.  Enable
//  INTEGRATED_GRAPHICS_COMPATIBILITY_MODE if you still see error C6001 or:
//  error C6002: Instruction limit of 1024 exceeded: 1523 instructions needed
//  to compile program.
//  Enabling integrated graphics compatibility mode will automatically disable:
//  1.) PHOSPHOR_MASK_MANUALLY_RESIZE: The phosphor mask will be softer.
//      (This may be reenabled in a later release.)
//  2.) RUNTIME_GEOMETRY_MODE
//  3.) The high-quality 4x4 Gaussian resize for the bloom approximation
    //#define INTEGRATED_GRAPHICS_COMPATIBILITY_MODE


////////////////////////////  USER CODEPATH OPTIONS  ///////////////////////////

//  To disable a #define option, turn its line into a comment with "//."

//  RUNTIME VS. COMPILE-TIME OPTIONS (Major Performance Implications):
//  Enable runtime shader parameters in the Retroarch (etc.) GUI?  They override
//  many of the options in this file and allow real-time tuning, but many of
//  them are slower.  Disabling them and using this text file will boost FPS.
#define RUNTIME_SHADER_PARAMS_ENABLE
//  Specify the phosphor bloom sigma at runtime?  This option is 10% slower, but
//  it's the only way to do a wide-enough full bloom with a runtime dot pitch.
#define RUNTIME_PHOSPHOR_BLOOM_SIGMA
//  Specify antialiasing weight parameters at runtime?  (Costs ~20% with cubics)
#define RUNTIME_ANTIALIAS_WEIGHTS
//  Specify subpixel offsets at runtime? (WARNING: EXTREMELY EXPENSIVE!)
//#define RUNTIME_ANTIALIAS_SUBPIXEL_OFFSETS
//  Make beam_horiz_filter and beam_horiz_linear_rgb_weight into runtime shader
//  parameters?  This will require more math or dynamic branching.
#define RUNTIME_SCANLINES_HORIZ_FILTER_COLORSPACE
//  Specify the tilt at runtime?  This makes things about 3% slower.
#define RUNTIME_GEOMETRY_TILT
//  Specify the geometry mode at runtime?
#define RUNTIME_GEOMETRY_MODE
//  Specify the phosphor mask type (aperture grille, slot mask, shadow mask) and
//  mode (Lanczos-resize, hardware resize, or tile 1:1) at runtime, even without
//  dynamic branches?  This is cheap if mask_resize_viewport_scale is small.
#define FORCE_RUNTIME_PHOSPHOR_MASK_MODE_TYPE_SELECT

//  PHOSPHOR MASK:
//  Manually resize the phosphor mask for best results (slower)?  Disabling this
//  removes the option to do so, but it may be faster without dynamic branches.
#define PHOSPHOR_MASK_MANUALLY_RESIZE
//  If we sinc-resize the mask, should we Lanczos-window it (slower but better)?
#define PHOSPHOR_MASK_RESIZE_LANCZOS_WINDOW
//  Larger blurs are expensive, but we need them to blur larger triads.  We can
//  detect the right blur if the triad size is static or our profile allows
//  dynamic branches, but otherwise we use the largest blur the user indicates
//  they might need:
#define PHOSPHOR_BLOOM_TRIADS_LARGER_THAN_3_PIXELS
//#define PHOSPHOR_BLOOM_TRIADS_LARGER_THAN_6_PIXELS
//#define PHOSPHOR_BLOOM_TRIADS_LARGER_THAN_9_PIXELS
//#define PHOSPHOR_BLOOM_TRIADS_LARGER_THAN_12_PIXELS
//  Here's a helpful chart:
//  MaxTriadSize    BlurSize    MinTriadCountsByResolution
//  3.0             9.0         480/640/960/1920 triads at 1080p/1440p/2160p/4320p, 4:3 aspect
//  6.0             17.0        240/320/480/960 triads at 1080p/1440p/2160p/4320p, 4:3 aspect
//  9.0             25.0        160/213/320/640 triads at 1080p/1440p/2160p/4320p, 4:3 aspect
//  12.0            31.0        120/160/240/480 triads at 1080p/1440p/2160p/4320p, 4:3 aspect
//  18.0            43.0        80/107/160/320 triads at 1080p/1440p/2160p/4320p, 4:3 aspect


///////////////////////////////  USER PARAMETERS  //////////////////////////////

//  Note: Many of these static parameters are overridden by runtime shader
//  parameters when those are enabled.  However, many others are static codepath
//  options that were cleaner or more convert to code as static constants.

//  GAMMA:
static const float crt_gamma_static = 2.5;                  //  range [1, 5]
static const float lcd_gamma_static = 2.2;                  //  range [1, 5]

//  LEVELS MANAGEMENT:
    //  Control the final multiplicative image contrast:
static const float levels_contrast_static = 1.0;            //  range [0, 4)
//  We auto-dim to avoid clipping between passes and restore brightness
//  later.  Control the dim factor here: Lower values clip less but crush
//  blacks more (static only for now).
static const float levels_autodim_temp = 0.5;               //  range (0, 1]

//  HALATION/DIFFUSION/BLOOM:
    //  Halation weight: How much energy should be lost to electrons bounding
    //  around under the CRT glass and exciting random phosphors?
static const float halation_weight_static = 0.0;            //  range [0, 1]
//  Refractive diffusion weight: How much light should spread/diffuse from
//  refracting through the CRT glass?
static const float diffusion_weight_static = 0.075;         //  range [0, 1]
//  Underestimate brightness: Bright areas bloom more, but we can base the
//  bloom brightpass on a lower brightness to sharpen phosphors, or a higher
//  brightness to soften them.  Low values clip, but >= 0.8 looks okay.
static const float bloom_underestimate_levels_static = 0.8; //  range [0, 5]
//  Blur all colors more than necessary for a softer phosphor bloom?
static const float bloom_excess_static = 0.0;               //  range [0, 1]
//  The BLOOM_APPROX pass approximates a phosphor blur early on with a small
//  blurred resize of the input (convergence offsets are applied as well).
//  There are three filter options (static option only for now):
//  0.) Bilinear resize: A fast, close approximation to a 4x4 resize
//      if min_allowed_viewport_triads and the BLOOM_APPROX resolution are sane
//      and beam_max_sigma is low.
//  1.) 3x3 resize blur: Medium speed, soft/smeared from bilinear blurring,
//      always uses a static sigma regardless of beam_max_sigma or
//      mask_num_triads_desired.
//  2.) True 4x4 Gaussian resize: Slowest, technically correct.
//  These options are more pronounced for the fast, unbloomed shader version.
static const float bloom_approx_filter_static = 2.0;

//  ELECTRON BEAM SCANLINE DISTRIBUTION:
    //  How many scanlines should contribute light to each pixel?  Using more
    //  scanlines is slower (especially for a generalized Gaussian) but less
    //  distorted with larger beam sigmas (especially for a pure Gaussian).  The
    //  max_beam_sigma at which the closest unused weight is guaranteed <
    //  1.0/255.0 (for a 3x antialiased pure Gaussian) is:
    //      2 scanlines: max_beam_sigma = 0.2089; distortions begin ~0.34; 141.7 FPS pure, 131.9 FPS generalized
    //      3 scanlines, max_beam_sigma = 0.3879; distortions begin ~0.52; 137.5 FPS pure; 123.8 FPS generalized
    //      4 scanlines, max_beam_sigma = 0.5723; distortions begin ~0.70; 134.7 FPS pure; 117.2 FPS generalized
    //      5 scanlines, max_beam_sigma = 0.7591; distortions begin ~0.89; 131.6 FPS pure; 112.1 FPS generalized
    //      6 scanlines, max_beam_sigma = 0.9483; distortions begin ~1.08; 127.9 FPS pure; 105.6 FPS generalized
static const float beam_num_scanlines = 3.0;                //  range [2, 6]
//  A generalized Gaussian beam varies shape with color too, now just width.
//  It's slower but more flexible (static option only for now).
static const bool beam_generalized_gaussian = true;
//  What kind of scanline antialiasing do you want?
//  0: Sample weights at 1x; 1: Sample weights at 3x; 2: Compute an integral
//  Integrals are slow (especially for generalized Gaussians) and rarely any
//  better than 3x antialiasing (static option only for now).
static const float beam_antialias_level = 1.0;              //  range [0, 2]
//  Min/max standard deviations for scanline beams: Higher values widen and
//  soften scanlines.  Depending on other options, low min sigmas can alias.
static const float beam_min_sigma_static = 0.02;            //  range (0, 1]
static const float beam_max_sigma_static = 0.3;             //  range (0, 1]
//  Beam width varies as a function of color: A power function (0) is more
//  configurable, but a spherical function (1) gives the widest beam
//  variability without aliasing (static option only for now).
static const float beam_spot_shape_function = 0.0;
//  Spot shape power: Powers <= 1 give smoother spot shapes but lower
//  sharpness.  Powers >= 1.0 are awful unless mix/max sigmas are close.
static const float beam_spot_power_static = 1.0 / 3.0;    //  range (0, 16]
//  Generalized Gaussian max shape parameters: Higher values give flatter
//  scanline plateaus and steeper dropoffs, simultaneously widening and
//  sharpening scanlines at the cost of aliasing.  2.0 is pure Gaussian, and
//  values > ~40.0 cause artifacts with integrals.
static const float beam_min_shape_static = 2.0;         //  range [2, 32]
static const float beam_max_shape_static = 4.0;         //  range [2, 32]
//  Generalized Gaussian shape power: Affects how quickly the distribution
//  changes shape from Gaussian to steep/plateaued as color increases from 0
//  to 1.0.  Higher powers appear softer for most colors, and lower powers
//  appear sharper for most colors.
static const float beam_shape_power_static = 1.0 / 4.0;   //  range (0, 16]
//  What filter should be used to sample scanlines horizontally?
//  0: Quilez (fast), 1: Gaussian (configurable), 2: Lanczos2 (sharp)
static const float beam_horiz_filter_static = 0.0;
//  Standard deviation for horizontal Gaussian resampling:
static const float beam_horiz_sigma_static = 0.35;      //  range (0, 2/3]
//  Do horizontal scanline sampling in linear RGB (correct light mixing),
//  gamma-encoded RGB (darker, hard spot shape, may better match bandwidth-
//  limiting circuitry in some CRT's), or a weighted avg.?
static const float beam_horiz_linear_rgb_weight_static = 1.0;   //  range [0, 1]
//  Simulate scanline misconvergence?  This needs 3x horizontal texture
//  samples and 3x texture samples of BLOOM_APPROX and HALATION_BLUR in
//  later passes (static option only for now).
static const bool beam_misconvergence = true;
//  Convergence offsets in x/y directions for R/G/B scanline beams in units
//  of scanlines.  Positive offsets go right/down; ranges [-2, 2]
static const float2 convergence_offsets_r_static = float2(0.1, 0.2);
static const float2 convergence_offsets_g_static = float2(0.3, 0.4);
static const float2 convergence_offsets_b_static = float2(0.5, 0.6);
//  Detect interlacing (static option only for now)?
static const bool interlace_detect = true;
//  Assume 1080-line sources are interlaced?
static const bool interlace_1080i_static = false;
//  For interlaced sources, assume TFF (top-field first) or BFF order?
//  (Whether this matters depends on the nature of the interlaced input.)
static const bool interlace_bff_static = false;

//  ANTIALIASING:
    //  What AA level do you want for curvature/overscan/subpixels?  Options:
    //  0x (none), 1x (sample subpixels), 4x, 5x, 6x, 7x, 8x, 12x, 16x, 20x, 24x
    //  (Static option only for now)
static const float aa_level = 12.0;                     //  range [0, 24]
//  What antialiasing filter do you want (static option only)?  Options:
//  0: Box (separable), 1: Box (cylindrical),
//  2: Tent (separable), 3: Tent (cylindrical),
//  4: Gaussian (separable), 5: Gaussian (cylindrical),
//  6: Cubic* (separable), 7: Cubic* (cylindrical, poor)
//  8: Lanczos Sinc (separable), 9: Lanczos Jinc (cylindrical, poor)
//      * = Especially slow with RUNTIME_ANTIALIAS_WEIGHTS
static const float aa_filter = 6.0;                     //  range [0, 9]
//  Flip the sample grid on odd/even frames (static option only for now)?
static const bool aa_temporal = false;
//  Use RGB subpixel offsets for antialiasing?  The pixel is at green, and
//  the blue offset is the negative r offset; range [0, 0.5]
static const float2 aa_subpixel_r_offset_static = float2(-1.0 / 3.0, 0.0);//float2(0.0);
//  Cubics: See http://www.imagemagick.org/Usage/filter/#mitchell
//  1.) "Keys cubics" with B = 1 - 2C are considered the highest quality.
//  2.) C = 0.5 (default) is Catmull-Rom; higher C's apply sharpening.
//  3.) C = 1.0/3.0 is the Mitchell-Netravali filter.
//  4.) C = 0.0 is a soft spline filter.
static const float aa_cubic_c_static = 0.5;             //  range [0, 4]
//  Standard deviation for Gaussian antialiasing: Try 0.5/aa_pixel_diameter.
static const float aa_gauss_sigma_static = 0.5;     //  range [0.0625, 1.0]

//  PHOSPHOR MASK:
    //  Mask type: 0 = aperture grille, 1 = slot mask, 2 = EDP shadow mask
static const float mask_type_static = 1.0;                  //  range [0, 2]
//  We can sample the mask three ways.  Pick 2/3 from: Pretty/Fast/Flexible.
//  0.) Sinc-resize to the desired dot pitch manually (pretty/slow/flexible).
//      This requires PHOSPHOR_MASK_MANUALLY_RESIZE to be #defined.
//  1.) Hardware-resize to the desired dot pitch (ugly/fast/flexible).  This
//      is halfway decent with LUT mipmapping but atrocious without it.
//  2.) Tile it without resizing at a 1:1 texel:pixel ratio for flat coords
//      (pretty/fast/inflexible).  Each input LUT has a fixed dot pitch.
//      This mode reuses the same masks, so triads will be enormous unless
//      you change the mask LUT filenames in your .cgp file.
static const float mask_sample_mode_static = 0.0;           //  range [0, 2]
//  Prefer setting the triad size (0.0) or number on the screen (1.0)?
//  If RUNTIME_PHOSPHOR_BLOOM_SIGMA isn't #defined, the specified triad size
//  will always be used to calculate the full bloom sigma statically.
static const float mask_specify_num_triads_static = 0.0;    //  range [0, 1]
//  Specify the phosphor triad size, in pixels.  Each tile (usually with 8
//  triads) will be rounded to the nearest integer tile size and clamped to
//  obey minimum size constraints (imposed to reduce downsize taps) and
//  maximum size constraints (imposed to have a sane MASK_RESIZE FBO size).
//  To increase the size limit, double the viewport-relative scales for the
//  two MASK_RESIZE passes in crt-royale.cgp and user-cgp-contants.h.
//      range [1, mask_texture_small_size/mask_triads_per_tile]
static const float mask_triad_size_desired_static = 24.0 / 8.0;
//  If mask_specify_num_triads is 1.0/true, we'll go by this instead (the
//  final size will be rounded and constrained as above); default 480.0
static const float mask_num_triads_desired_static = 480.0;
//  How many lobes should the sinc/Lanczos resizer use?  More lobes require
//  more samples and avoid moire a bit better, but some is unavoidable
//  depending on the destination size (static option for now).
static const float mask_sinc_lobes = 3.0;                   //  range [2, 4]
//  The mask is resized using a variable number of taps in each dimension,
//  but some Cg profiles always fetch a constant number of taps no matter
//  what (no dynamic branching).  We can limit the maximum number of taps if
//  we statically limit the minimum phosphor triad size.  Larger values are
//  faster, but the limit IS enforced (static option only, forever);
//      range [1, mask_texture_small_size/mask_triads_per_tile]
//  TODO: Make this 1.0 and compensate with smarter sampling!
static const float mask_min_allowed_triad_size = 2.0;

//  GEOMETRY:
    //  Geometry mode:
    //  0: Off (default), 1: Spherical mapping (like cgwg's),
    //  2: Alt. spherical mapping (more bulbous), 3: Cylindrical/Trinitron
static const float geom_mode_static = 0.0;      //  range [0, 3]
//  Radius of curvature: Measured in units of your viewport's diagonal size.
static const float geom_radius_static = 2.0;    //  range [1/(2*pi), 1024]
//  View dist is the distance from the player to their physical screen, in
//  units of the viewport's diagonal size.  It controls the field of view.
static const float geom_view_dist_static = 2.0; //  range [0.5, 1024]
//  Tilt angle in radians (clockwise around up and right vectors):
static const float2 geom_tilt_angle_static = float2(0.0, 0.0);  //  range [-pi, pi]
//  Aspect ratio: When the true viewport size is unknown, this value is used
//  to help convert between the phosphor triad size and count, along with
//  the mask_resize_viewport_scale constant from user-cgp-constants.h.  Set
//  this equal to Retroarch's display aspect ratio (DAR) for best results;
//  range [1, geom_max_aspect_ratio from user-cgp-constants.h];
//  default (256/224)*(54/47) = 1.313069909 (see below)
static const float geom_aspect_ratio_static = 1.313069909;
//  Before getting into overscan, here's some general aspect ratio info:
//  - DAR = display aspect ratio = SAR * PAR; as in your Retroarch setting
//  - SAR = storage aspect ratio = DAR / PAR; square pixel emulator frame AR
//  - PAR = pixel aspect ratio   = DAR / SAR; holds regardless of cropping
//  Geometry processing has to "undo" the screen-space 2D DAR to calculate
//  3D view vectors, then reapplies the aspect ratio to the simulated CRT in
//  uv-space.  To ensure the source SAR is intended for a ~4:3 DAR, either:
//  a.) Enable Retroarch's "Crop Overscan"
//  b.) Readd horizontal padding: Set overscan to e.g. N*(1.0, 240.0/224.0)
//  Real consoles use horizontal black padding in the signal, but emulators
//  often crop this without cropping the vertical padding; a 256x224 [S]NES
//  frame (8:7 SAR) is intended for a ~4:3 DAR, but a 256x240 frame is not.
//  The correct [S]NES PAR is 54:47, found by blargg and NewRisingSun:
//      http://board.zsnes.com/phpBB3/viewtopic.php?f=22&t=11928&start=50
//      http://forums.nesdev.com/viewtopic.php?p=24815#p24815
//  For flat output, it's okay to set DAR = [existing] SAR * [correct] PAR
//  without doing a. or b., but horizontal image borders will be tighter
//  than vertical ones, messing up curvature and overscan.  Fixing the
//  padding first corrects this.
//  Overscan: Amount to "zoom in" before cropping.  You can zoom uniformly
//  or adjust x/y independently to e.g. readd horizontal padding, as noted
//  above: Values < 1.0 zoom out; range (0, inf)
static const float2 geom_overscan_static = float2(1.0, 1.0);// * 1.005 * (1.0, 240/224.0)
//  Compute a proper pixel-space to texture-space matrix even without ddx()/
//  ddy()?  This is ~8.5% slower but improves antialiasing/subpixel filtering
//  with strong curvature (static option only for now).
static const bool geom_force_correct_tangent_matrix = true;

//  BORDERS:
    //  Rounded border size in texture uv coords:
static const float border_size_static = 0.015;           //  range [0, 0.5]
//  Border darkness: Moderate values darken the border smoothly, and high
//  values make the image very dark just inside the border:
static const float border_darkness_static = 2.0;        //  range [0, inf)
//  Border compression: High numbers compress border transitions, narrowing
//  the dark border area.
static const float border_compress_static = 2.5;        //  range [1, inf)


//!PASS 1
//!BIND INPUT
//!SAVE tex1

// 移植自 https://github.com/libretro/common-shaders/blob/master/crt/shaders/crt-royale/src/crt-royale-first-pass-linearize-crt-gamma-bob-fields.cg


//  PASS SETTINGS:
//  gamma-management.h needs to know what kind of pipeline we're using and
//  what pass this is in that pipeline.  This will become obsolete if/when we
//  can #define things like this in the .cgp preset file.
#define FIRST_PASS
#define SIMULATE_CRT_ON_LCD

#include "CRT_Royale_bind-shader-params.hlsli"
#include "CRT_Royale_gamma-management.hlsli"
#include "CRT_Royale_scanline-functions.hlsli"


float4 Pass1(float2 pos) {
    float interlaced = is_interlaced(inputHeight);

    //  Linearize the input based on CRT gamma and bob interlaced fields.
    //  Bobbing ensures we can immediately blur without getting artifacts.
    //  Note: TFF/BFF won't matter for sources that double-weave or similar.
    if (interlace_detect) {
        //  Sample the current line and an average of the previous/next line;
        //  tex2D_linearize will decode CRT gamma.  Don't bother branching:
        const float2 v_step = float2(0, inputPtY);
        const float3 curr_line = tex2D_linearize(
            INPUT, samPoint, pos).rgb;
        const float3 last_line = tex2D_linearize(
            INPUT, samPoint, pos - v_step).rgb;
        const float3 next_line = tex2D_linearize(
            INPUT, samPoint, pos + v_step).rgb;
        const float3 interpolated_line = 0.5 * (last_line + next_line);
        //  If we're interlacing, determine which field curr_line is in:
        const float modulus = interlaced + 1.0;
        const float field_offset =
            fmod(frame_count + float(interlace_bff), modulus);
        const float curr_line_texel = pos.y * inputHeight;
        //  Use under_half to fix a rounding bug around exact texel locations.
        const float line_num_last = floor(curr_line_texel - under_half);
        const float wrong_field = fmod(line_num_last + field_offset, modulus);
        //  Select the correct color, and output the result:
        const float3 color = lerp(curr_line, interpolated_line, wrong_field);
        return encode_output(float4(color, 1.0));
    } else {
        return encode_output(tex2D_linearize(INPUT, samPoint, pos));
    }
}

//!PASS 2
//!BIND tex1
//!SAVE tex2

// 移植自 https://github.com/libretro/common-shaders/blob/master/crt/shaders/crt-royale/src/crt-royale-scanlines-vertical-interlacing.cg

#include "CRT_Royale_bind-shader-params.hlsli"
#include "CRT_Royale_scanline-functions.hlsli"
#include "CRT_Royale_gamma-management.hlsli"


float4 Pass2(float2 pos) {
    const float y_step = 1.0 + float(is_interlaced(inputHeight));
    const float2 il_step_multiple = { 1.0, y_step };
    //  Get the uv tex coords step between one texel (x) and scanline (y):
    const float2 uv_step = il_step_multiple / float2(inputWidth, inputHeight);
    //  We need the pixel height in scanlines for antialiased/integral sampling:
    const float ph = rcpScaleY / il_step_multiple.y;

    //  This pass: Sample multiple (misconverged?) scanlines to the final
    //  vertical resolution.  Temporarily auto-dim the output to avoid clipping.

    //  Read some attributes into local variables:
    const float2 texture_size = { inputWidth, inputHeight };
    const float2 texture_size_inv = { inputPtX, inputPtY };

    //  Get the uv coords of the previous scanline (in this field), and the
    //  scanline's distance from this sample, in scanlines.
    float dist;
    const float2 scanline_uv = get_last_scanline_uv(pos, texture_size,
        texture_size_inv, il_step_multiple, frame_count, dist);
    //  Consider 2, 3, 4, or 6 scanlines numbered 0-5: The previous and next
    //  scanlines are numbered 2 and 3.  Get scanline colors colors (ignore
    //  horizontal sampling, since since IN.output_size.x = video_size.x).
    //  NOTE: Anisotropic filtering creates interlacing artifacts, which is why
    //  ORIG_LINEARIZED bobbed any interlaced input before this pass.
    const float2 v_step = float2(0.0, uv_step.y);
    const float3 scanline2_color = tex2D_linearize(tex1, samLinear, scanline_uv).rgb;
    const float3 scanline3_color =
        tex2D_linearize(tex1, samLinear, scanline_uv + v_step).rgb;
    float3 scanline0_color, scanline1_color, scanline4_color, scanline5_color,
        scanline_outside_color;
    float dist_round;
    //  Use scanlines 0, 1, 4, and 5 for a total of 6 scanlines:
    if (beam_num_scanlines > 5.5) {
        scanline1_color =
            tex2D_linearize(tex1, samLinear, scanline_uv - v_step).rgb;
        scanline4_color =
            tex2D_linearize(tex1, samLinear, scanline_uv + 2.0 * v_step).rgb;
        scanline0_color =
            tex2D_linearize(tex1, samLinear, scanline_uv - 2.0 * v_step).rgb;
        scanline5_color =
            tex2D_linearize(tex1, samLinear, scanline_uv + 3.0 * v_step).rgb;
    }
    //  Use scanlines 1, 4, and either 0 or 5 for a total of 5 scanlines:
    else if (beam_num_scanlines > 4.5) {
        scanline1_color =
            tex2D_linearize(tex1, samLinear, scanline_uv - v_step).rgb;
        scanline4_color =
            tex2D_linearize(tex1, samLinear, scanline_uv + 2.0 * v_step).rgb;
        //  dist is in [0, 1]
        dist_round = round(dist);
        const float2 sample_0_or_5_uv_off =
            lerp(-2.0 * v_step, 3.0 * v_step, dist_round);
        //  Call this "scanline_outside_color" to cope with the conditional
        //  scanline number:
        scanline_outside_color = tex2D_linearize(
            tex1, samLinear, scanline_uv + sample_0_or_5_uv_off).rgb;
    }
    //  Use scanlines 1 and 4 for a total of 4 scanlines:
    else if (beam_num_scanlines > 3.5) {
        scanline1_color =
            tex2D_linearize(tex1, samLinear, scanline_uv - v_step).rgb;
        scanline4_color =
            tex2D_linearize(tex1, samLinear, scanline_uv + 2.0 * v_step).rgb;
    }
    //  Use scanline 1 or 4 for a total of 3 scanlines:
    else if (beam_num_scanlines > 2.5) {
        //  dist is in [0, 1]
        dist_round = round(dist);
        const float2 sample_1or4_uv_off =
            lerp(-v_step, 2.0 * v_step, dist_round);
        scanline_outside_color = tex2D_linearize(
            tex1, samLinear, scanline_uv + sample_1or4_uv_off).rgb;
    }

    //  Compute scanline contributions, accounting for vertical convergence.
    //  Vertical convergence offsets are in units of current-field scanlines.
    //  dist2 means "positive sample distance from scanline 2, in scanlines:"
    float3 dist2 = dist;
    if (beam_misconvergence) {
        const float3 convergence_offsets_vert_rgb =
            get_convergence_offsets_y_vector();
        dist2 = dist-convergence_offsets_vert_rgb;
    }
    //  Calculate {sigma, shape}_range outside of scanline_contrib so it's only
    //  done once per pixel (not 6 times) with runtime params.  Don't reuse the
    //  vertex shader calculations, so static versions can be constant-folded.
    const float sigma_range = max(beam_max_sigma, beam_min_sigma) -
        beam_min_sigma;
    const float shape_range = max(beam_max_shape, beam_min_shape) -
        beam_min_shape;
    //  Calculate and sum final scanline contributions, starting with lines 2/3.
    //  There is no normalization step, because we're not interpolating a
    //  continuous signal.  Instead, each scanline is an additive light source.
    const float3 scanline2_contrib = scanline_contrib(dist2,
        scanline2_color, ph, sigma_range, shape_range);
    const float3 scanline3_contrib = scanline_contrib(abs(1.0 - dist2),
        scanline3_color, ph, sigma_range, shape_range);
    float3 scanline_intensity = scanline2_contrib + scanline3_contrib;
    if (beam_num_scanlines > 5.5) {
        const float3 scanline0_contrib =
            scanline_contrib(dist2 + 2.0, scanline0_color,
                ph, sigma_range, shape_range);
        const float3 scanline1_contrib =
            scanline_contrib(dist2 + 1.0, scanline1_color,
                ph, sigma_range, shape_range);
        const float3 scanline4_contrib =
            scanline_contrib(abs(2.0 - dist2), scanline4_color,
                ph, sigma_range, shape_range);
        const float3 scanline5_contrib =
            scanline_contrib(abs(3.0 - dist2), scanline5_color,
                ph, sigma_range, shape_range);
        scanline_intensity += scanline0_contrib + scanline1_contrib +
            scanline4_contrib + scanline5_contrib;
    } else if (beam_num_scanlines > 4.5) {
        const float3 scanline1_contrib =
            scanline_contrib(dist2 + 1.0, scanline1_color,
                ph, sigma_range, shape_range);
        const float3 scanline4_contrib =
            scanline_contrib(abs(2.0 - dist2), scanline4_color,
                ph, sigma_range, shape_range);
        const float3 dist0or5 = lerp(
            dist2 + 2.0, 3.0 - dist2, dist_round);
        const float3 scanline0or5_contrib = scanline_contrib(
            dist0or5, scanline_outside_color, ph, sigma_range, shape_range);
        scanline_intensity += scanline1_contrib + scanline4_contrib +
            scanline0or5_contrib;
    } else if (beam_num_scanlines > 3.5) {
        const float3 scanline1_contrib =
            scanline_contrib(dist2 + 1.0, scanline1_color,
                ph, sigma_range, shape_range);
        const float3 scanline4_contrib =
            scanline_contrib(abs(2.0 - dist2), scanline4_color,
                ph, sigma_range, shape_range);
        scanline_intensity += scanline1_contrib + scanline4_contrib;
    } else if (beam_num_scanlines > 2.5) {
        const float3 dist1or4 = lerp(
            dist2 + 1.0, 2.0 - dist2, dist_round);
        const float3 scanline1or4_contrib = scanline_contrib(
            dist1or4, scanline_outside_color, ph, sigma_range, shape_range);
        scanline_intensity += scanline1or4_contrib;
    }

    //  Auto-dim the image to avoid clipping, encode if necessary, and output.
    //  My original idea was to compute a minimal auto-dim factor and put it in
    //  the alpha channel, but it wasn't working, at least not reliably.  This
    //  is faster anyway, levels_autodim_temp = 0.5 isn't causing banding.
    return encode_output(float4(scanline_intensity * levels_autodim_temp, 1.0));
}

//!PASS 3
//!BIND tex2
//!SAVE tex3

// 移植自 https://github.com/libretro/common-shaders/blob/master/crt/shaders/crt-royale/src/crt-royale-bloom-approx.cg

#include "CRT_Royale_bind-shader-params.hlsli"
#include "CRT_Royale_scanline-functions.hlsli"
#include "CRT_Royale_gamma-management.hlsli"
#include "CRT_Royale_blur-functions.hlsli"
#include "CRT_Royale_bloom-functions.hlsli"


float3 tex2Dresize_gaussian4x4(Texture2D tex, SamplerState sam, const float2 tex_uv,
    const float2 dxdy, const float2 texture_size, const float2 texture_size_inv,
    const float2 tex_uv_to_pixel_scale, const float sigma) {
    //  Requires:   1.) All requirements of gamma-management.h must be satisfied!
    //              2.) filter_linearN must == "true" in your .cgp preset.
    //              3.) mipmap_inputN must == "true" in your .cgp preset if
    //                  IN.output_size << SRC.video_size.
    //              4.) dxdy should contain the uv pixel spacing:
    //                      dxdy = max(float2(1.0),
    //                          SRC.video_size/IN.output_size)/SRC.texture_size;
    //              5.) texture_size == SRC.texture_size
    //              6.) texture_size_inv == float2(1.0)/SRC.texture_size
    //              7.) tex_uv_to_pixel_scale == IN.output_size *
    //                      SRC.texture_size / SRC.video_size;
    //              8.) sigma is the desired Gaussian standard deviation, in
    //                  terms of output pixels.  It should be < ~0.66171875 to
    //                  ensure the first unused sample (outside the 4x4 box) has
    //                  a weight < 1.0/256.0.
    //  Returns:    A true 4x4 Gaussian resize of the input.
    //  Description:
    //  Given correct inputs, this Gaussian resizer samples 4 pixel locations
    //  along each downsized dimension and/or 4 texel locations along each
    //  upsized dimension.  It computes dynamic weights based on the pixel-space
    //  distance of each sample from the destination pixel.  It is arbitrarily
    //  resizable and higher quality than tex2Dblur3x3_resize, but it's slower.
    //  TODO: Move this to a more suitable file once there are others like it.
    const float denom_inv = 0.5 / (sigma * sigma);
    //  We're taking 4x4 samples, and we're snapping to texels for upsizing.
    //  Find texture coords for sample 5 (second row, second column):
    const float2 curr_texel = tex_uv * texture_size;
    const float2 prev_texel =
        floor(curr_texel - under_half) + 0.5;
    const float2 prev_texel_uv = prev_texel * texture_size_inv;
    const float2 snap = float2(dxdy <= texture_size_inv);
    const float2 sample5_downsize_uv = tex_uv - 0.5 * dxdy;
    const float2 sample5_uv = lerp(sample5_downsize_uv, prev_texel_uv, snap);
    //  Compute texture coords for other samples:
    const float2 dx = float2(dxdy.x, 0.0);
    const float2 sample0_uv = sample5_uv - dxdy;
    const float2 sample10_uv = sample5_uv + dxdy;
    const float2 sample15_uv = sample5_uv + 2.0 * dxdy;
    const float2 sample1_uv = sample0_uv + dx;
    const float2 sample2_uv = sample0_uv + 2.0 * dx;
    const float2 sample3_uv = sample0_uv + 3.0 * dx;
    const float2 sample4_uv = sample5_uv - dx;
    const float2 sample6_uv = sample5_uv + dx;
    const float2 sample7_uv = sample5_uv + 2.0 * dx;
    const float2 sample8_uv = sample10_uv - 2.0 * dx;
    const float2 sample9_uv = sample10_uv - dx;
    const float2 sample11_uv = sample10_uv + dx;
    const float2 sample12_uv = sample15_uv - 3.0 * dx;
    const float2 sample13_uv = sample15_uv - 2.0 * dx;
    const float2 sample14_uv = sample15_uv - dx;
    //  Load each sample:
    const float3 sample0 = tex2D_linearize(tex, sam, sample0_uv).rgb;
    const float3 sample1 = tex2D_linearize(tex, sam, sample1_uv).rgb;
    const float3 sample2 = tex2D_linearize(tex, sam, sample2_uv).rgb;
    const float3 sample3 = tex2D_linearize(tex, sam, sample3_uv).rgb;
    const float3 sample4 = tex2D_linearize(tex, sam, sample4_uv).rgb;
    const float3 sample5 = tex2D_linearize(tex, sam, sample5_uv).rgb;
    const float3 sample6 = tex2D_linearize(tex, sam, sample6_uv).rgb;
    const float3 sample7 = tex2D_linearize(tex, sam, sample7_uv).rgb;
    const float3 sample8 = tex2D_linearize(tex, sam, sample8_uv).rgb;
    const float3 sample9 = tex2D_linearize(tex, sam, sample9_uv).rgb;
    const float3 sample10 = tex2D_linearize(tex, sam, sample10_uv).rgb;
    const float3 sample11 = tex2D_linearize(tex, sam, sample11_uv).rgb;
    const float3 sample12 = tex2D_linearize(tex, sam, sample12_uv).rgb;
    const float3 sample13 = tex2D_linearize(tex, sam, sample13_uv).rgb;
    const float3 sample14 = tex2D_linearize(tex, sam, sample14_uv).rgb;
    const float3 sample15 = tex2D_linearize(tex, sam, sample15_uv).rgb;
    //  Compute destination pixel offsets for each sample:
    const float2 dest_pixel = tex_uv * tex_uv_to_pixel_scale;
    const float2 sample0_offset = sample0_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample1_offset = sample1_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample2_offset = sample2_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample3_offset = sample3_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample4_offset = sample4_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample5_offset = sample5_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample6_offset = sample6_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample7_offset = sample7_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample8_offset = sample8_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample9_offset = sample9_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample10_offset = sample10_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample11_offset = sample11_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample12_offset = sample12_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample13_offset = sample13_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample14_offset = sample14_uv * tex_uv_to_pixel_scale - dest_pixel;
    const float2 sample15_offset = sample15_uv * tex_uv_to_pixel_scale - dest_pixel;
    //  Compute Gaussian sample weights:
    const float w0 = exp(-LENGTH_SQ(sample0_offset) * denom_inv);
    const float w1 = exp(-LENGTH_SQ(sample1_offset) * denom_inv);
    const float w2 = exp(-LENGTH_SQ(sample2_offset) * denom_inv);
    const float w3 = exp(-LENGTH_SQ(sample3_offset) * denom_inv);
    const float w4 = exp(-LENGTH_SQ(sample4_offset) * denom_inv);
    const float w5 = exp(-LENGTH_SQ(sample5_offset) * denom_inv);
    const float w6 = exp(-LENGTH_SQ(sample6_offset) * denom_inv);
    const float w7 = exp(-LENGTH_SQ(sample7_offset) * denom_inv);
    const float w8 = exp(-LENGTH_SQ(sample8_offset) * denom_inv);
    const float w9 = exp(-LENGTH_SQ(sample9_offset) * denom_inv);
    const float w10 = exp(-LENGTH_SQ(sample10_offset) * denom_inv);
    const float w11 = exp(-LENGTH_SQ(sample11_offset) * denom_inv);
    const float w12 = exp(-LENGTH_SQ(sample12_offset) * denom_inv);
    const float w13 = exp(-LENGTH_SQ(sample13_offset) * denom_inv);
    const float w14 = exp(-LENGTH_SQ(sample14_offset) * denom_inv);
    const float w15 = exp(-LENGTH_SQ(sample15_offset) * denom_inv);
    const float weight_sum_inv = 1.0 / (
        w0 + w1 + w2 + w3 + w4 + w5 + w6 + w7 +
        w8 + w9 + w10 + w11 + w12 + w13 + w14 + w15);
    //  Weight and sum the samples:
    const float3 sum = w0 * sample0 + w1 * sample1 + w2 * sample2 + w3 * sample3 +
        w4 * sample4 + w5 * sample5 + w6 * sample6 + w7 * sample7 +
        w8 * sample8 + w9 * sample9 + w10 * sample10 + w11 * sample11 +
        w12 * sample12 + w13 * sample13 + w14 * sample14 + w15 * sample15;
    return sum * weight_sum_inv;
}

float4 Pass3(float2 pos) {
    //  Get the uv sample distance between output pixels.  We're using a resize
    //  blur, so arbitrary upsizing will be acceptable if filter_linearN =
    //  "true," and arbitrary downsizing will be acceptable if mipmap_inputN =
    //  "true" too.  The blur will be much more accurate if a true 4x4 Gaussian
    //  resize is used instead of tex2Dblur3x3_resize (which samples between
    //  texels even for upsizing).
    const float2 texture_size = float2(inputWidth, inputHeight);
    const float2 dxdy_min_scale = texture_size / float2(BLOOM_APPROX_WIDTH, BLOOM_APPROX_HEIGHT);
    const float2 texture_size_inv = { inputPtX, inputPtY };

    //  tex2Dresize_gaussian4x4 needs to know a bit more than the other filters:
    float2 tex_uv_to_pixel_scale = float2(BLOOM_APPROX_WIDTH, BLOOM_APPROX_HEIGHT);

    const float y_step = 1.0 + float(is_interlaced(inputHeight));
    const float2 il_step_multiple = float2(1.0, y_step);
    //  Get the uv distance between (texels, same-field scanlines):
    float2 uv_scanline_step = il_step_multiple / texture_size;

    //  The last pass (vertical scanlines) had a viewport y scale, so we can
    //  use it to calculate a better runtime sigma:
    float estimated_viewport_size_x = inputHeight * geom_aspect_ratio_x / geom_aspect_ratio_y;

    float2 blur_dxdy;
    if (bloom_approx_filter > 1.5)   //  4x4 true Gaussian resize
    {
        //  For upsizing, we'll snap to texels and sample the nearest 4.
        const float2 dxdy_scale = max(dxdy_min_scale, 1.0);
        blur_dxdy = dxdy_scale * texture_size_inv;
    } else {
        const float2 dxdy_scale = dxdy_min_scale;
        blur_dxdy = dxdy_scale * texture_size_inv;
    }

    //  Would a viewport-relative size work better for this pass?  (No.)
    //  PROS:
    //  1.) Instead of writing an absolute size to user-cgp-constants.h, we'd
    //      write a viewport scale.  That number could be used to directly scale
    //      the viewport-resolution bloom sigma and/or triad size to a smaller
    //      scale.  This way, we could calculate an optimal dynamic sigma no
    //      matter how the dot pitch is specified.
    //  CONS:
    //  1.) Texel smearing would be much worse at small viewport sizes, but
    //      performance would be much worse at large viewport sizes, so there
    //      would be no easy way to calculate a decent scale.
    //  2.) Worse, we could no longer get away with using a constant-size blur!
    //      Instead, we'd have to face all the same difficulties as the real
    //      phosphor bloom, which requires static #ifdefs to decide the blur
    //      size based on the expected triad size...a dynamic value.
    //  3.) Like the phosphor bloom, we'd have less control over making the blur
    //      size correct for an optical blur.  That said, we likely overblur (to
    //      maintain brightness) more than the eye would do by itself: 20/20
    //      human vision distinguishes ~1 arc minute, or 1/60 of a degree.  The
    //      highest viewing angle recommendation I know of is THX's 40.04 degree
    //      recommendation, at which 20/20 vision can distinguish about 2402.4
    //      lines.  Assuming the "TV lines" definition, that means 1201.2
    //      distinct light lines and 1201.2 distinct dark lines can be told
    //      apart, i.e. 1201.2 pairs of lines.  This would correspond to 1201.2
    //      pairs of alternating lit/unlit phosphors, so 2402.4 phosphors total
    //      (if they're alternately lit).  That's a max of 800.8 triads.  Using
    //      a more popular 30 degree viewing angle recommendation, 20/20 vision
    //      can distinguish 1800 lines, or 600 triads of alternately lit
    //      phosphors.  In contrast, we currently blur phosphors all the way
    //      down to 341.3 triads to ensure full brightness.
    //  4.) Realistically speaking, we're usually just going to use bilinear
    //      filtering in this pass anyway, but it only works well to limit
    //      bandwidth if it's done at a small constant scale.

    //  Get the constants we need to sample:
    const float2 tex_uv = pos;
    float2 tex_uv_r, tex_uv_g, tex_uv_b;
    if (beam_misconvergence) {
        const float2 convergence_offsets_r = get_convergence_offsets_r_vector();
        const float2 convergence_offsets_g = get_convergence_offsets_g_vector();
        const float2 convergence_offsets_b = get_convergence_offsets_b_vector();
        tex_uv_r = tex_uv - convergence_offsets_r * uv_scanline_step;
        tex_uv_g = tex_uv - convergence_offsets_g * uv_scanline_step;
        tex_uv_b = tex_uv - convergence_offsets_b * uv_scanline_step;
    }
    //  Get the blur sigma:
    const float bloom_approx_sigma = get_bloom_approx_sigma(BLOOM_APPROX_WIDTH,
        estimated_viewport_size_x);

    //  Sample the resized and blurred texture, and apply convergence offsets if
    //  necessary.  Applying convergence offsets here triples our samples from
    //  16/9/1 to 48/27/3, but faster and easier than sampling BLOOM_APPROX and
    //  HALATION_BLUR 3 times at full resolution every time they're used.
    float3 color_r, color_g, color_b, color;
    if (bloom_approx_filter > 1.5) {
        //  Use a 4x4 Gaussian resize.  This is slower but technically correct.
        if (beam_misconvergence) {
            color_r = tex2Dresize_gaussian4x4(tex2, samLinear, tex_uv_r,
                blur_dxdy, texture_size, texture_size_inv,
                tex_uv_to_pixel_scale, bloom_approx_sigma);
            color_g = tex2Dresize_gaussian4x4(tex2, samLinear, tex_uv_g,
                blur_dxdy, texture_size, texture_size_inv,
                tex_uv_to_pixel_scale, bloom_approx_sigma);
            color_b = tex2Dresize_gaussian4x4(tex2, samLinear, tex_uv_b,
                blur_dxdy, texture_size, texture_size_inv,
                tex_uv_to_pixel_scale, bloom_approx_sigma);
        } else {
            color = tex2Dresize_gaussian4x4(tex2, samLinear, tex_uv,
                blur_dxdy, texture_size, texture_size_inv,
                tex_uv_to_pixel_scale, bloom_approx_sigma);
        }
    } else if (bloom_approx_filter > 0.5) {
        //  Use a 3x3 resize blur.  This is the softest option, because we're
        //  blurring already blurry bilinear samples.  It doesn't play quite as
        //  nicely with convergence offsets, but it has its charms.
        if (beam_misconvergence) {
            color_r = tex2Dblur3x3resize(tex2, samLinear, tex_uv_r,
                blur_dxdy, bloom_approx_sigma);
            color_g = tex2Dblur3x3resize(tex2, samLinear, tex_uv_g,
                blur_dxdy, bloom_approx_sigma);
            color_b = tex2Dblur3x3resize(tex2, samLinear, tex_uv_b,
                blur_dxdy, bloom_approx_sigma);
        } else {
            color = tex2Dblur3x3resize(tex2, samLinear, tex_uv, blur_dxdy);
        }
    } else {
        //  Use bilinear sampling.  This approximates a 4x4 Gaussian resize MUCH
        //  better than tex2Dblur3x3_resize for the very small sigmas we're
        //  likely to use at small output resolutions.  (This estimate becomes
        //  too sharp above ~400x300, but the blurs break down above that
        //  resolution too, unless min_allowed_viewport_triads is high enough to
        //  keep bloom_approx_scale_x/min_allowed_viewport_triads < ~1.1658025.)
        if (beam_misconvergence) {
            color_r = tex2D_linearize(tex2, samLinear, tex_uv_r).rgb;
            color_g = tex2D_linearize(tex2, samLinear, tex_uv_g).rgb;
            color_b = tex2D_linearize(tex2, samLinear, tex_uv_b).rgb;
        } else {
            color = tex2D_linearize(tex2, samLinear, tex_uv).rgb;
        }
    }
    //  Pack the colors from the red/green/blue beams into a single vector:
    if (beam_misconvergence) {
        color = float3(color_r.r, color_g.g, color_b.b);
    }
    //  Encode and output the blurred image:
    return encode_output(float4(color, 1.0));
}


//!PASS 4
//!BIND tex3
//!SAVE tex4

// 移植自 https://github.com/libretro/common-shaders/blob/master/blurs/blur9fast-vertical.cg

#include "CRT_Royale_gamma-management.hlsli"
#include "CRT_Royale_blur-functions.hlsli"

float4 Pass4(float2 pos) {
    //  Get the uv sample distance between output pixels.  Blurs are not generic
    //  Gaussian resizers, and correct blurs require:
    //  1.) IN.output_size == IN.video_size * 2^m, where m is an integer <= 0.
    //  2.) mipmap_inputN = "true" for this pass in .cgp preset if m != 0
    //  3.) filter_linearN = "true" except for 1x scale nearest neighbor blurs
    //  Gaussian resizers would upsize using the distance between input texels
    //  (not output pixels), but we avoid this and consistently blur at the
    //  destination size.  Otherwise, combining statically calculated weights
    //  with bilinear sample exploitation would result in terrible artifacts.

    //  This blur is vertical-only, so zero out the horizontal offset:
    float2 blur_dxdy = { 0.0, 1 / BLOOM_APPROX_HEIGHT };

    float3 color = tex2Dblur9fast(tex3, samLinear, pos, blur_dxdy);
    //  Encode and output the blurred image:
    return encode_output(float4(color, 1.0));
}

//!PASS 5
//!BIND tex4
//!SAVE tex5

// 移植自 https://github.com/libretro/common-shaders/blob/master/blurs/blur9fast-horizontal.cg

#include "CRT_Royale_gamma-management.hlsli"
#include "CRT_Royale_blur-functions.hlsli"

float4 Pass5(float2 pos) {
    //  Get the uv sample distance between output pixels.  Blurs are not generic
    //  Gaussian resizers, and correct blurs require:
    //  1.) IN.output_size == IN.video_size * 2^m, where m is an integer <= 0.
    //  2.) mipmap_inputN = "true" for this pass in .cgp preset if m != 0
    //  3.) filter_linearN = "true" except for 1x scale nearest neighbor blurs
    //  Gaussian resizers would upsize using the distance between input texels
    //  (not output pixels), but we avoid this and consistently blur at the
    //  destination size.  Otherwise, combining statically calculated weights
    //  with bilinear sample exploitation would result in terrible artifacts.

    //  This blur is horizontal-only, so zero out the vertical offset:
    float2 blur_dxdy = { 1 / BLOOM_APPROX_WIDTH, 0.0 };

    float3 color = tex2Dblur9fast(tex4, samLinear, pos, blur_dxdy);
    //  Encode and output the blurred image:
    return encode_output(float4(color, 1.0));
}

//!PASS 6
//!BIND mask_grille_texture_small, mask_slot_texture_small, mask_shadow_texture_small
//!SAVE tex6

// 移植自 https://github.com/libretro/common-shaders/blob/master/crt/shaders/crt-royale/src/crt-royale-mask-resize-vertical.cg

#include "CRT_Royale_bind-shader-params.hlsli"
#include "CRT_Royale_phosphor-mask-resizing.hlsli"


float4 Pass6(float2 pos) {
    float2 output_size = { 64, 0.0625 * outputHeight };

    //  First estimate the viewport size (the user will get the wrong number of
    //  triads if it's wrong and mask_specify_num_triads is 1.0/true).
    const float viewport_y = output_size.y / mask_resize_viewport_scale.y;
    const float aspect_ratio = geom_aspect_ratio_x / geom_aspect_ratio_y;
    const float2 estimated_viewport_size =
        float2(viewport_y * aspect_ratio, viewport_y);
    //  Estimate the output size of MASK_RESIZE (the next pass).  The estimated
    //  x component shouldn't matter, because we're not using the x result, and
    //  we're not swearing it's correct (if we did, the x result would influence
    //  the y result to maintain the tile aspect ratio).
    const float2 estimated_mask_resize_output_size =
        float2(output_size.y * aspect_ratio, output_size.y);
    //  Find the final intended [y] size of our resized phosphor mask tiles,
    //  then the tile size for the current pass (resize y only):
    const float2 mask_resize_tile_size = get_resized_mask_tile_size(
        estimated_viewport_size, estimated_mask_resize_output_size, false);
    const float2 pass_output_tile_size = float2(min(
        mask_resize_src_lut_size.x, output_size.x), mask_resize_tile_size.y);

    //  We'll render resized tiles until filling the output FBO or meeting a
    //  limit, so compute [wrapped] tile uv coords based on the output uv coords
    //  and the number of tiles that will fit in the FBO.
    const float2 output_tiles_this_pass = output_size / pass_output_tile_size;
    const float2 output_video_uv = pos;
    const float2 tile_uv_wrap = output_video_uv * output_tiles_this_pass;

    //  The input LUT is just a single mask tile, so texture uv coords are the
    //  same as tile uv coords (save frac() for the fragment shader).  The
    //  magnification scale is also straightforward:
    const float2 src_tex_uv_wrap = tile_uv_wrap;
    const float resize_magnification_scale =
        pass_output_tile_size.y / mask_resize_src_lut_size.y;

    //  Statically select small [non-mipmapped] or large [mipmapped] textures:
#ifdef PHOSPHOR_MASK_RESIZE_MIPMAPPED_LUT
    //Texture2D mask_grille_texture = mask_grille_texture_large;
    //Texture2D mask_slot_texture = mask_slot_texture_large;
    //Texture2D mask_shadow_texture = mask_shadow_texture_large;
#else
    Texture2D mask_grille_texture = mask_grille_texture_small;
    Texture2D mask_slot_texture = mask_slot_texture_small;
    Texture2D mask_shadow_texture = mask_shadow_texture_small;
#endif

    //  Resize the input phosphor mask tile to the final vertical size it will
    //  appear on screen.  Keep 1x horizontal size if possible (IN.output_size
    //  >= mask_resize_src_lut_size), and otherwise linearly sample horizontally
    //  to fit exactly one tile.  Lanczos-resizing the phosphor mask achieves
    //  much sharper results than mipmapping, and vertically resizing first
    //  minimizes the total number of taps required.  We output a number of
    //  resized tiles >= mask_resize_num_tiles for easier tiled sampling later.
#ifdef PHOSPHOR_MASK_MANUALLY_RESIZE
    //  Discard unneeded fragments in case our profile allows real branches.
    if (get_mask_sample_mode() < 0.5 &&
        tile_uv_wrap.y <= mask_resize_num_tiles) {
        static const float src_dy = 1.0 / mask_resize_src_lut_size.y;
        const float2 src_tex_uv = frac(src_tex_uv_wrap);
        float3 pixel_color;
        //  If mask_type is static, this branch will be resolved statically.
        if (mask_type < 0.5) {
            pixel_color = downsample_vertical_sinc_tiled(
                mask_grille_texture, samLinear, src_tex_uv, mask_resize_src_lut_size,
                src_dy, resize_magnification_scale, 1.0);
        } else if (mask_type < 1.5) {
            pixel_color = downsample_vertical_sinc_tiled(
                mask_slot_texture, samLinear, src_tex_uv, mask_resize_src_lut_size,
                src_dy, resize_magnification_scale, 1.0);
        } else {
            pixel_color = downsample_vertical_sinc_tiled(
                mask_shadow_texture, samLinear, src_tex_uv, mask_resize_src_lut_size,
                src_dy, resize_magnification_scale, 1.0);
        }
        //  The input LUT was linear RGB, and so is our output:
        return float4(pixel_color, 1.0);
    } else {
        discard;
        return float4(0, 0, 0, 1);
    }
#else
    discard;
    return float4(0, 0, 0, 1);
#endif
}


//!PASS 7
//!BIND tex6
//!SAVE tex7

// 移植自 https://github.com/libretro/common-shaders/blob/master/crt/shaders/crt-royale/src/crt-royale-mask-resize-horizontal.cg

#include "CRT_Royale_bind-shader-params.hlsli"
#include "CRT_Royale_phosphor-mask-resizing.hlsli"


float4 Pass7(float2 pos) {
    float2 texture_size = { 64, 0.0625 * outputHeight };
    float2 output_size = 0.0625 * float2(outputWidth, outputHeight);

    //  First estimate the viewport size (the user will get the wrong number of
    //  triads if it's wrong and mask_specify_num_triads is 1.0/true).
    const float2 estimated_viewport_size =
        output_size / mask_resize_viewport_scale;
    //  Find the final size of our resized phosphor mask tiles.  We probably
    //  estimated the viewport size and MASK_RESIZE output size differently last
    //  pass, so do not swear they were the same. ;)
    const float2 mask_resize_tile_size = get_resized_mask_tile_size(
        estimated_viewport_size, output_size, false);

    //  We'll render resized tiles until filling the output FBO or meeting a
    //  limit, so compute [wrapped] tile uv coords based on the output uv coords
    //  and the number of tiles that will fit in the FBO.
    const float2 output_tiles_this_pass = output_size / mask_resize_tile_size;
    const float2 output_video_uv = pos;
    const float2 tile_uv_wrap = output_video_uv * output_tiles_this_pass;

    //  Get the texel size of an input tile and related values:
    const float2 input_tile_size = float2(min(
        mask_resize_src_lut_size.x, texture_size.x), mask_resize_tile_size.y);
    const float2 tile_size_uv = input_tile_size / texture_size;
    const float2 input_tiles_per_texture = texture_size / input_tile_size;

    //  Derive [wrapped] texture uv coords from [wrapped] tile uv coords and
    //  the tile size in uv coords, and save frac() for the fragment shader.
    const float2 src_tex_uv_wrap = tile_uv_wrap * tile_size_uv;

    float2 src_dxdy = float2(1.0 / texture_size.x, 0.0);
    float resize_magnification_scale = mask_resize_tile_size.x / input_tile_size.x;

    //  The input contains one mask tile horizontally and a number vertically.
    //  Resize the tile horizontally to its final screen size and repeat it
    //  until drawing at least mask_resize_num_tiles, leaving it unchanged
    //  vertically.  Lanczos-resizing the phosphor mask achieves much sharper
    //  results than mipmapping, outputting >= mask_resize_num_tiles makes for
    //  easier tiled sampling later.
#ifdef PHOSPHOR_MASK_MANUALLY_RESIZE
    //  Discard unneeded fragments in case our profile allows real branches.
    if (get_mask_sample_mode() < 0.5 &&
        max(tile_uv_wrap.x, tile_uv_wrap.y) <= mask_resize_num_tiles) {
        const float src_dx = src_dxdy.x;
        const float2 src_tex_uv = frac(src_tex_uv_wrap);
        const float3 pixel_color = downsample_horizontal_sinc_tiled(tex6, samPoint,
            src_tex_uv, texture_size, src_dxdy.x,
            resize_magnification_scale, tile_size_uv.x);
        //  The input LUT was linear RGB, and so is our output:
        return float4(pixel_color, 1.0);
    } else {
        discard;
        return float4(0, 0, 0, 1);
    }
#else
    discard;
    return float4(0, 0, 0, 1);
#endif
}


//!PASS 8
//!BIND tex2, tex3, tex5, tex7

// 移植自 https://github.com/libretro/common-shaders/blob/master/crt/shaders/crt-royale/src/crt-royale-scanlines-horizontal-apply-mask.cg

#include "CRT_Royale_bind-shader-params.hlsli"
#include "CRT_Royale_scanline-functions.hlsli"
#include "CRT_Royale_phosphor-mask-resizing.hlsli"
#include "CRT_Royale_bloom-functions.hlsli"
#include "CRT_Royale_gamma-management.hlsli"


float4 tex2Dtiled_mask_linearize(Texture2D tex, SamplerState sam, const float2 tex_uv) {
    //  If we're manually tiling a texture, anisotropic filtering can get
    //  confused.  One workaround is to just select the lowest mip level:
#ifdef PHOSPHOR_MASK_MANUALLY_RESIZE
#ifdef ANISOTROPIC_TILING_COMPAT_TEX2DLOD
    //  TODO: Use tex2Dlod_linearize with a calculated mip level.
    return tex2Dlod_linearize(tex, sam, float4(tex_uv, 0.0, 0.0));
#else
#ifdef ANISOTROPIC_TILING_COMPAT_TEX2DBIAS
    return tex2Dbias_linearize(tex, sam, float4(tex_uv, 0.0, -16.0));
#else
    return tex2D_linearize(tex, sam, tex_uv);
#endif
#endif
#else
    return tex2D_linearize(tex, sam, tex_uv);
#endif
}

float4 Pass8(float2 pos) {
    //  Our various input textures use different coords.
    const float2 video_uv = pos;
    const float2 scanline_texture_size_inv = float2(inputPtX, inputPtY);
    float2 scanline_tex_uv = video_uv;
    float2 blur3x3_tex_uv = video_uv;
    float2 halation_tex_uv = video_uv;

    //  Get a consistent name for the final mask texture size.  Sample mode 0
    //  uses the manually resized mask, but ignore it if we never resized.
#ifdef PHOSPHOR_MASK_MANUALLY_RESIZE
    const float mask_sample_mode = get_mask_sample_mode();
    const float2 MASK_RESIZE_video_size = 0.0625 * float2(outputWidth, outputHeight);
    const float2 mask_resize_texture_size = mask_sample_mode < 0.5 ?
        MASK_RESIZE_video_size : mask_texture_large_size;
    const float2 mask_resize_video_size = mask_sample_mode < 0.5 ?
        MASK_RESIZE_video_size : mask_texture_large_size;
#else
    const float2 mask_resize_texture_size = mask_texture_large_size;
    const float2 mask_resize_video_size = mask_texture_large_size;
#endif
    //  Compute mask tile dimensions, starting points, etc.:
    float2 mask_tiles_per_screen;
    float4 mask_tile_start_uv_and_size = get_mask_sampling_parameters(
        mask_resize_texture_size, mask_resize_video_size, float2(outputWidth, outputHeight),
        mask_tiles_per_screen);

    //  This pass: Sample (misconverged?) scanlines to the final horizontal
    //  resolution, apply halation (bouncing electrons), and apply the phosphor
    //  mask.  Fake a bloom if requested.  Unless we fake a bloom, the output
    //  will be dim from the scanline auto-dim, mask dimming, and low gamma.

    //  Horizontally sample the current row (a vertically interpolated scanline)
    //  and account for horizontal convergence offsets, given in units of texels.
    const float3 scanline_color_dim = sample_rgb_scanline_horizontal(
        tex2, samLinear, scanline_tex_uv,
        float2(inputWidth, inputHeight), scanline_texture_size_inv);
    const float auto_dim_factor = levels_autodim_temp;

    //  Sample the phosphor mask:
    const float2 tile_uv_wrap = video_uv * mask_tiles_per_screen;
    const float2 mask_tex_uv = convert_phosphor_tile_uv_wrap_to_tex_uv(
        tile_uv_wrap, mask_tile_start_uv_and_size);
    float3 phosphor_mask_sample;
#ifdef PHOSPHOR_MASK_MANUALLY_RESIZE
    const bool sample_orig_luts = get_mask_sample_mode() > 0.5;
#else
    static const bool sample_orig_luts = true;
#endif
    if (sample_orig_luts) {
        //  If mask_type is static, this branch will be resolved statically.
        /*if (mask_type < 0.5) {
            phosphor_mask_sample = tex2D_linearize(
                mask_grille_texture_large, mask_tex_uv).rgb;
        } else if (mask_type < 1.5) {
            phosphor_mask_sample = tex2D_linearize(
                mask_slot_texture_large, mask_tex_uv).rgb;
        } else {
            phosphor_mask_sample = tex2D_linearize(
                mask_shadow_texture_large, mask_tex_uv).rgb;
        }*/
    } else {
        //  Sample the resized mask, and avoid tiling artifacts:
        phosphor_mask_sample = tex2Dtiled_mask_linearize(
            tex7, samLinear, mask_tex_uv).rgb;
    }

    //  Sample the halation texture (auto-dim to match the scanlines), and
    //  account for both horizontal and vertical convergence offsets, given
    //  in units of texels horizontally and same-field scanlines vertically:
    const float3 halation_color = tex2D_linearize(
        tex5, samLinear, halation_tex_uv).rgb;

    //  Apply halation: Halation models electrons flying around under the glass
    //  and hitting the wrong phosphors (of any color).  It desaturates, so
    //  average the halation electrons to a scalar.  Reduce the local scanline
    //  intensity accordingly to conserve energy.
    const float3 halation_intensity_dim = dot(halation_color, auto_dim_factor / 3.0);
    const float3 electron_intensity_dim = lerp(scanline_color_dim,
        halation_intensity_dim, halation_weight);

    //  Apply the phosphor mask:
    const float3 phosphor_emission_dim = electron_intensity_dim *
        phosphor_mask_sample;

#ifdef PHOSPHOR_BLOOM_FAKE
    //  The BLOOM_APPROX pass approximates a blurred version of a masked
    //  and scanlined image.  It's usually used to compute the brightpass,
    //  but we can also use it to fake the bloom stage entirely.  Caveats:
    //  1.) A fake bloom is conceptually different, since we're mixing in a
    //      fully blurred low-res image, and the biggest implication are:
    //  2.) If mask_amplify is incorrect, results deteriorate more quickly.
    //  3.) The inaccurate blurring hurts quality in high-contrast areas.
    //  4.) The bloom_underestimate_levels parameter seems less sensitive.
    //  Reverse the auto-dimming and amplify to compensate for mask dimming:
#define PHOSPHOR_BLOOM_FAKE_WITH_SIMPLE_BLEND
#ifdef PHOSPHOR_BLOOM_FAKE_WITH_SIMPLE_BLEND
    static const float blur_contrast = 1.05;
#else
    static const float blur_contrast = 1.0;
#endif
    const float mask_amplify = get_mask_amplify();
    const float undim_factor = 1.0 / auto_dim_factor;
    const float3 phosphor_emission =
        phosphor_emission_dim * undim_factor * mask_amplify;
    //  Get a phosphor blur estimate, accounting for convergence offsets:
    const float3 electron_intensity = electron_intensity_dim * undim_factor;
    const float3 phosphor_blur_approx_soft = tex2D_linearize(
        tex3, samLinear, blur3x3_tex_uv).rgb;
    const float3 phosphor_blur_approx = lerp(phosphor_blur_approx_soft,
        electron_intensity, 0.1) * blur_contrast;
    //  We could blend between phosphor_emission and phosphor_blur_approx,
    //  solving for the minimum blend_ratio that avoids clipping past 1.0:
    //      1.0 >= total_intensity
    //      1.0 >= phosphor_emission * (1.0 - blend_ratio) +
    //              phosphor_blur_approx * blend_ratio
    //      blend_ratio = (phosphor_emission - 1.0)/
    //          (phosphor_emission - phosphor_blur_approx);
    //  However, this blurs far more than necessary, because it aims for
    //  full brightness, not minimal blurring.  To fix it, base blend_ratio
    //  on a max area intensity only so it varies more smoothly:
    const float3 phosphor_blur_underestimate =
        phosphor_blur_approx * bloom_underestimate_levels;
    const float3 area_max_underestimate =
        phosphor_blur_underestimate * mask_amplify;
#ifdef PHOSPHOR_BLOOM_FAKE_WITH_SIMPLE_BLEND
    const float3 blend_ratio_temp =
        (area_max_underestimate - 1.0) /
        (area_max_underestimate - phosphor_blur_underestimate);
#else
    //  Try doing it like an area-based brightpass.  This is nearly
    //  identical, but it's worth toying with the code in case I ever
    //  find a way to make it look more like a real bloom.  (I've had
    //  some promising textures from combining an area-based blend ratio
    //  for the phosphor blur and a more brightpass-like blend-ratio for
    //  the phosphor emission, but I haven't found a way to make the
    //  brightness correct across the whole color range, especially with
    //  different bloom_underestimate_levels values.)
    const float desired_triad_size = lerp(mask_triad_size_desired,
        outputWidth / mask_num_triads_desired,
        mask_specify_num_triads);
    const float bloom_sigma = get_min_sigma_to_blur_triad(
        desired_triad_size, bloom_diff_thresh);
    const float center_weight = get_center_weight(bloom_sigma);
    const float3 max_area_contribution_approx =
        max(0.0, phosphor_blur_approx -
            center_weight * phosphor_emission);
    const float3 area_contrib_underestimate =
        bloom_underestimate_levels * max_area_contribution_approx;
    const float3 blend_ratio_temp =
        ((1.0 - area_contrib_underestimate) /
            area_max_underestimate - 1.0) / (center_weight - 1.0);
#endif
    //  Clamp blend_ratio in case it's out-of-range, but be SUPER careful:
    //  min/max/clamp are BIZARRELY broken with lerp (optimization bug?),
    //  and this redundant sequence avoids bugs, at least on nVidia cards:
    const float3 blend_ratio_clamped = max(clamp(blend_ratio_temp, 0.0, 1.0), 0.0);
    const float3 blend_ratio = lerp(blend_ratio_clamped, 1.0, bloom_excess);
    //  Blend the blurred and unblurred images:
    const float3 phosphor_emission_unclipped =
        lerp(phosphor_emission, phosphor_blur_approx, blend_ratio);
    //  Simulate refractive diffusion by reusing the halation sample.
    const float3 pixel_color = lerp(phosphor_emission_unclipped,
        halation_color, diffusion_weight);
#else
    const float3 pixel_color = phosphor_emission_dim;
#endif
    //  Encode if necessary, and output.
    return encode_output(float4(pixel_color, 1.0));
}
