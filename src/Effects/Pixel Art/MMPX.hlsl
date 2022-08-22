// MMPX
// 移植自 https://casual-effects.com/research/McGuire2021PixelArt/index.html

//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH * 2
//!OUTPUT_HEIGHT INPUT_HEIGHT * 2


//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!IN INPUT
//!BLOCK_SIZE 16
//!NUM_THREADS 64


#define src(x, y) INPUT.SampleLevel(sam, float2(x, y) * GetInputPt(), 0).rgb

float luma(float3 C) {
    return C.r + C.g + C.b;
}

bool all_eq2(float3 B, float3 A0, float3 A1) {
    return all(B == A0) && all(B == A1);
}

bool all_eq3(float3 B, float3 A0, float3 A1, float3 A2) {
    return all(B == A0) && all(B == A1) && all(B == A2);
}

bool all_eq4(float3 B, float3 A0, float3 A1, float3 A2, float3 A3) {
    return all(B == A0) && all(B == A1) && all(B == A2) && all(B == A3);
}

bool any_eq3(float3 B, float3 A0, float3 A1, float3 A2) {
    return all(B == A0) || all(B == A1) || all(B == A2);
}

bool none_eq2(float3 B, float3 A0, float3 A1) {
    return any(B != A0) && any(B != A1);
}

bool none_eq4(float3 B, float3 A0, float3 A1, float3 A2, float3 A3) {
    return any(B != A0) && any(B != A1) && any(B != A2) && any(B != A3);
}

void Pass1(uint2 blockStart, uint3 threadId) {
    uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;

    if (!CheckViewport(gxy)) {
        return;
    }

    float srcX = (gxy.x >> 1) + 0.5f;
    float srcY = (gxy.y >> 1) + 0.5f;

    float3 A = src(srcX - 1, srcY - 1), B = src(srcX, srcY - 1), C = src(srcX + 1, srcY - 1);
    float3 D = src(srcX - 1, srcY + 0), E = src(srcX, srcY + 0), F = src(srcX + 1, srcY + 0);
    float3 G = src(srcX - 1, srcY + 1), H = src(srcX, srcY + 1), I = src(srcX + 1, srcY + 1);

    float3 J = E, K = E, L = E, M = E;

    if (any(A != E) || any(B != E) || any(C != E) || any(D != E) || any(F != E) || any(G != E) || any(H != E) || any(I != E)) {
        float3 P = src(srcX, srcY - 2), S = src(srcX, srcY + 2);
        float3 Q = src(srcX - 2, srcY), R = src(srcX + 2, srcY);
        float Bl = luma(B), Dl = luma(D), El = luma(E), Fl = luma(F), Hl = luma(H);

        // 1:1 slope rules
        if ((all(D == B) && any(D != H) && any(D != F)) && (El >= Dl || all(E == A)) && any_eq3(E, A, C, G) && ((El < Dl) || any(A != D) || any(E != P) || any(E != Q))) J = D;
        if ((all(B == F) && any(B != D) && any(B != H)) && (El >= Bl || all(E == C)) && any_eq3(E, A, C, I) && ((El < Bl) || any(C != B) || any(E != P) || any(E != R))) K = B;
        if ((all(H == D) && any(H != F) && any(H != B)) && (El >= Hl || all(E == G)) && any_eq3(E, A, G, I) && ((El < Hl) || any(G != H) || any(E != S) || any(E != Q))) L = H;
        if ((all(F == H) && any(F != B) && any(F != D)) && (El >= Fl || all(E == I)) && any_eq3(E, C, G, I) && ((El < Fl) || any(I != H) || any(E != R) || any(E != S))) M = F;

        // Intersection rules
        if ((any(E != F) && all_eq4(E, C, I, D, Q) && all_eq2(F, B, H)) && (any(F != src(srcX + 3, srcY)))) K = M = F;
        if ((any(E != D) && all_eq4(E, A, G, F, R) && all_eq2(D, B, H)) && (any(D != src(srcX - 3, srcY)))) J = L = D;
        if ((any(E != H) && all_eq4(E, G, I, B, P) && all_eq2(H, D, F)) && (any(H != src(srcX, srcY + 3)))) L = M = H;
        if ((any(E != B) && all_eq4(E, A, C, H, S) && all_eq2(B, D, F)) && (any(B != src(srcX, srcY - 3)))) J = K = B;
        if (Bl < El && all_eq4(E, G, H, I, S) && none_eq4(E, A, D, C, F)) J = K = B;
        if (Hl < El && all_eq4(E, A, B, C, P) && none_eq4(E, D, G, I, F)) L = M = H;
        if (Fl < El && all_eq4(E, A, D, G, Q) && none_eq4(E, B, C, I, H)) K = M = F;
        if (Dl < El && all_eq4(E, C, F, I, R) && none_eq4(E, B, A, G, H)) J = L = D;

        // 2:1 slope rules
        if (any(H != B)) {
            if (any(H != A) && any(H != E) && any(H != C)) {
                if (all_eq3(H, G, F, R) && none_eq2(H, D, src(srcX + 2, srcY - 1))) L = M;
                if (all_eq3(H, I, D, Q) && none_eq2(H, F, src(srcX - 2, srcY - 1))) M = L;
            }

            if (any(B != I) && any(B != G) && any(B != E)) {
                if (all_eq3(B, A, F, R) && none_eq2(B, D, src(srcX + 2, srcY + 1))) J = K;
                if (all_eq3(B, C, D, Q) && none_eq2(B, F, src(srcX - 2, srcY + 1))) K = J;
            }
        } // H !== B

        if (any(F != D)) {
            if (any(D != I) && any(D != E) && any(D != C)) {
                if (all_eq3(D, A, H, S) && none_eq2(D, B, src(srcX + 1, srcY + 2))) J = L;
                if (all_eq3(D, G, B, P) && none_eq2(D, H, src(srcX + 1, srcY - 2))) L = J;
            }

            if (any(F != E) && any(F != A) && any(F != G)) {
                if (all_eq3(F, C, H, S) && none_eq2(F, B, src(srcX - 1, srcY + 2))) K = M;
                if (all_eq3(F, I, B, P) && none_eq2(F, H, src(srcX - 1, srcY - 2))) M = K;
            }
        } // F !== D
    } // not constant

    // Write four pixels at once
    WriteToOutput(gxy, J);

    ++gxy.x;
    if (CheckViewport(gxy)) {
        WriteToOutput(gxy, K);
    }

    ++gxy.y;
    if (CheckViewport(gxy)) {
        WriteToOutput(gxy, M);
    }

    --gxy.x;
    if (CheckViewport(gxy)) {
        WriteToOutput(gxy, L);
    }
}
