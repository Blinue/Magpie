// Anime4K_Restore_CNN_Soft_S
// 移植自 https://github.com/bloc97/Anime4K/blob/master/glsl/Restore/Anime4K_Restore_CNN_Soft_S.glsl

//!MAGPIE EFFECT
//!VERSION 3
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT
//!SORT_NAME Anime4K_Restore_0


//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D tex1;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D tex2;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D tex3;


//!PASS 1
//!DESC Conv-4x3x3x3
//!IN INPUT
//!OUT tex1
//!BLOCK_SIZE 16
//!NUM_THREADS 64

void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}
	float2 inputPt = GetInputPt();

	uint i, j;

	float3 src[4][4];
	[unroll]
	for (i = 0; i <= 2; i += 2) {
		[unroll]
		for (j = 0; j <= 2; j += 2) {
			float2 tpos = (gxy + uint2(i, j)) * inputPt;
			const float4 sr = INPUT.GatherRed(sam, tpos);
			const float4 sg = INPUT.GatherGreen(sam, tpos);
			const float4 sb = INPUT.GatherBlue(sam, tpos);

			// w z
			// x y
			src[i][j] = float3(sr.w, sg.w, sb.w);
			src[i][j + 1] = float3(sr.x, sg.x, sb.x);
			src[i + 1][j] = float3(sr.z, sg.z, sb.z);
			src[i + 1][j + 1] = float3(sr.y, sg.y, sb.y);
		}
	}

	[unroll]
	for (i = 1; i <= 2; ++i) {
		[unroll]
		for (j = 1; j <= 2; ++j) {
			uint2 destPos = gxy + uint2(i - 1, j - 1);

			if (i != 1 || j != 1) {
				if (destPos.x >= inputSize.x || destPos.y >= inputSize.y) {
					continue;
				}
			}

			float4 result = mul(src[i - 1][j - 1], float3x4(0.10922428, -0.16249932, 0.15452726, -0.15669551, 0.053448875, -0.16528402, 0.01697721, -0.049275912, 0.20947173, -0.10576949, 0.19738325, -0.025417482));
			result += mul(src[i - 1][j], float3x4(-0.3285196, 0.15909512, -0.5273671, 0.23778777, -0.40508887, -0.0609677, -0.4188177, 0.11137456, -0.24131267, 0.10453423, -0.36216277, 0.053446792));
			result += mul(src[i - 1][j + 1], float3x4(0.23072472, -0.082083695, -0.0041477727, -0.09136237, 0.11958912, -0.312698, -0.15842685, -0.013882424, 0.10933724, 0.017880991, -0.022167003, 0.014662608));
			result += mul(src[i][j - 1], float3x4(-0.2789985, 0.054727737, 0.22577816, -0.49625716, -0.48472273, -0.011525487, 0.5354349, -0.08814955, -0.27021924, -0.044563178, 0.008232271, -0.13480483));
			result += mul(src[i][j], float3x4(-0.18203105, 0.09277001, 0.27071548, -0.17773713, -0.4335171, 1.2275106, -0.07663438, -0.29020032, 0.011992759, 0.060106967, 0.11002492, -0.046098012));
			result += mul(src[i][j + 1], float3x4(0.08363418, 0.063420765, -0.10278259, 0.09357691, 0.38670546, 0.13577081, 0.048631024, -0.024960777, 0.0846784, -0.057097007, 0.06049236, 0.042082917));
			result += mul(src[i + 1][j - 1], float3x4(0.12315548, -0.056513585, -0.09826642, -0.17079762, 0.06479095, -0.36984903, -0.12512982, 0.042867575, 0.08360464, 0.12835538, -0.005067881, 0.02542005));
			result += mul(src[i + 1][j], float3x4(0.18997705, 0.086363226, -0.0007131526, 0.19858918, 0.39745626, -0.0090341605, 0.27864447, 0.20052041, 0.010576528, -0.089242525, -0.025109483, -0.030768145));
			result += mul(src[i + 1][j + 1], float3x4(0.05427315, -0.060894873, 0.06548642, 0.095537595, 0.29116166, -0.16159569, -0.13293959, -0.112566955, 0.0059667625, 0.016041303, 0.03831561, 0.09869594));
			result += float4(0.0113532655, -0.06449327, 0.035503868, 0.5683031);

			tex1[destPos] = result;
		}
	}
}


//!PASS 2
//!DESC Conv-4x3x3x8
//!IN tex1
//!OUT tex2
//!BLOCK_SIZE 16
//!NUM_THREADS 64

