//!MAGPIE EFFECT
//!VERSION 4

//============================================================================
// CMAA2 - Conservative Morphological Anti-Aliasing 2
//============================================================================
// A high-quality anti-aliasing solution for Magpie based on Intel's CMAA2
// This implementation provides edge-directed anti-aliasing with shape detection
//
// Originally developed by Intel:
// https://github.com/GameTechDev/CMAA2
//
// Usage:
// - CMAA_EDGE_THRESHOLD: Controls edge detection sensitivity
// - CMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR: Controls how local contrast affects AA
// - CMAA_CORNER_ROUNDING: Controls how aggressively to round corners
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
// Default: 0.075, Range: 0.05-0.2
#define CMAA_EDGE_THRESHOLD 0.075

// Controls how much local contrast affects edge importance
// Default: 0.8, Range: 0.5-1.0
#define CMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR 0.8

// Controls how much corners are rounded during anti-aliasing
// Default: 0.22, Range: 0.1-0.3
#define CMAA_CORNER_ROUNDING 0.22

// Maximum blend factor to prevent over-blurring
// Default: 0.75, Range: 0.5-1.0
#define CMAA_MAX_BLEND_FACTOR 0.75

// Early exit threshold for subtle edges
// Default: 0.015, Range: 0.01-0.05
#define CMAA_EARLY_EXIT_THRESHOLD 0.015

// Maximum steps when searching along edges
// Default: 16, Range: 8-32
#define CMAA_MAX_SEARCH_STEPS 16

