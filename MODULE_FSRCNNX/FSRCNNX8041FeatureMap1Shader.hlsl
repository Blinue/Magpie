// (FSRCNNX_x2_8-0-4-1) feature map 1


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
};


#define MAGPIE_INPUT_COUNT 1
#include "FSRCNNX8041.hlsli"


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

	float4 res = { -0.1572492271661758, -0.0120896836742759, 0.0061487639322877, -0.2852848768234253 };
	res += float4(-0.0047900392673910, 0.0537447109818459, -0.0000247144635068, 0.0066653941757977)
		* SampleInput(0, float2(left2X, top2Y)).x;
	res += float4(0.0073144687339664, -0.0309004038572311, -0.0109181385487318, -0.0092840325087309)
		* SampleInput(0, float2(left2X, top1Y)).x;
	res += float4(0.0591700896620750, 0.1974907070398331, -0.0197357516735792, -0.0546554848551750)
		* SampleInput(0, float2(left2X, Coord(0).y)).x;
	res += float4(-0.0011764382943511, -0.0299451071768999, 0.0229587312787771, 0.0021908886265010)
		* SampleInput(0, float2(left2X, bottom1Y)).x;
	res += float4(0.0098101310431957, 0.0080995410680771, -0.0030452020000666, -0.0132035519927740)
		* SampleInput(0, float2(left2X, bottom2Y)).x;
	res += float4(-0.0168330334126949, -0.0743711441755295, -0.0259261634200811, 0.0234480481594801)
		* SampleInput(0, float2(left1X, top2Y)).x;
	res += float4(0.0239933785051107, 0.1896541714668274, 0.0207756329327822, -0.0370332375168800)
		* SampleInput(0, float2(left1X, top1Y)).x;
	res += float4(0.0094799501821399, -0.0652511194348335, -0.0004292793164495, -0.0726212188601494)
		* SampleInput(0, float2(left1X, Coord(0).y)).x;
	res += float4(0.0297284796833992, -0.1210186630487442, -0.0202929321676493, -0.0574462898075581)
		* SampleInput(0, float2(left1X, bottom1Y)).x;
	res += float4(-0.0318185277283192, 0.0840775370597839, 0.0110451309010386, 0.0415569432079792)
		* SampleInput(0, float2(left1X, bottom2Y)).x;
	res += float4(-0.0253141783177853, 0.1168256178498268, 0.1159729585051537, 0.0963164269924164)
		* SampleInput(0, float2(Coord(0).x, top2Y)).x;
	res += float4(-0.1103615835309029, -0.0276833958923817, -0.4999594092369080, 0.1053867191076279)
		* SampleInput(0, float2(Coord(0).x, top1Y)).x;
	res += float4(1.1100435256958008, 0.0646764487028122, 0.0154005717486143, 0.8891586661338806)
		* SampleInputCur(0).x;
	res += float4(0.1229330673813820, 0.1719468832015991, 0.5730338096618652, -0.1645544171333313)
		* SampleInput(0, float2(Coord(0).x, bottom1Y)).x;
	res += float4(-0.0090442728251219, -0.3023961782455444, -0.1589493155479431, 0.0418574027717113)
		* SampleInput(0, float2(Coord(0).x, bottom2Y)).x;
	res += float4(0.0031942036002874, -0.1310926079750061, 0.0075543406419456, -0.0016449346439913)
		* SampleInput(0, float2(right1X, top2Y)).x;
	res += float4(-0.0995150282979012, -0.0701921209692955, -0.0130895879119635, 0.1344170123338699)
		* SampleInput(0, float2(right1X, top1Y)).x;
	res += float4(0.0060519003309309, -0.1533465683460236, 0.0114194005727768, 0.0264683905988932)
		* SampleInput(0, float2(right1X, Coord(0).y)).x;
	res += float4(0.0244008023291826, 0.1881769001483917, -0.0206351149827242, -0.0628309547901154)
		* SampleInput(0, float2(right1X, bottom1Y)).x;
	res += float4(0.0075713125988841, 0.0508594363927841, 0.0430423170328140, -0.0124188791960478)
		* SampleInput(0, float2(right1X, bottom2Y)).x;
	res += float4(-0.0166875869035721, -0.0047865519300103, 0.0006719123339280, 0.0316803231835365)
		* SampleInput(0, float2(right2X, top2Y)).x;
	res += float4(-0.0058461269363761, 0.0990798473358154, -0.0177743826061487, -0.0066122291609645)
		* SampleInput(0, float2(right2X, top1Y)).x;
	res += float4(-0.0972401946783066, -0.0225446373224258, -0.0037693574558944, 0.1953062713146210)
		* SampleInput(0, float2(right2X, Coord(0).y)).x;
	res += float4(-0.0216837190091610, -0.1824268400669098, 0.0069816261529922, 0.0283037684857845)
		* SampleInput(0, float2(right2X, bottom1Y)).x;
	res += float4(-0.0025767991319299, 0.0459827110171318, -0.0080216089263558, 0.0084134787321091)
		* SampleInput(0, float2(right2X, bottom2Y)).x;

	return compressLinear(res, -1, 1.5);
}
