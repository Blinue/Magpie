// Conv-4x3x3x8 (2)
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Upscale%2BDeblur/Anime4K_Upscale_CNN_M_x2_Deblur.glsl
//
// Anime4K-v3.1-Upscale(x2)-CNN(M)-Conv-4x3x3x8


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_COMPLEX
#define MAGPIE_USE_SAMPLE_INPUT
#include "Anime4K.hlsli"


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float left1X = GetCheckedLeft(1);
	float right1X = GetCheckedRight(1);
	float top1Y = GetCheckedTop(1);
	float bottom1Y = GetCheckedBottom(1);

	// [ a, d, g ]
	// [ b, e, h ]
	// [ c, f, i ]
	float4 a = Uncompress(SampleInputRGBANoCheck(0, float2(left1X, top1Y)));
	float4 b = Uncompress(SampleInputRGBANoCheck(0, float2(left1X, coord.y)));
	float4 c = Uncompress(SampleInputRGBANoCheck(0, float2(left1X, bottom1Y)));
	float4 d = Uncompress(SampleInputRGBANoCheck(0, float2(coord.x, top1Y)));
	float4 e = Uncompress(SampleInputRGBACur(0));
	float4 f = Uncompress(SampleInputRGBANoCheck(0, float2(coord.x, bottom1Y)));
	float4 g = Uncompress(SampleInputRGBANoCheck(0, float2(right1X, top1Y)));
	float4 h = Uncompress(SampleInputRGBANoCheck(0, float2(right1X, coord.y)));
	float4 i = Uncompress(SampleInputRGBANoCheck(0, float2(right1X, bottom1Y)));

	float4 na = -min(a, ZEROS4);
	float4 nb = -min(b, ZEROS4);
	float4 nc = -min(c, ZEROS4);
	float4 nd = -min(d, ZEROS4);
	float4 ne = -min(e, ZEROS4);
	float4 nf = -min(f, ZEROS4);
	float4 ng = -min(g, ZEROS4);
	float4 nh = -min(h, ZEROS4);
	float4 ni = -min(i, ZEROS4);

	a = max(a, ZEROS4);
	b = max(b, ZEROS4);
	c = max(c, ZEROS4);
	d = max(d, ZEROS4);
	e = max(e, ZEROS4);
	f = max(f, ZEROS4);
	g = max(g, ZEROS4);
	h = max(h, ZEROS4);
	i = max(i, ZEROS4);

	float s = 0.018682314 * a.x + 0.44203937 * b.x + 0.011201117 * c.x + 0.1707163 * d.x + -0.13163331 * e.x + 0.10832957 * f.x + -0.25094667 * g.x + -0.37710962 * h.x + -0.099689476 * i.x;
	float t = -0.0066981018 * a.y + 0.26694995 * b.y + 0.057795994 * c.y + 0.045658633 * d.y + 0.86929697 * e.y + -0.8076145 * f.y + 0.048658714 * g.y + -0.082118966 * h.y + -0.45287862 * i.y;
	float u = -0.27032706 * a.z + 0.15319459 * b.z + -0.18298155 * c.z + 0.39646947 * d.z + 0.44529167 * e.z + -0.03776809 * f.z + -0.23567463 * g.z + -0.088091925 * h.z + -0.060605783 * i.z;
	float v = -0.17260903 * a.w + 0.29680675 * b.w + -0.11133945 * c.w + 0.7379717 * d.w + 0.57794976 * e.w + 0.15571105 * f.w + 0.032278296 * g.w + -0.45364308 * h.w + 0.011278548 * i.w;
	float w = 0.23664898 * na.x + -0.20770212 * nb.x + 0.08866236 * nc.x + -0.3988166 * nd.x + -0.06265793 * ne.x + -0.23789996 * nf.x + -0.073015444 * ng.x + 0.3039035 * nh.x + 0.022692805 * ni.x;
	float x = 0.08930171 * na.y + -0.58211017 * nb.y + -0.1497316 * nc.y + -0.476922 * nd.y + 0.16330953 * ne.y + -0.24215609 * nf.y + 0.022338377 * ng.y + -0.24107097 * nh.y + 0.1570652 * ni.y;
	float y = 0.008214529 * na.z + 0.16042182 * nb.z + -0.14862658 * nc.z + -0.3509392 * nd.z + -0.9592652 * ne.z + -0.24547556 * nf.z + 0.3663963 * ng.z + 0.72054815 * nh.z + 0.29660952 * ni.z;
	float z = 0.28773922 * na.w + -0.4086489 * nb.w + 0.20410532 * nc.w + -0.36254752 * nd.w + -0.44383284 * ne.w + -0.047999498 * nf.w + -0.038450666 * ng.w + 0.49856558 * nh.w + 0.14509656 * ni.w;
	float o = s + t + u + v + w + x + y + z + 0.047628842;
	s = 0.1842036 * a.x + 0.69354624 * b.x + 0.2072707 * c.x + 0.2558653 * d.x + 0.9648104 * e.x + 0.51031196 * f.x + -0.107327186 * g.x + 0.0013468182 * h.x + -0.019195512 * i.x;
	t = -0.28576428 * a.y + -0.42089957 * b.y + -0.47228622 * c.y + -0.098811954 * d.y + -1.2135493 * e.y + 0.59586686 * f.y + -0.002664521 * g.y + -0.3705708 * h.y + -0.37632635 * i.y;
	u = -0.3551953 * a.z + 0.4631558 * b.z + -0.49070838 * c.z + 0.286012 * d.z + -0.3025882 * e.z + -0.44014844 * f.z + -0.1937611 * g.z + 0.3830139 * h.z + 0.36162966 * i.z;
	v = -0.3278485 * a.w + 0.75486267 * b.w + 0.43210003 * c.w + -0.0147996545 * d.w + -0.23216681 * e.w + 0.2740509 * f.w + 0.042906776 * g.w + 0.05781658 * h.w + -0.0086696 * i.w;
	w = 0.28949758 * na.x + 0.90602577 * nb.x + 0.5258729 * nc.x + 0.03663859 * nd.x + -0.5505775 * ne.x + -0.29051554 * nf.x + -0.076815076 * ng.x + 0.19885658 * nh.x + -0.087935984 * ni.x;
	x = 0.70628744 * na.y + -0.22958279 * nb.y + 0.08402731 * nc.y + 0.15837549 * nd.y + 0.39830247 * ne.y + 1.1849017 * nf.y + 0.35547593 * ng.y + -0.202646 * nh.y + -0.24137132 * ni.y;
	y = 0.08127177 * na.z + -0.7835795 * nb.z + -0.78620285 * nc.z + -0.48470613 * nd.z + 0.4768066 * ne.z + -0.87363476 * nf.z + 0.11905553 * ng.z + -0.11505627 * nh.z + -0.110398 * ni.z;
	z = -0.062861845 * na.w + 0.56250936 * nb.w + -0.08572646 * nc.w + -0.03632719 * nd.w + -0.31594795 * ne.w + -0.17919087 * nf.w + -0.13694055 * ng.w + 0.27909452 * nh.w + 0.19670367 * ni.w;
	float p = s + t + u + v + w + x + y + z + -0.07143907;
	s = 0.05651364 * a.x + 0.24637659 * b.x + -0.16196722 * c.x + 0.25773972 * d.x + 0.33928764 * e.x + 0.30295405 * f.x + 0.25079685 * g.x + -0.026224324 * h.x + 0.10382699 * i.x;
	t = -0.03018759 * a.y + -0.32138908 * b.y + -0.22939964 * c.y + -0.06819758 * d.y + 0.60410833 * e.y + 0.6038164 * f.y + -0.062554374 * g.y + -0.22067818 * h.y + -0.33446926 * i.y;
	u = -0.11069349 * a.z + 0.22467056 * b.z + -0.22629468 * c.z + -0.036879618 * d.z + 0.24277517 * e.z + 0.013148594 * f.z + 0.17209497 * g.z + 0.1183592 * h.z + -0.018353969 * i.z;
	v = -0.0933339 * a.w + -0.36371025 * b.w + -0.41810217 * c.w + 0.065053195 * d.w + 0.23782931 * e.w + 0.3557887 * f.w + 0.011487943 * g.w + -0.32776782 * h.w + -0.22873801 * i.w;
	w = 0.07554202 * na.x + -0.09785583 * nb.x + 0.39642334 * nc.x + -0.14174528 * nd.x + 0.40234557 * ne.x + 0.10772596 * nf.x + -0.100343354 * ng.x + 0.3403603 * nh.x + 0.08025647 * ni.x;
	x = 0.14248708 * na.y + -0.16136892 * nb.y + 0.52202225 * nc.y + -0.26288292 * nd.y + -0.7474859 * ne.y + -0.7591303 * nf.y + 0.16527072 * ng.y + 0.2446596 * nh.y + -0.1825532 * ni.y;
	y = 0.09868614 * na.z + -0.10381665 * nb.z + -0.25812206 * nc.z + -0.08914129 * nd.z + -0.5686791 * ne.z + -0.9652397 * nf.z + -0.30101392 * ng.z + -0.4405244 * nh.z + -0.17934786 * ni.z;
	z = -0.05868854 * na.w + 0.6854232 * nb.w + -0.17763789 * nc.w + -0.2133866 * nd.w + 0.0042767767 * ne.w + -0.501675 * nf.w + -0.23358314 * ng.w + 0.032784555 * nh.w + 0.060270388 * ni.w;
	float q = s + t + u + v + w + x + y + z + 0.011247989;
	s = 0.03503288 * a.x + 0.2845259 * b.x + 0.16413508 * c.x + -0.34577143 * d.x + 0.44561076 * e.x + 0.030196154 * f.x + -0.0065472857 * g.x + -0.042291068 * h.x + 0.06683734 * i.x;
	t = 0.059763495 * a.y + -0.25460613 * b.y + -0.041759964 * c.y + 0.11462825 * d.y + 0.24414043 * e.y + -0.541063 * f.y + -0.004890282 * g.y + 0.42774653 * h.y + -0.07330144 * i.y;
	u = 0.019507758 * a.z + -0.04953541 * b.z + -0.007673771 * c.z + -0.6131209 * d.z + 0.7618412 * e.z + -0.48944667 * f.z + -0.00863334 * g.z + 0.29244134 * h.z + 0.15565501 * i.z;
	v = 0.23191626 * a.w + -0.18614769 * b.w + 0.2090005 * c.w + -0.3833056 * d.w + -0.3266635 * e.w + -0.4969288 * f.w + 0.017049763 * g.w + 0.0011545217 * h.w + 0.037597295 * i.w;
	w = -0.09295784 * na.x + -0.22808313 * nb.x + 0.1440326 * nc.x + 0.44798684 * nd.x + -0.20869905 * ne.x + 0.29082736 * nf.x + 0.053629175 * ng.x + 0.10080793 * nh.x + 0.097120985 * ni.x;
	x = -0.21155189 * na.y + -0.057156052 * nb.y + -0.43517458 * nc.y + -0.30298027 * nd.y + 0.034714725 * ne.y + 2.3735354 * nf.y + 0.16829175 * ng.y + 0.025805125 * nh.y + -0.70330936 * ni.y;
	y = 0.012663191 * na.z + -0.116386354 * nb.z + -0.09403862 * nc.z + 0.3583474 * nd.z + -0.67109364 * ne.z + 1.4337319 * nf.z + -0.04739808 * ng.z + 0.053359438 * nh.z + -0.24203655 * ni.z;
	z = -0.24584045 * na.w + 0.21607958 * nb.w + 0.61612964 * nc.w + 0.15417401 * nd.w + 0.39434698 * ne.w + 0.91960156 * nf.w + -0.32311833 * ng.w + -0.040789135 * nh.w + 0.25973907 * ni.w;
	float r = s + t + u + v + w + x + y + z + -0.08489115;

	return Compress(float4(o, p, q, r));
}