// Conv-4x3x3x8 (4)
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

	float s = -0.11336011 * a.x + -0.5546093 * b.x + -0.1980563 * c.x + -0.09243965 * d.x + -0.30328315 * e.x + 0.002288521 * f.x + -0.1719883 * g.x + -0.25393653 * h.x + -0.106638394 * i.x;
	float t = 0.060408574 * a.y + -0.4343821 * b.y + -0.09417895 * c.y + 0.04474836 * d.y + 0.66423136 * e.y + -0.16684487 * f.y + -0.046044365 * g.y + 0.1707526 * h.y + 0.13041699 * i.y;
	float u = -0.062083997 * a.z + 0.036378354 * b.z + -0.053244796 * c.z + -0.07462912 * d.z + 0.009707783 * e.z + -0.007606468 * f.z + -0.14536221 * g.z + 0.12193374 * h.z + 0.08321257 * i.z;
	float v = 0.13822994 * a.w + -0.16968055 * b.w + -0.029302116 * c.w + -0.36366686 * d.w + 0.12202857 * e.w + -0.04416427 * f.w + -0.101989016 * g.w + 0.0015375882 * h.w + 0.05370286 * i.w;
	float w = 0.009522841 * na.x + 0.0050035953 * nb.x + 0.047815464 * nc.x + 0.13878924 * nd.x + -0.12706435 * ne.x + -0.17028658 * nf.x + 0.03381212 * ng.x + 0.04619007 * nh.x + 0.040886693 * ni.x;
	float x = -0.21648684 * na.y + 0.4527824 * nb.y + 0.062216494 * nc.y + -0.0760834 * nd.y + -0.6288988 * ne.y + -0.07717104 * nf.y + 0.031264722 * ng.y + -0.1372035 * nh.y + -0.24431548 * ni.y;
	float y = 0.12201707 * na.z + 0.1075342 * nb.z + 0.064964525 * nc.z + 0.24627735 * nd.z + -0.20334174 * ne.z + 0.17362922 * nf.z + 0.14341247 * ng.z + -0.10367405 * nh.z + 0.018187301 * ni.z;
	float z = 0.29656824 * na.w + 0.36550194 * nb.w + 0.08121883 * nc.w + 0.5109051 * nd.w + 0.15495986 * ne.w + 0.14060786 * nf.w + 0.16695265 * ng.w + 0.08530239 * nh.w + -0.012310244 * ni.w;
	float o = s + t + u + v + w + x + y + z + -0.016565753;
	s = -0.005542627 * a.x + -0.4815576 * b.x + -0.16739582 * c.x + -0.17829932 * d.x + 0.16477403 * e.x + -0.13324437 * f.x + -0.18502134 * g.x + -0.29108498 * h.x + 0.05097406 * i.x;
	t = -0.005128074 * a.y + 0.07065858 * b.y + -0.10661442 * c.y + 0.0008864973 * d.y + 0.10546469 * e.y + 0.014007102 * f.y + -0.008473495 * g.y + 0.10157764 * h.y + 0.08799642 * i.y;
	u = 0.032889523 * a.z + 0.031691857 * b.z + 0.012797057 * c.z + -0.047379013 * d.z + 0.035736393 * e.z + 0.091394894 * f.z + 0.039340183 * g.z + 0.02964313 * h.z + 0.0028917622 * i.z;
	v = -0.02029337 * a.w + 0.094220296 * b.w + -0.069206014 * c.w + 0.07011055 * d.w + -0.14635274 * e.w + 0.026304023 * f.w + -0.0646385 * g.w + 0.041667704 * h.w + 0.042158034 * i.w;
	w = 0.04289771 * na.x + -0.011802231 * nb.x + -0.03529944 * nc.x + 0.03370418 * nd.x + -0.5103195 * ne.x + -0.014530308 * nf.x + 0.029903982 * ng.x + 0.029009927 * nh.x + 0.03932113 * ni.x;
	x = -0.058687404 * na.y + -0.032492165 * nb.y + 0.039978307 * nc.y + 0.0805997 * nd.y + 0.16392574 * ne.y + -0.26796678 * nf.y + 0.05706328 * ng.y + -0.026416704 * nh.y + -0.009198004 * ni.y;
	y = 0.036424693 * na.z + 0.076260746 * nb.z + 0.039109852 * nc.z + 0.06012574 * nd.z + 0.029961608 * ne.z + 0.000601677 * nf.z + 0.08278338 * ng.z + -0.104361504 * nh.z + -0.010410115 * ni.z;
	z = 0.045120504 * na.w + 0.15542893 * nb.w + 0.018970646 * nc.w + -0.040475003 * nd.w + 0.41810912 * ne.w + 0.046625525 * nf.w + 0.11266217 * ng.w + 0.03605587 * nh.w + 0.010441441 * ni.w;
	float p = s + t + u + v + w + x + y + z + 0.026173912;
	s = -0.01932058 * a.x + -0.15786819 * b.x + 0.34817162 * c.x + -0.085794635 * d.x + 0.61148757 * e.x + 0.1997698 * f.x + 0.04283192 * g.x + -0.40659058 * h.x + 0.031031879 * i.x;
	t = -0.14110667 * a.y + 0.09737822 * b.y + 0.2136361 * c.y + 0.21648659 * d.y + 0.057102222 * e.y + -0.32274124 * f.y + -0.041056994 * g.y + 0.062500246 * h.y + 0.058675725 * i.y;
	u = -0.05310352 * a.z + 0.06363739 * b.z + -0.02352861 * c.z + -0.19859853 * d.z + -0.6924276 * e.z + -0.052719172 * f.z + -0.022153398 * g.z + 0.022755368 * h.z + 0.044493068 * i.z;
	v = -0.020281073 * a.w + 0.049738068 * b.w + 0.05115416 * c.w + 0.1943825 * d.w + -0.15483907 * e.w + -0.13703728 * f.w + -0.21366403 * g.w + 0.12213289 * h.w + 0.07315763 * i.w;
	w = -0.01340149 * na.x + 0.039190706 * nb.x + -0.06010495 * nc.x + -0.031242317 * nd.x + -0.3344305 * ne.x + -0.15759717 * nf.x + 0.035195023 * ng.x + 0.09947819 * nh.x + 0.06487847 * ni.x;
	x = 0.100255184 * na.y + 0.16374993 * nb.y + -0.26901463 * nc.y + -0.088238806 * nd.y + 0.9469761 * ne.y + 0.20339924 * nf.y + 0.013781394 * ng.y + -0.40143126 * nh.y + -0.3224882 * ni.y;
	y = 0.06307512 * na.z + 0.103746325 * nb.z + 0.07068691 * nc.z + 0.21719252 * nd.z + 0.643172 * ne.z + 0.044743262 * nf.z + 0.02847355 * ng.z + -0.044767838 * nh.z + 0.026216555 * ni.z;
	z = -0.04675472 * na.w + 0.09125546 * nb.w + -0.015525009 * nc.w + 0.7057314 * nd.w + 0.37025008 * ne.w + 0.27427557 * nf.w + -0.03265264 * ng.w + -0.06721917 * nh.w + -0.0419604 * ni.w;
	float q = s + t + u + v + w + x + y + z + 0.024316467;
	s = -0.00033182523 * a.x + 0.047188014 * b.x + 0.29892203 * c.x + 0.09357643 * d.x + 0.030232515 * e.x + 0.19758245 * f.x + 0.13172227 * g.x + -0.12781778 * h.x + 0.02373677 * i.x;
	t = -0.03755632 * a.y + 0.093976915 * b.y + 0.070628785 * c.y + 0.05336897 * d.y + 0.85352725 * e.y + -0.10880546 * f.y + 0.051450137 * g.y + 0.09816019 * h.y + 0.07808459 * i.y;
	u = 0.011919896 * a.z + -0.020400822 * b.z + 0.04710827 * c.z + 0.08461272 * d.z + -0.11439534 * e.z + 0.037500232 * f.z + 0.113371514 * g.z + 0.086692594 * h.z + 0.06414449 * i.z;
	v = 0.011173226 * a.w + 0.049693275 * b.w + 0.03778407 * c.w + 0.16974896 * d.w + 0.30079946 * e.w + -0.060604237 * f.w + -0.0018831388 * g.w + 0.07333319 * h.w + 0.014693669 * i.w;
	w = -0.11176503 * na.x + -0.0652572 * nb.x + -0.08387277 * nc.x + -0.07379581 * nd.x + -0.07855383 * ne.x + -0.14641859 * nf.x + -0.02941203 * ng.x + -0.06744651 * nh.x + -0.052957434 * ni.x;
	x = 0.03692434 * na.y + 0.06360258 * nb.y + -0.031729735 * nc.y + -0.10867498 * nd.y + -0.3276245 * ne.y + 0.17300029 * nf.y + 0.036537584 * ng.y + -0.025287196 * nh.y + -0.1810677 * ni.y;
	y = 0.006600475 * na.z + -0.00840068 * nb.z + 0.0028671592 * nc.z + -0.14595534 * nd.z + 0.20311914 * ne.z + -0.023778193 * nf.z + -0.05664581 * ng.z + 0.09111506 * nh.z + -0.051484156 * ni.z;
	z = 0.07922705 * na.w + -0.011248128 * nb.w + 0.011819444 * nc.w + -0.11729043 * nd.w + -0.28706518 * ne.w + 0.07792864 * nf.w + -0.013736298 * ng.w + -0.15252969 * nh.w + 0.0014625692 * ni.w;
	float r = s + t + u + v + w + x + y + z + -0.017528493;

	return Compress(float4(o, p, q, r));
}