// Helper function to calculate luminance from RGB
// Using Rec. 709 coefficients for accurate perceptual brightness
float ComputeLuma(float3 color) {
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

// Search along an edge direction
float4 SearchAlongEdge(Texture2D tex, SamplerState samp, float2 uv, float2 direction, float2 texelSize, int maxSteps) {
    float edgeStrength = 0.0f;
    float2 finalOffset = float2(0, 0);
    float stepCount = 0;
    
    for (int i = 1; i <= maxSteps; i++) {
        float2 samplePos = uv + direction * i * texelSize;
        
        // Sample neighborhood at this position
        float3 colorCenter = tex.SampleLevel(samp, samplePos, 0).rgb;
        float3 colorLeft = tex.SampleLevel(samp, samplePos + float2(-texelSize.x, 0), 0).rgb;
        float3 colorRight = tex.SampleLevel(samp, samplePos + float2(texelSize.x, 0), 0).rgb;
        float3 colorTop = tex.SampleLevel(samp, samplePos + float2(0, -texelSize.y), 0).rgb;
        float3 colorBottom = tex.SampleLevel(samp, samplePos + float2(0, texelSize.y), 0).rgb;
        
        // Compute luma values
        float lumaCenter = ComputeLuma(colorCenter);
        float lumaLeft = ComputeLuma(colorLeft);
        float lumaRight = ComputeLuma(colorRight);
        float lumaTop = ComputeLuma(colorTop);
        float lumaBottom = ComputeLuma(colorBottom);
        
        // Calculate edge strengths
        float edgeHorizontal = abs(lumaTop - lumaCenter) + abs(lumaBottom - lumaCenter);
        float edgeVertical = abs(lumaLeft - lumaCenter) + abs(lumaRight - lumaCenter);
        
        // Compute overall edge strength
        float sampleStrength = max(edgeHorizontal, edgeVertical);
        
        if (sampleStrength > CMAA_EDGE_THRESHOLD) {
            edgeStrength = max(edgeStrength, sampleStrength);
            finalOffset = direction * i * texelSize;
            stepCount = i;
        } else {
            break; // End of edge found
        }
    }
    
    return float4(finalOffset.x, finalOffset.y, edgeStrength, stepCount);
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
    float2 texelSize = float2(1.0 / width, 1.0 / height);
    
    // 1. SAMPLE PIXEL NEIGHBORHOOD
    //-----------------------------------------------------------
    float4 originalColor = INPUT.SampleLevel(sam, pos, 0);
    float3 center = originalColor.rgb;
    
    // Sample 3x3 neighborhood
    float3 top = INPUT.SampleLevel(sam, pos + float2(0, -texelSize.y), 0).rgb;
    float3 right = INPUT.SampleLevel(sam, pos + float2(texelSize.x, 0), 0).rgb;
    float3 bottom = INPUT.SampleLevel(sam, pos + float2(0, texelSize.y), 0).rgb;
    float3 left = INPUT.SampleLevel(sam, pos + float2(-texelSize.x, 0), 0).rgb;
    
    float3 topLeft = INPUT.SampleLevel(sam, pos + float2(-texelSize.x, -texelSize.y), 0).rgb;
    float3 topRight = INPUT.SampleLevel(sam, pos + float2(texelSize.x, -texelSize.y), 0).rgb;
    float3 bottomLeft = INPUT.SampleLevel(sam, pos + float2(-texelSize.x, texelSize.y), 0).rgb;
    float3 bottomRight = INPUT.SampleLevel(sam, pos + float2(texelSize.x, texelSize.y), 0).rgb;
    
    // 2. CALCULATE LUMINANCE VALUES
    //-----------------------------------------------------------
    float lumaCenter = ComputeLuma(center);
    
    float lumaTop = ComputeLuma(top);
    float lumaRight = ComputeLuma(right);
    float lumaBottom = ComputeLuma(bottom);
    float lumaLeft = ComputeLuma(left);
    
    float lumaTopLeft = ComputeLuma(topLeft);
    float lumaTopRight = ComputeLuma(topRight);
    float lumaBottomLeft = ComputeLuma(bottomLeft);
    float lumaBottomRight = ComputeLuma(bottomRight);
    
    // Apply sharpening to center luma to better detect edges
    float lumaSharpened = lumaCenter * 5.0f - (lumaTopLeft + lumaTopRight + lumaBottomLeft + lumaBottomRight);
    lumaCenter = lerp(lumaCenter, lumaSharpened, 0.2f);
    
    // 3. DETECT EDGES
    //-----------------------------------------------------------
    // Compute edge strengths in different directions
    float edgeHorizontal = abs(lumaLeft - lumaCenter) + abs(lumaRight - lumaCenter);
    float edgeVertical = abs(lumaTop - lumaCenter) + abs(lumaBottom - lumaCenter);
    float edgeDiagonal1 = abs(lumaTopLeft - lumaCenter) + abs(lumaBottomRight - lumaCenter);
    float edgeDiagonal2 = abs(lumaTopRight - lumaCenter) + abs(lumaBottomLeft - lumaCenter);
    
    // Early exit if no significant edges are found
    if (max(max(edgeHorizontal, edgeVertical), max(edgeDiagonal1, edgeDiagonal2)) <= CMAA_EARLY_EXIT_THRESHOLD) {
        return originalColor;
    }
    
    // Determine if we have horizontal, vertical or diagonal edges
    bool hasHorzEdge = edgeHorizontal >= CMAA_EDGE_THRESHOLD;
    bool hasVertEdge = edgeVertical >= CMAA_EDGE_THRESHOLD;
    bool hasDiag1Edge = edgeDiagonal1 >= CMAA_EDGE_THRESHOLD;
    bool hasDiag2Edge = edgeDiagonal2 >= CMAA_EDGE_THRESHOLD;
    
    // If no edges were found, return the original color
    if (!hasHorzEdge && !hasVertEdge && !hasDiag1Edge && !hasDiag2Edge) {
        return originalColor;
    }
    
    // 4. DETECT SHAPES
    //-----------------------------------------------------------
    float zStrength = 0.0f;
    float lStrength = 0.0f;
    float2 zDirection = float2(0, 0);
    
    // Detect L-junctions (horizontal + vertical edges)
    if (hasHorzEdge && hasVertEdge) {
        float4 horizontal = SearchAlongEdge(INPUT, sam, pos, float2(1, 0), texelSize, CMAA_MAX_SEARCH_STEPS);
        float4 vertical = SearchAlongEdge(INPUT, sam, pos, float2(0, 1), texelSize, CMAA_MAX_SEARCH_STEPS);
        
        lStrength = min(horizontal.z, vertical.z) * CMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR;
    }
    
    // Detect Z-shapes (diagonal edges)
    if (hasDiag1Edge || hasDiag2Edge) {
        float2 diagonalDir = (edgeDiagonal1 > edgeDiagonal2) ? float2(1, 1) : float2(1, -1);
        float4 diagonalSearch = SearchAlongEdge(INPUT, sam, pos, diagonalDir, texelSize, CMAA_MAX_SEARCH_STEPS);
        
        zStrength = diagonalSearch.z * CMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR;
        zDirection = diagonalDir * normalize(diagonalSearch.xy);
    }
    
    // 5. APPLY ANTI-ALIASING
    //-----------------------------------------------------------
    float3 result = center;
    float totalBlendWeight = 0;
    
    // Process Z-shapes (diagonal edges)
    if (zStrength > 0.1f) {
        // Refine direction based on local gradient
        float2 gradient = float2(
            lumaRight - lumaLeft,
            lumaBottom - lumaTop
        );
        
        if (length(gradient) > 0.02) {
            float2 gradientNorm = normalize(gradient);
            zDirection = normalize(lerp(zDirection, gradientNorm, 0.3));
        }
        
        // Adaptive corner rounding
        float2 offset1 = zDirection * texelSize * CMAA_CORNER_ROUNDING * (1.0 + 0.5 * zStrength);
        float2 offset2 = -offset1;
        
        // Higher quality sampling with multiple taps
        float3 color1 = 0, color2 = 0;
        float totalWeight = 0;
        
        for (int i = -1; i <= 1; i++) {
            float weight = (i == 0) ? 0.5 : 0.25;
            float2 perpOffset = float2(-zDirection.y, zDirection.x) * texelSize * i * 0.5;
            color1 += INPUT.SampleLevel(sam, pos + offset1 + perpOffset, 0).rgb * weight;
            color2 += INPUT.SampleLevel(sam, pos + offset2 + perpOffset, 0).rgb * weight;
            totalWeight += weight * 2;
        }
        
        color1 /= totalWeight * 0.5;
        color2 /= totalWeight * 0.5;
        
        float zBlendFactor = min(saturate(zStrength * 1.2), CMAA_MAX_BLEND_FACTOR);
        
        // Color preservation - reduce blending in high-contrast areas
        float contrastFactor = (length(color1 - color2) > 0.5) ? 0.7 : 1.0;
        
        result = lerp(result, (color1 + color2) * 0.5, zBlendFactor * contrastFactor);
        totalBlendWeight += zBlendFactor * contrastFactor;
    }
    
    // Process L-shapes (corners)
    if (lStrength > 0.1f) {
        float blendFactor = min(saturate(lStrength * 0.6f), CMAA_MAX_BLEND_FACTOR);
        
        // Weighted average
        float3 avgColor = center * 2.0;
        float totalWeight = 2.0;
        
        // Diamond pattern sampling
        float3 samples[4] = { left, right, top, bottom };
        
        for (int i = 0; i < 4; i++) {
            float weight = max(1.0 - saturate(length(samples[i] - center) * 2.0), 0.1);
            avgColor += samples[i] * weight;
            totalWeight += weight;
        }
        
        avgColor /= totalWeight;
        result = lerp(result, avgColor, blendFactor);
        totalBlendWeight += blendFactor;
    }
    
    // Process simple edges if not already handled by shapes
    if (totalBlendWeight < 0.2f) {
        float edgeBlendFactor = 0.0f;
        float3 edgeColor = 0.0f;
        
        // Horizontal edge handling
        if (hasHorzEdge) {
            float upDistance = length(center - top);
            float downDistance = length(center - bottom);
            
            float upWeight = 1.0 / (0.01 + upDistance);
            float downWeight = 1.0 / (0.01 + downDistance);
            float totalWeight = upWeight + downWeight;
            
            float3 blendColor = (top * upWeight + bottom * downWeight) / totalWeight;
            float blendStrength = min(saturate(edgeHorizontal * 0.6f), CMAA_MAX_BLEND_FACTOR);
            
            edgeBlendFactor += blendStrength;
            edgeColor += blendColor * blendStrength;
        }
        
        // Vertical edge handling
        if (hasVertEdge) {
            float leftDistance = length(center - left);
            float rightDistance = length(center - right);
            
            float leftWeight = 1.0 / (0.01 + leftDistance);
            float rightWeight = 1.0 / (0.01 + rightDistance);
            float totalWeight = leftWeight + rightWeight;
            
            float3 blendColor = (left * leftWeight + right * rightWeight) / totalWeight;
            float blendStrength = min(saturate(edgeVertical * 0.6f), CMAA_MAX_BLEND_FACTOR);
            
            edgeBlendFactor += blendStrength;
            edgeColor += blendColor * blendStrength;
        }
        
        if (edgeBlendFactor > 0) {
            edgeColor /= edgeBlendFactor;
            edgeBlendFactor = min(edgeBlendFactor, CMAA_MAX_BLEND_FACTOR);
            result = lerp(result, edgeColor, edgeBlendFactor);
            totalBlendWeight += edgeBlendFactor;
        }
    }
    
    // Prevent over-blending
    if (totalBlendWeight > CMAA_MAX_BLEND_FACTOR) {
        result = lerp(center, result, CMAA_MAX_BLEND_FACTOR / totalBlendWeight);
    }
    
    return float4(result, originalColor.a);
}
