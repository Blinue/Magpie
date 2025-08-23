// Snapdragon™ Game Super Resolution
// 移植自 https://github.com/SnapdragonStudios/snapdragon-gsr/blob/main/sgsr/v1/include/hlsl/sgsr1_shader_mobile.hlsl

//!MAGPIE EFFECT
//!VERSION 4

//!PARAMETER
//!LABEL Edge Sharpness
//!DEFAULT 2.0
//!MIN 0.0
//!MAX 10.0
//!STEP 0.1
float EdgeSharpness;

//!PARAMETER
//!LABEL Edge Threshold
//!DEFAULT 8.0
//!MIN 0.0
//!MAX 16.0
//!STEP 0.1
float EdgeThreshold;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
Texture2D OUTPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;

//!PASS 1
//!STYLE PS
//!IN INPUT
//!OUT OUTPUT

#define UseEdgeDirection

float fastLanczos2(float x)
{
    float wA = x- float(4.0);
    float wB = x*wA-wA;
    wA *= wA;
    return wB*wA;
}

#if defined(UseEdgeDirection)
float2 weightY(float dx, float dy, float c, float3 data)
#else
float2 weightY(float dx, float dy, float c, float data)
#endif
{
#if defined(UseEdgeDirection)
    float std = data.x;
    float2 dir = data.yz;

    float edgeDis = ((dx*dir.y)+(dy*dir.x));
    float x = (((dx*dx)+(dy*dy))+((edgeDis*edgeDis)*((clamp(((c*c)*std),0.0,1.0)*0.7)+-1.0)));
#else
    float std = data;
    float x = ((dx*dx)+(dy* dy))* float(0.5) + clamp(abs(c)*std, 0.0, 1.0);
#endif

    float w = fastLanczos2(x);
    return float2(w, w * c);
}

float2 edgeDirection(float4 left, float4 right)
{
    float2 dir;
    float RxLz = (right.x + (-left.z));
    float RwLy = (right.w + (-left.y));
    float2 delta;
    delta.x = (RxLz + RwLy);
    delta.y = (RxLz + (-RwLy));
    float lengthInv = rsqrt((delta.x * delta.x+ 3.075740e-05) + (delta.y * delta.y));
    dir.x = (delta.x * lengthInv);
    dir.y = (delta.y * lengthInv);
    return dir;
}

float4 SGSRH(float2 p)
{
    return INPUT.GatherGreen(sam, p);
}

float4 SGSRRGBH(float2 p)
{
    return INPUT.SampleLevel(sam, p, 0);
}

float3 SgsrYuvH(float2 uv, float4 con1)
{
    float3 pix;
    float edgeThreshold = EdgeThreshold / 255.0;
    float edgeSharpness = EdgeSharpness;
    pix = SGSRRGBH(uv).xyz;
    float xCenter;
    xCenter = abs(uv.x+-0.5);
    float yCenter;
    yCenter = abs(uv.y+-0.5);

    float2 imgCoord = ((uv.xy*con1.zw)+ float2(-0.5,0.5));
    float2 imgCoordPixel = floor(imgCoord);
    float2 coord = (imgCoordPixel*con1.xy);
    float2 pl = (imgCoord+(-imgCoordPixel));
    float4  left = SGSRH(coord);

    float edgeVote = abs(left.z - left.y) + abs(pix[1] - left.y)  + abs(pix[1] - left.z) ;
    if (edgeVote > edgeThreshold)
    {
        coord.x += con1.x;

        float4 right = SGSRH(coord + float2(con1.x,  0.0));
        float4 upDown;
        upDown.xy = SGSRH(coord + float2(0.0, -con1.y)).wz;
        upDown.zw = SGSRH(coord + float2(0.0,  con1.y)).yx;

        float mean = (left.y+left.z+right.x+right.w)* float(0.25);
        left = left - float4(mean,mean,mean,mean);
        right = right - float4(mean, mean, mean, mean);
        upDown = upDown - float4(mean, mean, mean, mean);
        float pix_G = pix[1] - mean;

        float sum = (((((abs(left.x)+abs(left.y))+abs(left.z))+abs(left.w))+(((abs(right.x)+abs(right.y))+abs(right.z))+abs(right.w)))+(((abs(upDown.x)+abs(upDown.y))+abs(upDown.z))+abs(upDown.w)));
        float sumMean = 1.014185e+01/sum;
        float std = (sumMean*sumMean);

#if defined(UseEdgeDirection)
        float3 data = float3(std, edgeDirection(left, right));
#else
        float data = std;
#endif

        float2 aWY = weightY(pl.x, pl.y+1.0, upDown.x,data);
        aWY += weightY(pl.x-1.0, pl.y+1.0, upDown.y,data);
        aWY += weightY(pl.x-1.0, pl.y-2.0, upDown.z,data);
        aWY += weightY(pl.x, pl.y-2.0, upDown.w,data);
        aWY += weightY(pl.x+1.0, pl.y-1.0, left.x,data);
        aWY += weightY(pl.x, pl.y-1.0, left.y,data);
        aWY += weightY(pl.x, pl.y, left.z,data);
        aWY += weightY(pl.x+1.0, pl.y, left.w,data);
        aWY += weightY(pl.x-1.0, pl.y-1.0, right.x,data);
        aWY += weightY(pl.x-2.0, pl.y-1.0, right.y,data);
        aWY += weightY(pl.x-2.0, pl.y, right.z,data);
        aWY += weightY(pl.x-1.0, pl.y, right.w,data);

        float finalY = aWY.y/aWY.x;

        float max4 = max(max(left.y,left.z),max(right.x,right.w));
        float min4 = min(min(left.y,left.z),min(right.x,right.w));
        finalY = clamp(edgeSharpness*finalY, min4, max4);

        float deltaY = finalY - pix_G;

        pix = saturate(pix+deltaY);
    }
    return pix;
}

MF4 Pass1(float2 texCoord)
{
    float2 inputSize = GetInputSize();
    float2 inputPt = GetInputPt();
    float4 viewportInfo = float4(inputPt.x, inputPt.y, inputSize.x, inputSize.y);

    return float4(SgsrYuvH(texCoord, viewportInfo), 1);
}

