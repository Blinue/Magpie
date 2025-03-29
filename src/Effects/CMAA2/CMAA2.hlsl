//!MAGPIE EFFECT
//!VERSION 4

//============================================================================
// CMAA2 - Conservative Morphological Anti-Aliasing 2 (Ultra Quality)
//============================================================================
// A high-quality anti-aliasing solution for Magpie based on Intel's CMAA2
// This implementation provides edge-directed anti-aliasing with shape detection
//
// Originally developed by Intel:
// https://github.com/GameTechDev/CMAA2
//
// Ultra quality preset with enhanced edge detection and shape processing
//============================================================================

//!TEXTURE
Texture2D INPUT;
//!TEXTURE
Texture2D OUTPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam;

//!COMMON
// Adjustable parameters - ULTRA QUALITY PRESET
//--------------------------------------
// Edge detection threshold - lower values detect more edges
// Further reduced for ultra-quality anti-aliasing
#define CMAA_EDGE_THRESHOLD 0.045

// Controls how much local contrast affects edge importance
// Increased for better handling of complex edge patterns
#define CMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR 1.1

// Controls how much corners are rounded during anti-aliasing
// Increased for smoother corner transitions
#define CMAA_CORNER_ROUNDING 0.35

// Maximum blend factor - prevents over-blurring
// Carefully balanced for smoothing without loss of detail
#define CMAA_MAX_BLEND_FACTOR 0.75

// Early exit threshold for subtle edges
// Further lowered to catch very subtle aliasing artifacts
#define CMAA_EARLY_EXIT_THRESHOLD 0.005

// Maximum steps when searching along edges
// Significantly increased for ultra-quality detection of longer edges
#define CMAA_MAX_SEARCH_STEPS 32

// Multi-directional search steps
// New parameter for enhanced edge tracking
#define CMAA_SECONDARY_SEARCH_STEPS 16

// Enhanced color matching tolerance 
// New parameter for improved edge handling
#define CMAA_COLOR_MATCHING_TOLERANCE 0.05

