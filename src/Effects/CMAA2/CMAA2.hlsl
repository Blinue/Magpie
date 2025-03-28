//!MAGPIE EFFECT
//!VERSION 4

//============================================================================
// CMAA2 - Conservative Morphological Anti-Aliasing 2
//============================================================================
// A lightweight anti-aliasing solution for Magpie based on Intel's CMAA2
// This implementation provides a simplified but efficient version of CMAA2
// designed specifically for compatibility with Magpie
//
// Originally developed by Intel:
// https://github.com/GameTechDev/CMAA2
//
// Based on concepts from:
// - Intel's original CMAA2 implementation
// - Pascal Gilcher's ReShade port of CMAA2
//
// Usage:
// - Adjust CMAA_EDGE_THRESHOLD to control sensitivity (lower = more edges detected)
// - Adjust CMAA_BLEND_STRENGTH to control anti-aliasing intensity
//
// Recommended usage with FSR:
// 1. CMAA2 (to remove jaggies)
// 2. FSR EASU (for upscaling)
// 3. FSR CAS (for final sharpening)
//============================================================================

//!TEXTURE
Texture2D INPUT;
//!TEXTURE
Texture2D OUTPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam;

//!COMMON
// Adjustable parameters
//--------------------------------------
// Controls edge detection sensitivity
// Lower values detect more edges but may affect texture detail
// Default: 0.1, Range: 0.05-0.2
#define CMAA_EDGE_THRESHOLD 0.1

// Controls how strongly to blend edges
// Higher values create smoother transitions but may blur details
// Default: 0.75, Range: 0.3-1.0
#define CMAA_BLEND_STRENGTH 0.75
//--------------------------------------

// Helper function to calculate luminance from RGB
// Using Rec. 601 luma coefficients for better perceptual accuracy
float luma(float3 color) {
    return dot(color, float3(0.299, 0.587, 0.114));
}

//!PASS 1
//!DESC CMAA2 Anti-aliasing
//!STYLE PS
//!IN INPUT
//!OUT OUTPUT
float4 Pass1(float2 pos) {
    // Get texture dimensions and calculate inverse for sampling
    uint width, height;
    INPUT.GetDimensions(width, height);
    float2 invTexSize = float2(1.0 / width, 1.0 / height);
    
    // 1. SAMPLE PIXEL NEIGHBORHOOD
    //-----------------------------------------------------------
    // Sample the current pixel and its 4 neighbors in a plus pattern:
    //      T
    //    L C R
    //      B
    float4 originalColor = INPUT.SampleLevel(sam, pos, 0);
    float3 center = originalColor.rgb;
    float3 top = INPUT.SampleLevel(sam, pos + float2(0, -invTexSize.y), 0).rgb;
    float3 right = INPUT.SampleLevel(sam, pos + float2(invTexSize.x, 0), 0).rgb;
    float3 bottom = INPUT.SampleLevel(sam, pos + float2(0, invTexSize.y), 0).rgb;
    float3 left = INPUT.SampleLevel(sam, pos + float2(-invTexSize.x, 0), 0).rgb;
    
    // 2. CALCULATE LUMINANCE VALUES
    //-----------------------------------------------------------
    // Luminance provides a perceptual measure of brightness
    // We use this for edge detection instead of raw RGB differences
    float lumaCenter = luma(center);
    float lumaTop = luma(top);
    float lumaRight = luma(right);
    float lumaBottom = luma(bottom);
    float lumaLeft = luma(left);
    
    // 3. DETECT EDGES
    //-----------------------------------------------------------
    // Calculate absolute luminance differences between center and neighbors
    float deltaTop = abs(lumaTop - lumaCenter);
    float deltaRight = abs(lumaRight - lumaCenter);
    float deltaBottom = abs(lumaBottom - lumaCenter);
    float deltaLeft = abs(lumaLeft - lumaCenter);
    
    // Detect horizontal and vertical edges based on luminance differences
    // Horizontal edges: strong difference between top/bottom and center
    // Vertical edges: strong difference between left/right and center
    bool isHorzEdge = deltaTop + deltaBottom >= CMAA_EDGE_THRESHOLD;
    bool isVertEdge = deltaLeft + deltaRight >= CMAA_EDGE_THRESHOLD;
    
    // Skip processing if no edges detected
    if (!isHorzEdge && !isVertEdge) {
        return originalColor;
    }
    
    // 4. APPLY ANTI-ALIASING
    //-----------------------------------------------------------
    // Start with original color
    float3 outColor = center;
    
    // For horizontal edges, blend with the average of top and bottom pixels
    if (isHorzEdge) {
        float3 blendColor = (top + bottom) * 0.5;
        outColor = lerp(outColor, blendColor, CMAA_BLEND_STRENGTH * 0.5);
    }
    
    // For vertical edges, blend with the average of left and right pixels
    if (isVertEdge) {
        float3 blendColor = (left + right) * 0.5;
        outColor = lerp(outColor, blendColor, CMAA_BLEND_STRENGTH * 0.5);
    }
    
    // Preserve original alpha value
    return float4(outColor, originalColor.a);
}
