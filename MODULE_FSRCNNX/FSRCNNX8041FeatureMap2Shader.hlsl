// (FSRCNNX_x2_8-0-4-1) feature map 2


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

	float4 res = { 0.0541447550058365,0.0088306749239564,-0.0112389577552676,-0.0127860950306058 };
	res += float4(0.0142660010606050, 0.0137931071221828, 0.0061188107356429, -0.0104134222492576)
		* SampleInput(0, float2(left2X, top2Y)).x;
	res += float4(0.0147292809560895, -0.0289912857115269, 0.0266769435256720, 0.0933856964111328)
		* SampleInput(0, float2(left2X, top1Y)).x;
	res += float4(-0.1734338253736496, 0.1116316691040993, -0.1973157376050949, -0.0581855811178684)
		* SampleInput(0, float2(left2X, Coord(0).y)).x;
	res += float4(0.0347507223486900, -0.0341566652059555, 0.0061667622067034, 0.0075258882716298)
		* SampleInput(0, float2(left2X, bottom1Y)).x;
	res += float4(0.0069884369149804, -0.0194250214844942, 0.0080830128863454, -0.0036874092184007)
		* SampleInput(0, float2(left2X, bottom2Y)).x;
	res += float4(0.0233764201402664, 0.0344744995236397, 0.0162145942449570, 0.0979529991745949)
		* SampleInput(0, float2(left1X, top2Y)).x;
	res += float4(0.1280796974897385, -0.1018339172005653, -0.0132977198809385, -0.0019474622095004)
		* SampleInput(0, float2(left1X, top1Y)).x;
	res += float4(0.4286882579326630, 0.1222677752375603, 0.7046694159507751, 0.0945475697517395)
		* SampleInput(0, float2(left1X, Coord(0).y)).x;
	res += float4(0.1107441782951355, -0.0134433070197701, -0.0174900908023119, -0.1686445474624634)
		* SampleInput(0, float2(left1X, bottom1Y)).x;
	res += float4(0.0321478620171547, 0.0065357843413949, 0.0300805997103453, 0.0420113280415535)
		* SampleInput(0, float2(left1X, bottom2Y)).x;
	res += float4(-0.1240341588854790, 0.0950303301215172, -0.0129648456349969, -0.2681856453418732)
		* SampleInput(0, float2(Coord(0).x, top2Y)).x;
	res += float4(0.4846960902214050, 0.0351924635469913, 0.0223043337464333, -0.1273630708456039)
		* SampleInput(0, float2(Coord(0).x, top1Y)).x;
	res += float4(-1.9379507303237915, -0.2444442063570023, 0.0291962660849094, -0.3835578560829163)
		* SampleInputCur(0).x;
	res += float4(0.6396278142929077, -0.0765938311815262, -0.0552659817039967, 0.4393545985221863)
		* SampleInput(0, float2(Coord(0).x, bottom1Y)).x;
	res += float4(-0.1969728022813797, -0.0607173256576061, 0.0131113547831774, 0.0542017817497253)
		* SampleInput(0, float2(Coord(0).x, bottom2Y)).x;
	res += float4(0.0091696009039879, -0.0031533432193100, -0.0368777588009834, -0.0459998287260532)
		* SampleInput(0, float2(right1X, top2Y)).x;
	res += float4(0.1096992492675781, 0.2597902715206146, 0.0304869692772627, -0.0195200722664595)
		* SampleInput(0, float2(right1X, top1Y)).x;
	res += float4(0.2889648377895355, -0.4275591969490051, -0.7414156794548035, 0.2695442438125610)
		* SampleInput(0, float2(right1X, Coord(0).y)).x;
	res += float4(0.0892018377780914, -0.0229137558490038, 0.0244414471089840, -0.1926898956298828)
		* SampleInput(0, float2(right1X, bottom1Y)).x;
	res += float4(0.0576358586549759, 0.0027846973389387, -0.0036861505359411, -0.0253547113388777)
		* SampleInput(0, float2(right1X, bottom2Y)).x;
	res += float4(0.0159624069929123, 0.0319602824747562, 0.0019470085389912, 0.0089780492708087)
		* SampleInput(0, float2(right2X, top2Y)).x;
	res += float4(0.0552792511880398, 0.0543054342269897, 0.0134062822908163, 0.0545728243887424)
		* SampleInput(0, float2(right2X, top1Y)).x;
	res += float4(-0.1170092225074768, 0.1963327825069427, 0.1503890156745911, 0.1891828328371048)
		* SampleInput(0, float2(right2X, Coord(0).y)).x;
	res += float4(-0.0084421783685684, 0.1297017931938171, -0.0330600887537003, -0.0942063704133034)
		* SampleInput(0, float2(right2X, bottom1Y)).x;
	res += float4(0.0118440408259630, -0.0337875857949257, 0.0055063469335437, 0.0254479162395000)
		* SampleInput(0, float2(right2X, bottom2Y)).x;

	return compressLinear(res, -2, 2);
}
