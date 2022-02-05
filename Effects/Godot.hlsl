// Godot Super Scaling
// 移植自 https://github.com/cybereality/godot-super-scaling/blob/main/SuperScaling/SuperScaling.tres 
//
// This is a high performance but low edge preserving made by cybereality

//!MAGPIE EFFECT
//!VERSION 1


//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;

//!CONSTANT
//!DEFAULT 0.8
//!MIN 0
//!MAX 1
float smoothness;

//!PASS 1
//!BIND INPUT

#define min12(a, b, c, d, e, f, g, h, i, j, k, l) min(min(min(min(a, b), min(c, d)), min(min(e, f), min(g, h))), min(min(i, j), min(k, l)))

// finds the absolute distance from two floats
float float_diff(float float_a, float float_b) {
	return abs(float_a - float_b);
}

// maximum difference for all 3 color channels
float color_diff(float3 color_a, float3 color_b) {
	return max(float_diff(color_a.r, color_b.r), max(float_diff(color_a.g, color_b.g), float_diff(color_a.b, color_b.b)));
}

// simple average of two colors
float3 color_average(float3 color_a, float3 color_b) {
	return lerp(color_a, color_b, 0.5);
}

// partial derivative on x-axis
float2 deriv_x(float2 pos, float2 pixel) {
	float2 pos_plus = pos + float2(pixel.x, 0.0);
	float2 pos_minus = pos - float2(pixel.x, 0.0);
	return int(pixel.x) % 2 == 0 ? (pos_plus - pos) : (pos - pos_minus);
}

// partial derivative on y-axis
float2 deriv_y(float2 pos, float2 pixel) {
	float2 pos_plus = pos + float2(0.0, pixel.y);
	float2 pos_minus = pos - float2(0.0, pixel.y);
	return int(pixel.y) % 2 == 0 ? (pos_plus - pos) : (pos - pos_minus);
}


// take 4 samples in a rotated grid for super sampling
float3 directional_average(float2 pos) {
	// get the colors of all 9 pixels in the 3x3 grid
	float3 pixel_0 = INPUT.Sample(sam, pos).rgb; // center pixel
	float3 pixel_1 = INPUT.Sample(sam, pos + float2(0.0, inputPtY)).rgb; // north pixel
	float3 pixel_2 = INPUT.Sample(sam, pos + float2(0.0, -inputPtY)).rgb; // south pixel
	float3 pixel_3 = INPUT.Sample(sam, pos + float2(inputPtX, 0.0)).rgb; // east pixel
	float3 pixel_4 = INPUT.Sample(sam, pos + float2(-inputPtX, 0.0)).rgb; // west pixel
	float3 pixel_5 = INPUT.Sample(sam, pos + float2(0.0, inputPtY) + float2(inputPtX, 0.0)).rgb; // north-east pixel
	float3 pixel_6 = INPUT.Sample(sam, pos + float2(0.0, inputPtY) + float2(-inputPtX, 0.0)).rgb; // north-west pixel
	float3 pixel_7 = INPUT.Sample(sam, pos + float2(0.0, -inputPtY) + float2(inputPtX, 0.0)).rgb; // south-east pixel
	float3 pixel_8 = INPUT.Sample(sam, pos + float2(0.0, -inputPtY) + float2(-inputPtX, 0.0)).rgb; // south-west pixel

	// find the maximum color difference for each of 12 directions
	float dir_1 = color_diff(pixel_0, pixel_1); // center to north
	float dir_2 = color_diff(pixel_0, pixel_2); // center to south
	float dir_3 = color_diff(pixel_0, pixel_3); // center to east
	float dir_4 = color_diff(pixel_0, pixel_4); // center to west
	float dir_5 = color_diff(pixel_0, pixel_5); // center to north-east
	float dir_6 = color_diff(pixel_0, pixel_6); // center to north-west
	float dir_7 = color_diff(pixel_0, pixel_7); // center to south-east
	float dir_8 = color_diff(pixel_0, pixel_8); // center to south-west
	float dir_9 = color_diff(pixel_1, pixel_4); // north to west
	float dir_10 = color_diff(pixel_1, pixel_3); // north to east
	float dir_11 = color_diff(pixel_2, pixel_3); // south to east
	float dir_12 = color_diff(pixel_2, pixel_4); // south to west

	// find the minimum distance of each of the 12 directions
	float min_dist = min12(dir_1, dir_2, dir_3, dir_4, dir_5, dir_6, dir_7, dir_8, dir_9, dir_10, dir_11, dir_12);

	// get the average color along the minimum direction
	float3 result = pixel_0;

	if (min_dist == dir_1)
		result = color_average(pixel_0, pixel_1); // center to north
	else if (min_dist == dir_2)
		result = color_average(pixel_0, pixel_2); // center to south
	else if (min_dist == dir_3)
		result = color_average(pixel_0, pixel_3); // center to east
	else if (min_dist == dir_4)
		result = color_average(pixel_0, pixel_4); // center to west
	else if (min_dist == dir_5)
		result = color_average(pixel_0, pixel_5); // center to north-east
	else if (min_dist == dir_6)
		result = color_average(pixel_0, pixel_6); // center to north-west
	else if (min_dist == dir_7)
		result = color_average(pixel_0, pixel_7); // center to south-east
	else if (min_dist == dir_8)
		result = color_average(pixel_0, pixel_8); // center to south-west
	else if (min_dist == dir_9)
		result = color_average(pixel_0, color_average(pixel_1, pixel_4)); // north to west
	else if (min_dist == dir_10)
		result = color_average(pixel_0, color_average(pixel_1, pixel_3)); // north to east
	else if (min_dist == dir_11)
		result = color_average(pixel_0, color_average(pixel_2, pixel_3)); // south to east
	else if (min_dist == dir_12)
		result = color_average(pixel_0, color_average(pixel_2, pixel_4)); // south to west
	return result;
}


float4 Pass1(float2 pos) {
    // take 4 samples in a rotated grid for super sampling
	float2 dx = deriv_x(pos, float2(inputPtX, inputPtY));
	float2 dy = deriv_y(pos, float2(inputPtX, inputPtY));

	float3 color = 0;
	color += INPUT.Sample(sam, pos + 0.36 * dx + 0.64 * dy).rgb;
	color += INPUT.Sample(sam, pos - 0.36 * dx - 0.64 * dy).rgb;
	color += INPUT.Sample(sam, pos + 0.64 * dx - 0.36 * dy).rgb;
	color += INPUT.Sample(sam, pos - 0.64 * dx + 0.36 * dy).rgb;
	color *= 0.25;
	
	float3 dir_avg = directional_average(pos);
	color = lerp(dir_avg, color, smoothness);
	
	// draw the new color on the screen
	return float4(color.rgb, 1);
}
