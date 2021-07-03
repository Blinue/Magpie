// (FSRCNNX_x2_8-0-4-1_LA) feature map 2


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
};


#define MAGPIE_INPUT_COUNT 1
#include "common.hlsli"


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float left1X = max(0, Coord(0).x - Coord(0).z);
	float left2X = max(0, left1X - Coord(0).z);
	float right1X = min(maxCoord0.x, Coord(0).x + Coord(0).z);
	float right2X = min(maxCoord0.x, right1X + Coord(0).z);
	float top1Y = max(0, Coord(0).y - Coord(0).w);
	float top2Y = max(0, top1Y - Coord(0).w);
	float bottom1Y = min(maxCoord0.y, Coord(0).y + Coord(0).w);
	float bottom2Y = min(maxCoord0.y, bottom1Y + Coord(0).w);

	float4 res = { 0.0165165197104216,0.0061719734221697,-0.0008248710073531,-0.0774794667959213 };
	res += float4(-0.0127812735736370, -0.0146999256685376, 0.0025963818188757, 0.0008133125957102)
		* SampleInput(0, float2(left2X, top2Y)).x;
	res += float4(0.0192508958280087, 0.0089628640562296, 0.0046624913811684, -0.0005601323791780)
		* SampleInput(0, float2(left2X, top1Y)).x;
	res += float4(-0.1021092385053635, -0.0491660982370377, -0.0818324312567711, -0.0719010531902313)
		* SampleInput(0, float2(left2X, Coord(0).y)).x;
	res += float4(0.0166876111179590, -0.0046075899153948, 0.0258100070059299, -0.0235325042158365)
		* SampleInput(0, float2(left2X, bottom1Y)).x;
	res += float4(-0.0028500237967819, -0.0020616643596441, -0.0073093594983220, -0.0034190006554127)
		* SampleInput(0, float2(left2X, bottom2Y)).x;
	res += float4(0.0024815262295306, 0.0222324915230274, -0.0080765523016453, 0.0105959763750434)
		* SampleInput(0, float2(left1X, top2Y)).x;
	res += float4(0.1017390340566635, 0.0138921840116382, 0.0559288635849953, -0.0168517548590899)
		* SampleInput(0, float2(left1X, top1Y)).x;
	res += float4(0.1267367750406265, -0.2365809977054596, 0.4724994897842407, -0.0154752098023891)
		* SampleInput(0, float2(left1X, Coord(0).y)).x;
	res += float4(0.0847241580486298, 0.1127829849720001, -0.0643212646245956, 0.0177757386118174)
		* SampleInput(0, float2(left1X, bottom1Y)).x;
	res += float4(-0.0354492329061031, -0.0234994646161795, 0.0336676724255085, 0.0153558924794197)
		* SampleInput(0, float2(left1X, bottom2Y)).x;
	res += float4(-0.1001686528325081, 0.0175829399377108, -0.0146998856216669, -0.0897502079606056)
		* SampleInput(0, float2(Coord(0).x, top2Y)).x;
	res += float4(0.0973328053951263, -0.5987607836723328, -0.0770601108670235, 0.2343221157789230)
		* SampleInput(0, float2(Coord(0).x, top1Y)).x;
	res += float4(-1.0639246702194214, 0.5335622429847717, -0.2365868240594864, 0.6484431028366089)
		* SampleInputCur(0).x;
	res += float4(-0.0258918590843678, 0.1439655423164368, 0.2597847878932953, -0.5380389094352722)
		* SampleInput(0, float2(Coord(0).x, bottom1Y)).x;
	res += float4(0.0333042629063129, -0.0408495217561722, 0.0026879014912993, 0.0496195442974567)
		* SampleInput(0, float2(Coord(0).x, bottom2Y)).x;
	res += float4(0.0017764334334061, 0.0032939016819000, -0.0121603077277541, -0.0066827093251050)
		* SampleInput(0, float2(right1X, top2Y)).x;
	res += float4(0.0497846752405167, 0.0766935721039772, 0.0505562871694565, 0.0058483541943133)
		* SampleInput(0, float2(right1X, top1Y)).x;
	res += float4(0.6903248429298401, 0.0658241882920265, -0.4562527537345886, -0.0117225451394916)
		* SampleInput(0, float2(right1X, Coord(0).y)).x;
	res += float4(0.1896255612373352, -0.0459045991301537, -0.0380226671695709, -0.0333303771913052)
		* SampleInput(0, float2(right1X, bottom1Y)).x;
	res += float4(-0.0868696048855782, 0.0157926902174950, 0.0011628456413746, 0.0207170285284519)
		* SampleInput(0, float2(right1X, bottom2Y)).x;
	res += float4(0.0130701754242182, -0.0067251212894917, -0.0007082104566507, -0.0017002354143187)
		* SampleInput(0, float2(right2X, top2Y)).x;
	res += float4(0.0029672298114747, -0.0060487915761769, 0.0191176552325487, 0.0520425662398338)
		* SampleInput(0, float2(right2X, top1Y)).x;
	res += float4(-0.0253955777734518, -0.0159530192613602, 0.0304108783602715, -0.0263646803796291)
		* SampleInput(0, float2(right2X, Coord(0).y)).x;
	res += float4(-0.0708072409033775, 0.0109798992052674, 0.0285820439457893, 0.0188453849405050)
		* SampleInput(0, float2(right2X, bottom1Y)).x;
	res += float4(0.0698847994208336, -0.0164128411561251, 0.0043246182613075, -0.0244176983833313)
		* SampleInput(0, float2(right2X, bottom2Y)).x;

	return compressLinear(res, -2, 2);
}