void Pass2(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}
	float2 inputPt = GetInputPt();

	uint i, j;

	float4 src[4][4];
	[unroll]
	for (i = 0; i <= 2; i += 2) {
		[unroll]
		for (j = 0; j <= 2; j += 2) {
			float2 tpos = (gxy + uint2(i, j)) * inputPt;
			const float4 sr = tex1.GatherRed(sam, tpos);
			const float4 sg = tex1.GatherGreen(sam, tpos);
			const float4 sb = tex1.GatherBlue(sam, tpos);
			const float4 sa = tex1.GatherAlpha(sam, tpos);

			// w z
			// x y
			src[i][j] = float4(sr.w, sg.w, sb.w, sa.w);
			src[i][j + 1] = float4(sr.x, sg.x, sb.x, sa.x);
			src[i + 1][j] = float4(sr.z, sg.z, sb.z, sa.z);
			src[i + 1][j + 1] = float4(sr.y, sg.y, sb.y, sa.y);
		}
	}

	[unroll]
	for (i = 1; i <= 2; ++i) {
		[unroll]
		for (j = 1; j <= 2; ++j) {
			uint2 destPos = gxy + uint2(i - 1, j - 1);

			if (i != 1 || j != 1) {
				if (destPos.x >= inputSize.x || destPos.y >= inputSize.y) {
					continue;
				}
			}

			float4 result = mul(max(src[i - 1][j - 1], 0), float4x4(-0.027102098, 0.2640691, 0.1169015, 0.030902913, 0.15404584, 0.1361459, -0.38066056, 0.096569136, -0.111836195, -0.0051853824, -0.0996669, -0.23538585, -0.07060754, -0.18889332, -0.10793357, -0.15154232));
			result += mul(max(src[i - 1][j], 0), float4x4(0.1378689, 0.21024452, 0.010976513, 0.0179521, -0.05993059, -0.28364083, 0.24486947, 0.21347582, -0.12522404, -0.16091396, 0.15499291, 0.08353191, -0.03342411, -0.08964405, 0.25111282, -0.07550899));
			result += mul(max(src[i - 1][j + 1], 0), float4x4(-0.06398718, 0.05763278, 0.021394925, 0.14780094, -0.033050716, 0.03346528, -0.0846797, 0.0125302235, 0.18018652, 0.24586707, 0.050538495, 0.09879243, -0.100035876, 0.043505374, 0.042692907, -0.08768257));
			result += mul(max(src[i][j - 1], 0), float4x4(-0.11572878, 0.0545887, 0.16437739, 0.2775331, 0.10323911, -0.18938646, -0.17097469, -0.188723, 0.085762165, 0.14605838, -0.15568069, -0.16947642, 0.09042493, -0.087587915, -0.041969277, 0.27252352));
			result += mul(max(src[i][j], 0), float4x4(0.21475963, -0.018211678, -0.5711054, -0.09235345, -0.20367791, -0.23041399, 0.16346097, 0.007901888, 0.12542121, 0.16807431, 0.09862575, 0.16968751, 0.28490388, 0.40945014, -0.22364445, 0.14460565));
			result += mul(max(src[i][j + 1], 0), float4x4(0.27512726, 0.14046481, -0.17684339, 0.102218024, -0.10503195, 0.3080809, 0.03681373, 0.2668656, -0.093752496, -0.07476867, 0.19900662, 0.06028286, -0.19815882, -0.0584525, 0.027984729, -0.02143819));
			result += mul(max(src[i + 1][j - 1], 0), float4x4(-0.16829525, -0.06818115, 0.0006509334, 0.01163159, 0.18918815, -0.10731989, -0.008126929, -0.47991323, -0.11022971, 0.19150843, 0.05272113, -0.34417602, 0.022882428, 0.1774031, 0.062597334, -0.09915319));
			result += mul(max(src[i + 1][j], 0), float4x4(0.32131585, 0.05668815, -0.34203658, 0.05542482, -0.008077225, 0.009148517, 0.10953332, -0.050969962, 0.09904077, 0.46938205, -0.5148919, -0.22275375, -0.10536104, -0.23662373, 0.002147416, -0.14256701));
			result += mul(max(src[i + 1][j + 1], 0), float4x4(-0.19335353, -0.103732094, 0.17156832, 0.0059756916, -0.118641876, 0.14529023, -0.18662338, 0.0447326, 0.026719248, 0.042491894, 0.026437795, 0.05601309, 0.08645617, 0.08365193, -0.039582565, 0.16612953));
			result += mul(max(-src[i - 1][j - 1], 0), float4x4(-0.014315469, 0.012588422, 0.037587024, 0.08707526, -0.08064868, -0.28149533, 0.27326405, 0.21468583, -0.04278333, 0.29369017, 0.18653142, 0.035729136, 0.079363555, 0.30725953, 0.0147137, 0.08527481));
			result += mul(max(-src[i - 1][j], 0), float4x4(0.06659263, 0.03452449, -0.33752796, 0.0066543026, 0.48697233, 0.019602561, -0.32033685, -0.20538871, 0.3089118, 0.4315903, -0.13524854, -0.10791581, 0.3315688, 0.13135147, -0.26904663, 0.142365));
			result += mul(max(-src[i - 1][j + 1], 0), float4x4(0.13619833, 0.045271892, -0.029841429, 0.010704955, -0.29257727, -0.10563375, 0.35345638, -0.06734038, -0.043791633, -0.0056891907, -0.078411415, 0.075443126, -0.05746597, -0.19959894, -0.12797245, 0.18837726));
			result += mul(max(-src[i][j - 1], 0), float4x4(0.25673476, 0.120482095, -0.23827696, -0.13557845, 0.300447, -0.3008584, -0.13834439, 0.5459493, -0.26155484, 0.06905137, 0.16247983, 0.039960653, -0.023218757, 0.07977591, -0.11354706, -0.25831422));
			result += mul(max(-src[i][j], 0), float4x4(0.0842605, 0.282916, 0.14062001, 0.06356874, 0.55912817, 0.1743876, -0.30324093, 0.052068707, -0.20756413, 0.27321506, -0.26560605, -0.27695876, -0.3927334, -0.5439608, 0.39293098, -0.001130203));
			result += mul(max(-src[i][j + 1], 0), float4x4(-0.021890296, -0.12703396, 0.06660714, -0.03164527, 0.0018722567, -0.26552317, 0.06978973, -0.24030049, 0.46008193, 0.5595346, 0.081981994, -0.038414747, -0.010446991, -0.56102365, -0.079274766, -0.01851302));
			result += mul(max(-src[i + 1][j - 1], 0), float4x4(0.052988984, 0.030581746, -0.06868741, 0.21545182, -0.5706256, -0.0034910638, 0.48361364, 0.9020033, -0.02242781, -0.13256042, 0.08997955, 0.21001706, -0.059571438, -0.040119104, -0.05029196, -0.127414));
			result += mul(max(-src[i + 1][j], 0), float4x4(-0.08275339, -0.05999088, 0.11068767, 0.014646892, -0.041986465, 0.1028236, -0.17218924, 0.026559748, -0.17412743, -0.38364175, 0.17410514, 0.13038695, 0.23155633, 0.2655843, 0.045085523, 0.13005458));
			result += mul(max(-src[i + 1][j + 1], 0), float4x4(-0.013383197, -0.064526096, 0.049046878, 0.015992291, 0.123987064, 0.0104690585, 0.07065378, -0.009824511, -0.036109775, 0.13384768, 0.29676288, -0.39475223, -0.009368096, -0.05666906, -0.09132696, -0.082638375));
			result += float4(-0.106538564, -0.065693766, -0.03790106, 0.04776706);

			tex2[destPos] = result;
		}
	}
}


