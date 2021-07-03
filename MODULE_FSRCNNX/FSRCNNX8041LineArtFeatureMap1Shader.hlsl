// (FSRCNNX_x2_8-0-4-1_LA) feature map 1


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

	float4 res = { -0.3117050230503082,0.1817725896835327,0.0011673698900267,-0.0044658286496997 };
	res += float4(-0.0187959559261799, -0.0206312909722328, 0.0226501729339361, 0.0111862262710929)
		* SampleInput(0, float2(left2X, top2Y)).x;
	res += float4(0.0469042696058750, 0.0428658165037632, -0.0208927169442177, -0.0053485808894038)
		* SampleInput(0, float2(left2X, top1Y)).x;
	res += float4(0.0486242026090622, 0.0268428903073072, -0.1095351055264473, -0.0197027549147606)
		* SampleInput(0, float2(left2X, Coord(0).y)).x;
	res += float4(-0.0301427692174911, -0.0444439016282558, 0.0803908482193947, -0.0072240661829710)
		* SampleInput(0, float2(left2X, bottom1Y)).x;
	res += float4(0.0097448397427797, 0.0132117131724954, -0.0087575586512685, 0.0003270092420280)
		* SampleInput(0, float2(left2X, bottom2Y)).x;
	res += float4(0.0227436870336533, 0.0284603293985128, -0.0899902656674385, 0.0174379274249077)
		* SampleInput(0, float2(left1X, top2Y)).x;
	res += float4(-0.0880827009677887, -0.0890802741050720, 0.3386772871017456, -0.0749290063977242)
		* SampleInput(0, float2(left1X, top1Y)).x;
	res += float4(-0.0832799598574638, -0.1518420130014420, 0.1693033277988434, 0.1514045447111130)
		* SampleInput(0, float2(left1X, Coord(0).y)).x;
	res += float4(0.0490957386791706, 0.0839962288737297, 0.0323486365377903, -0.0491475425660610)
		* SampleInput(0, float2(left1X, bottom1Y)).x;
	res += float4(0.0281097982078791, 0.0267692077904940, -0.0460123419761658, 0.0137899341061711)
		* SampleInput(0, float2(left1X, bottom2Y)).x;
	res += float4(0.0592067055404186, -0.0008030450553633, 0.1280025541782379, -0.0270480886101723)
		* SampleInput(0, float2(Coord(0).x, top2Y)).x;
	res += float4(-0.0784756019711494, -0.0078630214557052, -0.1963789612054825, 0.2132134586572647)
		* SampleInput(0, float2(Coord(0).x, top1Y)).x;
	res += float4(0.9478371739387512, -0.7432878613471985, -0.4691794812679291, -0.4196422100067139)
		* SampleInputCur(0).x;
	res += float4(0.1578149050474167, -0.0874812081456184, 0.1223142221570015, 0.2514914274215698)
		* SampleInput(0, float2(Coord(0).x, bottom1Y)).x;
	res += float4(0.0576529577374458, 0.0775778889656067, 0.0526014007627964, -0.1151828765869141)
		* SampleInput(0, float2(Coord(0).x, bottom2Y)).x;
	res += float4(-0.0459806136786938, -0.0550342053174973, -0.0553226508200169, -0.0042642662301660)
		* SampleInput(0, float2(right1X, top2Y)).x;
	res += float4(0.1346504986286163, 0.1795998811721802, -0.0741422399878502, -0.0004661275597755)
		* SampleInput(0, float2(right1X, top1Y)).x;
	res += float4(-0.0344312079250813, -0.0998986735939980, 0.2834288179874420, 0.1789152175188065)
		* SampleInput(0, float2(right1X, Coord(0).y)).x;
	res += float4(-0.0376542955636978, -0.0137260686606169, -0.2183600962162018, -0.0829529240727425)
		* SampleInput(0, float2(right1X, bottom1Y)).x;
	res += float4(0.0143303163349628, 0.0085790483281016, 0.0312815308570862, 0.0557830408215523)
		* SampleInput(0, float2(right1X, bottom2Y)).x;
	res += float4(0.0196402054280043, 0.0245775021612644, 0.0333996489644051, 0.0064323167316616)
		* SampleInput(0, float2(right2X, top2Y)).x;
	res += float4(-0.0247105974704027, -0.0139399459585547, 0.0039188005030155, 0.0138866743072867)
		* SampleInput(0, float2(right2X, top1Y)).x;
	res += float4(0.0688862130045891, 0.0629303157329559, -0.0323157459497452, -0.1300792843103409)
		* SampleInput(0, float2(right2X, Coord(0).y)).x;
	res += float4(0.0111092608422041, 0.0116711426526308, 0.0460555553436279, 0.0563828162848949)
		* SampleInput(0, float2(right2X, bottom1Y)).x;
	res += float4(-0.0043270774185658, -0.0096766958013177, -0.0235258601605892, -0.0409700050950050)
		* SampleInput(0, float2(right2X, bottom2Y)).x;

	return compressLinear(res, -1, 1.5);
}
