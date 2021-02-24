// Conv-4x3x3x8 (5)
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

	float s = 0.038567036 * a.x + 0.0705653 * b.x + -0.08456429 * c.x + 0.17830919 * d.x + 0.15700413 * e.x + -0.48310483 * f.x + -0.13568404 * g.x + 0.38844487 * h.x + -0.24666616 * i.x;
	float t = -0.022864863 * a.y + -0.09708212 * b.y + 0.080999635 * c.y + 0.10234689 * d.y + -0.9441054 * e.y + 0.37205943 * f.y + 0.023142772 * g.y + -0.30508518 * h.y + 0.23074047 * i.y;
	float u = 0.026479473 * a.z + -0.21789004 * b.z + 0.16894147 * c.z + 0.24001078 * d.z + -0.5764876 * e.z + 0.37745288 * f.z + -0.08588339 * g.z + 0.09972613 * h.z + 0.09875314 * i.z;
	float v = 0.23032863 * a.w + -0.0054795295 * b.w + -0.29228735 * c.w + 0.49698094 * d.w + -1.3256943 * e.w + -0.029346323 * f.w + 0.15363467 * g.w + -0.23334587 * h.w + -0.020885304 * i.w;
	float w = -0.08072811 * na.x + -0.12015889 * nb.x + 0.08035302 * nc.x + -0.2672039 * nd.x + -0.11779888 * ne.x + 0.2646977 * nf.x + -0.37465525 * ng.x + 0.29485774 * nh.x + -0.02784101 * ni.x;
	float x = -0.19388446 * na.y + 0.057232708 * nb.y + 0.11700054 * nc.y + 0.7411195 * nd.y + 0.07168483 * ne.y + 0.03476339 * nf.y + 0.606746 * ng.y + -0.6688325 * nh.y + -0.0019025522 * ni.y;
	float y = -0.19526717 * na.z + 0.3453801 * nb.z + -0.26843297 * nc.z + 0.29258463 * nd.z + -0.9860014 * ne.z + -0.029417401 * nf.z + -0.1935398 * ng.z + 0.3000599 * nh.z + -0.13897806 * ni.z;
	float z = -1.1220787 * na.w + 0.79593414 * nb.w + 0.08730238 * nc.w + -0.56224525 * nd.w + 0.2729329 * ne.w + -0.028041862 * nf.w + -0.08895821 * ng.w + 0.07971253 * nh.w + -0.1097084 * ni.w;
	float o = s + t + u + v + w + x + y + z + -0.053113442;
	s = 0.04177622 * a.x + -0.0036336621 * b.x + -0.031431884 * c.x + -0.13659629 * d.x + -0.059160918 * e.x + 0.031841677 * f.x + 0.07246833 * g.x + 0.112269066 * h.x + -0.242079 * i.x;
	t = 0.019453337 * a.y + -0.060118888 * b.y + -0.0056063943 * c.y + 0.37430856 * d.y + 0.66169596 * e.y + 0.22895052 * f.y + -0.116621315 * g.y + 0.019051338 * h.y + 0.2796385 * i.y;
	u = 0.025202444 * a.z + -0.063063145 * b.z + 0.03306231 * c.z + -0.087987594 * d.z + -0.15086448 * e.z + 0.015091712 * f.z + -0.016382191 * g.z + -0.2719014 * h.z + -0.1213443 * i.z;
	v = 0.15685205 * a.w + 0.16103153 * b.w + 0.12104904 * c.w + -0.31669632 * d.w + -0.5131755 * e.w + -0.2940805 * f.w + 0.03922636 * g.w + 0.10866127 * h.w + 0.08225846 * i.w;
	w = 0.027171694 * na.x + -0.19411181 * nb.x + -0.17554177 * nc.x + -0.06920469 * nd.x + 0.35313594 * ne.x + 0.5729236 * nf.x + 0.15976419 * ng.x + -0.22865002 * nh.x + -0.14128903 * ni.x;
	x = 0.06538863 * na.y + 0.3888246 * nb.y + -0.06093744 * nc.y + -0.0021450054 * nd.y + -0.7144259 * ne.y + 0.19505557 * nf.y + 0.017545538 * ng.y + 0.27564743 * nh.y + -0.1891801 * ni.y;
	y = -0.04018616 * na.z + 0.3965399 * nb.z + 0.03547221 * nc.z + -0.10051028 * nd.z + 0.5457805 * ne.z + -0.058937907 * nf.z + 0.24849786 * ng.z + 0.29411504 * nh.z + 0.09580802 * ni.z;
	z = -0.2584023 * na.w + -0.06644846 * nb.w + -0.0073378505 * nc.w + 0.59547085 * nd.w + 0.14581262 * ne.w + -0.025333082 * nf.w + -0.105372205 * ng.w + -0.14488424 * nh.w + 0.10319663 * ni.w;
	float p = s + t + u + v + w + x + y + z + -0.030240078;
	s = -0.024701372 * a.x + 0.025341522 * b.x + 0.06547468 * c.x + -0.1478955 * d.x + -0.48662245 * e.x + 0.29320917 * f.x + 0.119131036 * g.x + -0.44793203 * h.x + 0.4688749 * i.x;
	t = -0.108995534 * a.y + 0.070233956 * b.y + -0.13956581 * c.y + -0.21479332 * d.y + -0.12898977 * e.y + -0.40991426 * f.y + 0.020437365 * g.y + 0.17140286 * h.y + -0.40149483 * i.y;
	u = -0.00905909 * a.z + 0.10485684 * b.z + -0.08578809 * c.z + -0.17500319 * d.z + 0.4865422 * e.z + -0.38730884 * f.z + 0.061339755 * g.z + 0.034676336 * h.z + -0.020573441 * i.z;
	v = -0.2025594 * a.w + 0.0911606 * b.w + 0.082284346 * c.w + -0.35355926 * d.w + -0.61544394 * e.w + 0.32040882 * f.w + -0.14619522 * g.w + 0.2941107 * h.w + -0.078201585 * i.w;
	w = 0.0495746 * na.x + 0.08055962 * nb.x + -0.078246616 * nc.x + 0.26895896 * nd.x + -0.52850366 * ne.x + -0.28460968 * nf.x + 0.21908188 * ng.x + -0.14934134 * nh.x + -0.022079289 * ni.x;
	x = 0.18326212 * na.y + -0.1183244 * nb.y + -0.026135176 * nc.y + -0.63453907 * nd.y + 1.6052349 * ne.y + -0.3253275 * nf.y + -0.51091814 * ng.y + 0.42029953 * nh.y + 0.1379729 * ni.y;
	y = 0.13548519 * na.z + -0.28737086 * nb.z + 0.19119221 * nc.z + -0.2149449 * nd.z + -0.9226876 * ne.z + 0.033711042 * nf.z + 0.10450268 * ng.z + -0.4635861 * nh.z + 0.120921664 * ni.z;
	z = 0.95850354 * na.w + -0.86698693 * nb.w + 0.13464972 * nc.w + 0.46481287 * nd.w + -0.6893183 * ne.w + -0.12460651 * nf.w + 0.10310439 * ng.w + -0.067780524 * nh.w + 0.06973546 * ni.w;
	float q = s + t + u + v + w + x + y + z + -0.03733598;
	s = 0.0053327214 * a.x + 0.06828431 * b.x + 0.017400311 * c.x + -0.022229843 * d.x + 0.021846429 * e.x + 0.17038181 * f.x + -0.0036154294 * g.x + -0.0076043517 * h.x + 0.03144882 * i.x;
	t = -0.09128217 * a.y + -0.019056903 * b.y + -0.10658804 * c.y + -0.07005582 * d.y + 1.0119812 * e.y + -0.18679373 * f.y + 0.0014865986 * g.y + -0.13718273 * h.y + 0.011373519 * i.y;
	u = 0.007684441 * a.z + -0.0024304984 * b.z + -0.0005824064 * c.z + -0.046654146 * d.z + -0.18832356 * e.z + -0.06836408 * f.z + 0.03383738 * g.z + 0.0031701874 * h.z + -0.045616426 * i.z;
	v = -0.07080597 * a.w + 0.12452656 * b.w + 0.055096764 * c.w + -0.11060644 * d.w + 0.16163704 * e.w + -0.028699806 * f.w + -0.02837693 * g.w + 0.057780527 * h.w + 0.01604268 * i.w;
	w = 0.031746984 * na.x + 0.037449647 * nb.x + -0.17273042 * nc.x + 0.13193378 * nd.x + -0.083798654 * ne.x + 0.2653949 * nf.x + -0.022535313 * ng.x + 0.0860065 * nh.x + -0.15048456 * ni.x;
	x = 0.039347745 * na.y + 0.10379977 * nb.y + -0.062039107 * nc.y + -0.32586884 * nd.y + -0.3929803 * ne.y + -0.021611424 * nf.y + 0.06064707 * ng.y + -0.28440315 * nh.y + 0.025921293 * ni.y;
	y = -0.016964613 * na.z + -0.0066489074 * nb.z + 0.13551117 * nc.z + 0.12154996 * nd.z + -0.104811326 * ne.z + -0.18939951 * nf.z + -0.08372982 * ng.z + 0.055679534 * nh.z + -0.0070043216 * ni.z;
	z = 0.3667913 * na.w + -0.73069835 * nb.w + 0.06653495 * nc.w + -0.045829143 * nd.w + 0.84753215 * ne.w + -0.011205747 * nf.w + 0.062056217 * ng.w + -0.083614476 * nh.w + -0.05290749 * ni.w;
	float r = s + t + u + v + w + x + y + z + -0.066139325;

	return Compress(float4(o, p, q, r));
}