//!PASS 3
//!DESC Conv-4x3x3x8
//!IN tex2
//!OUT tex3
//!BLOCK_SIZE 16
//!NUM_THREADS 64

void Pass3(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}
	float2 inputPt = GetInputPt();

	uint i, j;

	float4 src[4][4];
	[unroll]
	for (i = 0; i <= 2; i += 2) {
		[unroll]
		for (j = 0; j <= 2; j += 2) {
			float2 tpos = (gxy + uint2(i, j)) * inputPt;
			const float4 sr = tex2.GatherRed(sam, tpos);
			const float4 sg = tex2.GatherGreen(sam, tpos);
			const float4 sb = tex2.GatherBlue(sam, tpos);
			const float4 sa = tex2.GatherAlpha(sam, tpos);

			// w z
			// x y
			src[i][j] = float4(sr.w, sg.w, sb.w, sa.w);
			src[i][j + 1] = float4(sr.x, sg.x, sb.x, sa.x);
			src[i + 1][j] = float4(sr.z, sg.z, sb.z, sa.z);
			src[i + 1][j + 1] = float4(sr.y, sg.y, sb.y, sa.y);
		}
	}

	[unroll]
	for (i = 1; i <= 2; ++i) {
		[unroll]
		for (j = 1; j <= 2; ++j) {
			uint2 destPos = gxy + uint2(i - 1, j - 1);

			if (i != 1 || j != 1) {
				if (destPos.x >= inputSize.x || destPos.y >= inputSize.y) {
					continue;
				}
			}

			float4 result = mul(max(src[i - 1][j - 1], 0), float4x4(0.024004154, -0.26474997, -0.5256586, 0.051624652, -0.16621786, 0.2964122, 0.6044247, -0.14335106, 0.17002718, -0.2679876, -0.30162668, 0.1273794, -0.17601459, -0.1782376, 0.104725115, -0.16351137));
			result += mul(max(src[i - 1][j], 0), float4x4(-0.121676154, 0.047741555, -0.06738679, -0.056402843, 0.004424971, -0.35099635, -0.073440626, 0.039784692, 0.15204315, -0.1165704, 0.11231046, -0.27369732, 0.33737272, -0.11880767, 0.09637475, -0.14709689));
			result += mul(max(src[i - 1][j + 1], 0), float4x4(-0.017987821, -0.08798823, -0.062515825, -0.046803873, -0.05871703, 0.27013004, 0.19397618, -0.052147817, 0.003271283, -0.0029015478, -0.07390092, -0.09348337, -0.1574738, 0.06750957, -0.07661155, 0.054327156));
			result += mul(max(src[i][j - 1], 0), float4x4(-0.15215784, -0.72508365, -0.3202069, 0.20295432, -0.19125701, 0.021401431, -0.051837035, -0.025939213, 0.25565025, -0.12872623, 0.13169816, 0.27377388, -0.008718429, -0.05864064, 0.028844763, 0.1144993));
			result += mul(max(src[i][j], 0), float4x4(-0.30012092, -0.1322455, -0.11868545, 0.09857058, 0.082795605, -0.075334676, -0.3752773, -0.02918163, -0.67764, -0.38598236, -0.21023573, 0.38274166, -0.07398165, -0.07213789, -0.28427607, 0.1266569));
			result += mul(max(src[i][j + 1], 0), float4x4(-0.37507388, 0.18809201, -0.21982779, 0.27208912, 0.022066567, -0.27627763, 0.12345216, -0.30041683, 0.017002959, -0.091398515, -0.25207692, -0.29253414, -0.08231422, -0.14665812, -0.07868529, -0.24562219));
			result += mul(max(src[i + 1][j - 1], 0), float4x4(0.08686712, 0.080837384, 0.20736577, 0.008233064, 0.14957365, 0.21801959, -0.04870689, 0.42149112, 0.27255878, 0.33320278, -0.08467146, 0.10381615, 0.055278245, 0.085710146, 0.009097151, 0.29092705));
			result += mul(max(src[i + 1][j], 0), float4x4(0.0012207404, -0.023874281, -0.027035477, 0.005157451, 0.19330226, 0.33711615, -0.16495204, 0.549021, 0.44879642, 0.1978837, -0.20492741, 0.28099406, 0.2631811, 0.40786585, -0.055340275, 0.2575511));
			result += mul(max(src[i + 1][j + 1], 0), float4x4(0.29127392, -0.06287165, 0.12715077, 0.14784902, -0.3183704, 0.42057636, -0.11483724, -0.3019506, 0.010730576, 0.29091576, -0.046116166, -0.23528357, -0.0037143505, 0.1191774, -0.06084074, 0.011641706));
			result += mul(max(-src[i - 1][j - 1], 0), float4x4(-0.2579205, 0.036545023, 0.11691888, 0.04996418, 0.21318026, 0.21370813, -0.14114271, 0.031217605, -0.06979331, -0.0690704, 0.04618086, 0.025164584, -0.10994228, 0.109930746, 0.103678934, 0.12193115));
			result += mul(max(-src[i - 1][j], 0), float4x4(-0.19843774, -0.11237926, 0.007291354, 0.16480611, -0.15669724, 0.46283355, 0.077065215, 0.112273656, 0.17143534, -0.19934891, -0.25481275, 0.034591813, -0.27032652, -0.2702769, 0.04816228, -0.031614583));
			result += mul(max(-src[i - 1][j + 1], 0), float4x4(-0.16307239, -0.11295217, 0.05861256, 0.14225823, -0.015648091, 0.11741865, 0.113366075, 0.023935538, 0.19560932, -0.10553561, -0.042583376, -0.048160724, -0.3116519, 0.13957061, -0.0044852323, -0.015472912));
			result += mul(max(-src[i][j - 1], 0), float4x4(-0.15629178, 0.06463271, -0.13176678, 0.025518289, -0.021733627, 0.22236359, 0.019508492, -0.11629477, 0.10801276, -0.021957984, -0.11272639, -0.03615053, -0.121420704, 0.2520835, 0.043395765, 0.1699031));
			result += mul(max(-src[i][j], 0), float4x4(0.2886654, 0.21755892, 0.21757497, 0.08442575, -0.109903164, -0.67295986, 0.22886126, -0.027185453, 0.3761606, 0.23199768, 0.05908783, -0.1496158, 0.10832971, -0.3530352, 0.20234483, -0.07615918));
			result += mul(max(-src[i][j + 1], 0), float4x4(0.11043024, 0.18943349, 0.42394367, 0.029350199, -0.15085667, 0.020204183, -0.081609115, 0.07907012, 0.33805525, 0.0066280114, 0.0018284445, 0.022983696, 0.004984607, 0.0429299, -0.14568979, -0.29143327));
			result += mul(max(-src[i + 1][j - 1], 0), float4x4(-0.16376027, -0.20387048, 0.06522074, 0.17484841, -0.13885716, -0.04380927, -0.03535832, -0.16978237, -0.004799155, -0.25407305, -0.039976966, -0.011992087, -0.22535577, -0.09583549, 0.0334331, 0.016292758));
			result += mul(max(-src[i + 1][j], 0), float4x4(-0.38688713, -0.20232083, 0.23887886, -0.10438324, -0.24170811, -0.074868314, 0.03977399, -0.22810821, -0.08257971, -0.11902456, 0.106009185, -0.078289054, -0.11932821, 0.024207884, 0.10070917, 0.79348284));
			result += mul(max(-src[i + 1][j + 1], 0), float4x4(-0.4018743, 0.050456528, 0.035341598, -0.03788609, 0.12964934, -0.44461823, 0.029031694, 0.29604837, -0.102386944, -0.13805065, 0.0055692918, 0.14659804, -0.22499937, 0.14680648, -0.3443954, -0.06994176));
			result += float4(-0.0063428865, 0.0057986965, -0.12526293, -0.059240736);

			tex3[destPos] = result;
		}
	}
}