// Helper function to calculate perceptual luminance from RGB
// Using Rec. 709 coefficients for accurate perceptual brightness
float ComputeLuma(float3 color) {
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

// Enhanced luminance calculation that accounts for color differences
float ComputeEnhancedLuma(float3 color1, float3 color2) {
    float lumaDiff = ComputeLuma(color1) - ComputeLuma(color2);
    float colorDiff = length(color1 - color2);
    
    // Combine luma and color differences for better edge detection
    return abs(lumaDiff) + colorDiff * 0.3;
}

// Enhanced search along an edge direction
float4 SearchAlongEdge(Texture2D tex, SamplerState samp, float2 uv, float2 direction, float2 texelSize, int maxSteps) {
    float edgeStrength = 0.0f;
    float2 finalOffset = float2(0, 0);
    float stepCount = 0;
    float consistencyFactor = 1.0f;
    
    float3 originalColor = tex.SampleLevel(samp, uv, 0).rgb;
    float3 prevSampleColor = originalColor;
    
    for (int i = 1; i <= maxSteps; i++) {
        float2 samplePos = uv + direction * i * texelSize;
        
        // Sample neighborhood at this position
        float3 colorCenter = tex.SampleLevel(samp, samplePos, 0).rgb;
        float3 colorLeft = tex.SampleLevel(samp, samplePos + float2(-texelSize.x, 0), 0).rgb;
        float3 colorRight = tex.SampleLevel(samp, samplePos + float2(texelSize.x, 0), 0).rgb;
        float3 colorTop = tex.SampleLevel(samp, samplePos + float2(0, -texelSize.y), 0).rgb;
        float3 colorBottom = tex.SampleLevel(samp, samplePos + float2(0, texelSize.y), 0).rgb;
        
        // Enhanced edge detection using both luma and color differences
        float edgeHorizontal = ComputeEnhancedLuma(colorTop, colorCenter) + ComputeEnhancedLuma(colorBottom, colorCenter);
        float edgeVertical = ComputeEnhancedLuma(colorLeft, colorCenter) + ComputeEnhancedLuma(colorRight, colorCenter);
        
        // Compute overall edge strength
        float sampleStrength = max(edgeHorizontal, edgeVertical);
        
        // Check color consistency along the edge (prevents crossing different edges)
        float colorConsistency = 1.0f - saturate(length(colorCenter - prevSampleColor) * 3.0f);
        consistencyFactor = min(consistencyFactor, colorConsistency);
        
        if (sampleStrength > CMAA_EDGE_THRESHOLD && consistencyFactor > 0.3f) {
            // Weight by distance (closer samples have more influence)
            float distanceWeight = 1.0f / (1.0f + i * 0.05f);
            float weightedStrength = sampleStrength * distanceWeight;
            
            if (weightedStrength > edgeStrength * 0.7f) {
                edgeStrength = max(edgeStrength, weightedStrength);
                finalOffset = direction * i * texelSize;
                stepCount = i;
                
                // Store for consistency check
                prevSampleColor = colorCenter;
            }
        } else if (i > 4 && consistencyFactor < 0.3f) {
            // Early termination if we're crossing a different edge
            break;
        }
    }
    
    // Scale strength by consistency factor
    edgeStrength *= consistencyFactor;
    
    return float4(finalOffset.x, finalOffset.y, edgeStrength, stepCount);
}

// Enhanced multi-directional search
float4 MultiDirectionalSearch(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize) {
    // Define 8 search directions for comprehensive edge detection
    const float2 directions[8] = {
        float2(1, 0),    // Right
        float2(-1, 0),   // Left
        float2(0, 1),    // Down
        float2(0, -1),   // Up
        float2(1, 1),    // Bottom-right
        float2(-1, -1),  // Top-left
        float2(1, -1),   // Top-right
        float2(-1, 1)    // Bottom-left
    };
    
    float4 bestSearch = float4(0, 0, 0, 0);
    
    // Search in each direction
    for (int i = 0; i < 8; i++) {
        float4 search = SearchAlongEdge(tex, samp, uv, directions[i], texelSize, 
                                      (i < 4) ? CMAA_MAX_SEARCH_STEPS : CMAA_SECONDARY_SEARCH_STEPS);
        
        // Keep the strongest edge
        if (search.z > bestSearch.z) {
            bestSearch = search;
        }
    }
    
    return bestSearch;
}

//!PASS 1
//!DESC CMAA2 Anti-aliasing (Ultra Quality)
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
    
    // Expanded 5x5 neighborhood sampling for higher quality
    // 3x3 core neighborhood
    float3 top = INPUT.SampleLevel(sam, pos + float2(0, -texelSize.y), 0).rgb;
    float3 right = INPUT.SampleLevel(sam, pos + float2(texelSize.x, 0), 0).rgb;
    float3 bottom = INPUT.SampleLevel(sam, pos + float2(0, texelSize.y), 0).rgb;
    float3 left = INPUT.SampleLevel(sam, pos + float2(-texelSize.x, 0), 0).rgb;
    
    float3 topLeft = INPUT.SampleLevel(sam, pos + float2(-texelSize.x, -texelSize.y), 0).rgb;
    float3 topRight = INPUT.SampleLevel(sam, pos + float2(texelSize.x, -texelSize.y), 0).rgb;
    float3 bottomLeft = INPUT.SampleLevel(sam, pos + float2(-texelSize.x, texelSize.y), 0).rgb;
    float3 bottomRight = INPUT.SampleLevel(sam, pos + float2(texelSize.x, texelSize.y), 0).rgb;
    
    // Extended samples for better pattern detection
    float3 top2 = INPUT.SampleLevel(sam, pos + float2(0, -2 * texelSize.y), 0).rgb;
    float3 right2 = INPUT.SampleLevel(sam, pos + float2(2 * texelSize.x, 0), 0).rgb;
    float3 bottom2 = INPUT.SampleLevel(sam, pos + float2(0, 2 * texelSize.y), 0).rgb;
    float3 left2 = INPUT.SampleLevel(sam, pos + float2(-2 * texelSize.x, 0), 0).rgb;
    
    // 2. CALCULATE LUMINANCE VALUES WITH ENHANCED COLOR PERCEPTION
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
    
    // Extended luma calculations
    float lumaTop2 = ComputeLuma(top2);
    float lumaRight2 = ComputeLuma(right2);
    float lumaBottom2 = ComputeLuma(bottom2);
    float lumaLeft2 = ComputeLuma(left2);
    
    // Adaptive local contrast enhancement for sharper edge detection
    // Calculate local contrast range
    float minLuma = min(min(min(lumaTop, lumaBottom), min(lumaLeft, lumaRight)), lumaCenter);
    float maxLuma = max(max(max(lumaTop, lumaBottom), max(lumaLeft, lumaRight)), lumaCenter);
    float lumaRange = maxLuma - minLuma;
    
    // Apply adaptive sharpening based on local contrast
    float adaptiveSharpen = 0.3f - 0.2f * saturate(lumaRange * 4.0f);
    float lumaSharpened = lumaCenter * (1.0f + 4.0f * adaptiveSharpen) - 
                         (lumaTopLeft + lumaTopRight + lumaBottomLeft + lumaBottomRight) * adaptiveSharpen;
    
    // Blend sharpened luma with original for enhanced edge detection
    lumaCenter = lerp(lumaCenter, lumaSharpened, 0.3f);
    
    // 3. DETECT EDGES WITH ENHANCED PRECISION
    //-----------------------------------------------------------
    // Primary edge detection (immediate neighbors)
    float edgeHorizontal = abs(lumaLeft - lumaCenter) + abs(lumaRight - lumaCenter);
    float edgeVertical = abs(lumaTop - lumaCenter) + abs(lumaBottom - lumaCenter);
    float edgeDiagonal1 = abs(lumaTopLeft - lumaCenter) + abs(lumaBottomRight - lumaCenter);
    float edgeDiagonal2 = abs(lumaTopRight - lumaCenter) + abs(lumaBottomLeft - lumaCenter);
    
    // Secondary edge detection (extended neighbors for smoother gradients)
    float edgeHorizontalExt = abs(lumaLeft2 - lumaLeft) + abs(lumaRight2 - lumaRight);
    float edgeVerticalExt = abs(lumaTop2 - lumaTop) + abs(lumaBottom2 - lumaBottom);
    
    // Combine primary and extended edge detection (with lower weight for extended)
    edgeHorizontal += edgeHorizontalExt * 0.3f;
    edgeVertical += edgeVerticalExt * 0.3f;
    
    // Early exit if no significant edges are found
    if (max(max(edgeHorizontal, edgeVertical), max(edgeDiagonal1, edgeDiagonal2)) <= CMAA_EARLY_EXIT_THRESHOLD) {
        return originalColor;
    }
    
    // Determine edge types with adaptive thresholds based on local contrast
    float adaptiveThreshold = CMAA_EDGE_THRESHOLD * (0.8f + 0.2f * saturate(lumaRange * 2.0f));
    
    bool hasHorzEdge = edgeHorizontal >= adaptiveThreshold;
    bool hasVertEdge = edgeVertical >= adaptiveThreshold;
    bool hasDiag1Edge = edgeDiagonal1 >= adaptiveThreshold;
    bool hasDiag2Edge = edgeDiagonal2 >= adaptiveThreshold;
    
    // If no edges were found, return the original color
    if (!hasHorzEdge && !hasVertEdge && !hasDiag1Edge && !hasDiag2Edge) {
        return originalColor;
    }
    
    // 4. ENHANCED SHAPE DETECTION
    //-----------------------------------------------------------
    // Multi-directional edge search for comprehensive edge detection
    float4 edgeSearch = MultiDirectionalSearch(INPUT, sam, pos, texelSize);
    float edgeStrength = edgeSearch.z;
    float2 edgeOffset = float2(edgeSearch.x, edgeSearch.y);
    
    // Determine if we have a complex edge pattern requiring special handling
    bool isComplex = (hasHorzEdge && hasVertEdge) || (hasDiag1Edge && hasDiag2Edge);
    
    // Adaptive pattern recognition
    float zStrength = 0.0f;
    float lStrength = 0.0f;
    float2 zDirection = float2(0, 0);
    
    // Detect L-junctions with enhanced accuracy (horizontal + vertical edges)
    if (hasHorzEdge && hasVertEdge) {
        float4 horizontal = SearchAlongEdge(INPUT, sam, pos, float2(1, 0), texelSize, CMAA_MAX_SEARCH_STEPS);
        float4 vertical = SearchAlongEdge(INPUT, sam, pos, float2(0, 1), texelSize, CMAA_MAX_SEARCH_STEPS);
        
        // Analyze shape consistency
        bool isLShaped = horizontal.w > 3 && vertical.w > 3;
        
        if (isLShaped) {
            // Enhanced L-junction strength calculation
            lStrength = min(horizontal.z, vertical.z) * CMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR;
            
            // Adjust strength based on junction angle
            float2 hDir = normalize(float2(horizontal.xy));
            float2 vDir = normalize(float2(vertical.xy));
            float dotProduct = abs(dot(hDir, vDir));
            
            // Strengthen L-detection when vectors are perpendicular (dot product near 0)
            lStrength *= (1.0f - dotProduct);
        }
    }
    
    // Detect Z-shapes with enhanced accuracy (diagonal edges)
    if (hasDiag1Edge || hasDiag2Edge) {
        float2 diagonalDir = (edgeDiagonal1 > edgeDiagonal2) ? float2(1, 1) : float2(1, -1);
        float4 diagonalSearch = SearchAlongEdge(INPUT, sam, pos, diagonalDir, texelSize, CMAA_MAX_SEARCH_STEPS);
        
        zStrength = diagonalSearch.z * CMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR;
        zDirection = diagonalDir * normalize(diagonalSearch.xy);
        
        // Enhance Z detection with orthogonal search
        float2 orthogonalDir = float2(-diagonalDir.y, diagonalDir.x);
        float4 orthogonalSearch = SearchAlongEdge(INPUT, sam, pos, orthogonalDir, texelSize, CMAA_SECONDARY_SEARCH_STEPS);
        
        // If we found strong edges in both directions, this is a crosspoint, not a Z
        if (orthogonalSearch.z > zStrength * 0.5f) {
            zStrength *= 0.8f; // Reduce Z strength at crosspoints
        }
    }
    
    // 5. APPLY ENHANCED ANTI-ALIASING WITH QUALITY OPTIMIZATIONS
    //-----------------------------------------------------------
    float3 result = center;
    float totalBlendWeight = 0;
    
    // Process Z-shapes (diagonal edges) with enhanced quality
    if (zStrength > 0.1f) {
        // Refine direction based on local gradient and color consistency
        float2 gradient = float2(
            lumaRight - lumaLeft,
            lumaBottom - lumaTop
        );
        
        if (length(gradient) > 0.01) {
            float2 gradientNorm = normalize(gradient);
            zDirection = normalize(lerp(zDirection, gradientNorm, 0.4));
        }
        
        // Adaptive multi-sample corner rounding
        float adaptiveCornerRounding = CMAA_CORNER_ROUNDING * (1.0f + 0.5f * zStrength);
        
        // Multi-tap sampling for higher quality blending
        const int numSamples = 3; // Increased from original
        float3 color1 = 0, color2 = 0;
        float totalWeight = 0;
        
        for (int i = -(numSamples-1)/2; i <= (numSamples-1)/2; i++) {
            float weight = 1.0f - abs(i) / (float(numSamples) * 0.5f);
            
            // Primary direction sampling
            float2 offset1 = zDirection * texelSize * adaptiveCornerRounding;
            float2 offset2 = -offset1;
            
            // Perpendicular offset for better area coverage
            float2 perpOffset = float2(-zDirection.y, zDirection.x) * texelSize * i * 0.7f;
            
            // Enhanced anisotropic sampling
            color1 += INPUT.SampleLevel(sam, pos + offset1 + perpOffset, 0).rgb * weight;
            color2 += INPUT.SampleLevel(sam, pos + offset2 + perpOffset, 0).rgb * weight;
            totalWeight += weight * 2;
        }
        
        color1 /= totalWeight * 0.5f;
        color2 /= totalWeight * 0.5f;
        
        // Adaptive strength based on pattern confidence
        float patternConfidence = saturate(zStrength * 2.0f - 0.2f);
        float zBlendFactor = min(saturate(zStrength * 1.4f * patternConfidence), CMAA_MAX_BLEND_FACTOR);
        
        // Color-aware blending - preserve details in high-contrast or colored edges
        float colorDiff = length(color1 - color2);
        float colorPreservation = saturate(1.0f - colorDiff * 2.0f);
        
        // Calculate center-relative color preservation
        float centerColorDiff1 = length(center - color1);
        float centerColorDiff2 = length(center - color2);
        float centerColorPreservation = saturate(1.0f - min(centerColorDiff1, centerColorDiff2) * 3.0f);
        
        // Composite preservation factors
        float contrastFactor = lerp(0.5f, 1.0f, colorPreservation * centerColorPreservation);
        
        result = lerp(result, (color1 + color2) * 0.5f, zBlendFactor * contrastFactor);
        totalBlendWeight += zBlendFactor * contrastFactor;
    }
    
    // Process L-shapes (corners) with enhanced quality
    if (lStrength > 0.1f) {
        float adaptiveBlend = min(saturate(lStrength * 0.8f), CMAA_MAX_BLEND_FACTOR);
        
        // Enhanced corner detection samples
        float3 avgColor = center * 2.0f;
        float totalWeight = 2.0f;
        
        // Create an enhanced sampling pattern (X-shaped)
        float3 samples[8] = { 
            left, right, top, bottom,
            topLeft, topRight, bottomLeft, bottomRight
        };
        float weights[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 0.7f, 0.7f, 0.7f, 0.7f };
        
        for (int i = 0; i < 8; i++) {
            // Adaptive weighting based on color similarity
            float colorSimilarity = saturate(1.0f - length(samples[i] - center) * 2.0f);
            float weight = weights[i] * (0.2f + 0.8f * colorSimilarity);
            
            avgColor += samples[i] * weight;
            totalWeight += weight;
        }
        
        avgColor /= totalWeight;
        
        // Adaptive corner strength
        float cornerConfidence = saturate(lStrength * 1.5f - 0.1f);
        float lBlendFactor = adaptiveBlend * cornerConfidence;
        
        result = lerp(result, avgColor, lBlendFactor);
        totalBlendWeight += lBlendFactor;
    }
    
    // Process simple edges with enhanced handling for the most common case
    if (totalBlendWeight < 0.15f) {
        float edgeBlendFactor = 0.0f;
        float3 edgeColor = 0.0f;
        
        // Enhanced horizontal edge handling
        if (hasHorzEdge) {
            // Compute directional gradients
            float topGrad = abs(lumaTop2 - lumaTop);
            float bottomGrad = abs(lumaBottom2 - lumaBottom);
            
            // Distance-based weighting
            float upDistance = ComputeEnhancedLuma(center, top);
            float downDistance = ComputeEnhancedLuma(center, bottom);
            
            float upWeight = 1.0f / (0.01f + upDistance) * (1.0f + topGrad);
            float downWeight = 1.0f / (0.01f + downDistance) * (1.0f + bottomGrad);
            float totalWeight = upWeight + downWeight;
            
            float3 blendColor = (top * upWeight + bottom * downWeight) / totalWeight;
            float edgeIntensity = saturate((edgeHorizontal - CMAA_EDGE_THRESHOLD) / (0.1f + CMAA_EDGE_THRESHOLD));
            float blendStrength = min(saturate(edgeIntensity * 0.8f), CMAA_MAX_BLEND_FACTOR);
            
            edgeBlendFactor += blendStrength;
            edgeColor += blendColor * blendStrength;
        }
        
        // Enhanced vertical edge handling
        if (hasVertEdge) {
            // Compute directional gradients
            float leftGrad = abs(lumaLeft2 - lumaLeft);
            float rightGrad = abs(lumaRight2 - lumaRight);
            
            // Distance-based weighting
            float leftDistance = ComputeEnhancedLuma(center, left);
            float rightDistance = ComputeEnhancedLuma(center, right);
            
            float leftWeight = 1.0f / (0.01f + leftDistance) * (1.0f + leftGrad);
            float rightWeight = 1.0f / (0.01f + rightDistance) * (1.0f + rightGrad);
            float totalWeight = leftWeight + rightWeight;
            
            float3 blendColor = (left * leftWeight + right * rightWeight) / totalWeight;
            float edgeIntensity = saturate((edgeVertical - CMAA_EDGE_THRESHOLD) / (0.1f + CMAA_EDGE_THRESHOLD));
            float blendStrength = min(saturate(edgeIntensity * 0.8f), CMAA_MAX_BLEND_FACTOR);
            
            edgeBlendFactor += blendStrength;
            edgeColor += blendColor * blendStrength;
        }
        
        if (edgeBlendFactor > 0) {
            edgeColor /= edgeBlendFactor;
            
            // Color preservation - adaptive blend based on color difference
            float colorDifference = length(edgeColor - center);
            float preservationFactor = saturate(1.0f - colorDifference * 2.0f);
            float finalEdgeBlend = min(edgeBlendFactor, CMAA_MAX_BLEND_FACTOR) * preservationFactor;
            
            result = lerp(result, edgeColor, finalEdgeBlend);
            totalBlendWeight += finalEdgeBlend;
        }
    }
    
    // Prevent over-blending with intelligent capping
    if (totalBlendWeight > CMAA_MAX_BLEND_FACTOR) {
        // Smoother transition when capping blend weight
        float smoothCap = saturate(CMAA_MAX_BLEND_FACTOR / totalBlendWeight);
        smoothCap = pow(smoothCap, 0.75f); // Softens the transition
        
        result = lerp(center, result, smoothCap * CMAA_MAX_BLEND_FACTOR);
    }
    
    // Final color preservation check - don't blur high contrast edges too much
    float finalColorDiff = length(result - center);
    if (finalColorDiff > 0.3f) {
        // Reduce blending to preserve important details
        float preservationFactor = saturate(1.0f - (finalColorDiff - 0.3f) * 2.0f);
        result = lerp(center, result, preservationFactor);
    }
    
    return float4(result, originalColor.a);
}