//!PASS 4
//!DESC Conv-3x3x3x8
//!IN INPUT, tex3
//!BLOCK_SIZE 8
//!NUM_THREADS 64

void Pass4(uint2 blockStart, uint3 threadId) {
	uint2 gxy = Rmp8x8(threadId.x) + blockStart;
	uint2 inputSize = GetInputSize();
	if (!CheckViewport(gxy)) {
		return;
	}

	float2 inputPt = GetInputPt();
	float2 pos = (gxy + 0.5f) * inputPt;

	// [ a, d, g ]
	// [ b, e, h ]
	// [ c, f, i ]
	float4 a = tex3.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 b = tex3.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 c = tex3.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 d = tex3.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 e = tex3.SampleLevel(sam, pos, 0);
	float4 f = tex3.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 g = tex3.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 h = tex3.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 i = tex3.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 result = mul(max(a, 0), float4x4(0.08631539, 0.09499331, 0.065609254, 0.0, -0.023760278, -0.027293118, -0.022839671, 0.0, -0.012447854, -0.008565141, -0.012041815, 0.0, -0.033292875, -0.031266093, -0.02874347, 0.0));
	result += mul(max(b, 0), float4x4(0.08709062, 0.09760889, 0.08988583, 0.0, -0.09099671, -0.102120616, -0.098076016, 0.0, 0.057814583, 0.06999608, 0.05961344, 0.0, 0.12246188, 0.1319784, 0.12254915, 0.0));
	result += mul(max(c, 0), float4x4(0.07694916, 0.0822054, 0.07549296, 0.0, -0.046808865, -0.051509347, -0.035890795, 0.0, 0.01599848, 0.014677793, 0.0086143715, 0.0, 0.033142705, 0.0426565, 0.035911378, 0.0));
	result += mul(max(d, 0), float4x4(-0.0008269902, 0.0009082343, 0.014101725, 0.0, 0.0006387551, 0.005079344, -0.013034868, 0.0, 0.013909732, 0.011026747, 0.012485332, 0.0, 0.027028518, 0.022164145, 0.03183532, 0.0));
	result += mul(max(e, 0), float4x4(-0.33575395, -0.36700967, -0.34140685, 0.0, 0.35850254, 0.37535715, 0.34613726, 0.0, -0.12680013, -0.1256115, -0.112494245, 0.0, -0.061541136, -0.059120018, -0.06552594, 0.0));
	result += mul(max(f, 0), float4x4(-0.047570463, -0.050335366, -0.04665491, 0.0, -0.110970475, -0.12363716, -0.11072252, 0.0, 0.041563414, 0.059771337, 0.045290247, 0.0, -0.17999935, -0.19700716, -0.17459513, 0.0));
	result += mul(max(g, 0), float4x4(0.078488424, 0.07483357, 0.08347933, 0.0, -0.0063715233, 0.00035415235, -0.010886946, 0.0, 0.031237155, 0.02512343, 0.034399323, 0.0, -0.023146842, -0.026732154, -0.027644241, 0.0));
	result += mul(max(h, 0), float4x4(-0.05906883, -0.06784104, -0.04506148, 0.0, -0.003939601, -0.0011749315, -0.006256036, 0.0, -0.1662408, -0.16871658, -0.16598499, 0.0, 0.051277652, 0.04837499, 0.05120855, 0.0));
	result += mul(max(i, 0), float4x4(0.08158806, 0.08674548, 0.07437206, 0.0, -0.05765347, -0.06196418, -0.057311118, 0.0, 0.26747537, 0.2668808, 0.2389857, 0.0, -0.010376844, -0.01690028, -0.008414153, 0.0));
	result += mul(max(-a, 0), float4x4(0.030539425, 0.02415435, 0.039969034, 0.0, 0.006491679, 0.014436586, 0.005435709, 0.0, -0.0058292216, -0.013982021, -0.011243379, 0.0, 0.025942149, 0.015361476, 0.019134998, 0.0));
	result += mul(max(-b, 0), float4x4(-0.06322247, -0.07146787, -0.06673042, 0.0, 0.028702464, 0.039047733, 0.039646607, 0.0, -0.072553575, -0.08046175, -0.07027197, 0.0, -0.1447189, -0.1539398, -0.1466465, 0.0));
	result += mul(max(-c, 0), float4x4(-0.046430312, -0.054549117, -0.048076343, 0.0, 0.032971155, 0.02980819, 0.029172963, 0.0, -0.017612953, -0.015100736, -0.01202649, 0.0, -0.026717246, -0.028401854, -0.034548033, 0.0));
	result += mul(max(-d, 0), float4x4(-0.0020459262, -0.0008748501, -0.012601956, 0.0, 0.0054226154, 0.008867029, 0.018921215, 0.0, -0.0021330053, -0.0036601655, -0.0022091097, 0.0, -0.08636891, -0.10203159, -0.09741449, 0.0));
	result += mul(max(-e, 0), float4x4(0.07306159, 0.08245483, 0.06548199, 0.0, -0.1933229, -0.20326294, -0.19189309, 0.0, 0.107496604, 0.11584994, 0.10907522, 0.0, 0.30877885, 0.31297725, 0.30890995, 0.0));
	result += mul(max(-f, 0), float4x4(0.03192904, 0.035112645, 0.033732817, 0.0, 0.074100636, 0.08349646, 0.06659352, 0.0, -0.1136165, -0.12470947, -0.11192198, 0.0, 0.14465587, 0.16328491, 0.13984151, 0.0));
	result += mul(max(-g, 0), float4x4(-0.05098033, -0.053096622, -0.05533725, 0.0, 0.0045651463, -0.007682458, 0.0026934785, 0.0, -0.021199327, -0.016210148, -0.030939564, 0.0, -0.031621892, -0.046702545, -0.02647333, 0.0));
	result += mul(max(-h, 0), float4x4(0.055801813, 0.06430485, 0.05052402, 0.0, 0.0241233, 0.013879883, 0.017344628, 0.0, 0.08707151, 0.10031039, 0.095042154, 0.0, -0.109053336, -0.11414017, -0.111838564, 0.0));
	result += mul(max(-i, 0), float4x4(0.030582374, 0.03604719, 0.040417343, 0.0, 0.038665913, 0.036998056, 0.030004544, 0.0, 0.09209076, 0.10010001, 0.08389406, 0.0, -0.014655714, -0.0074866647, -0.012227013, 0.0));
	result += float4(-0.008303841, -0.008251826, -0.0069884053, 0.0);

	result += INPUT.SampleLevel(sam, pos, 0);

	WriteToOutput(gxy, result.rgb);
}
