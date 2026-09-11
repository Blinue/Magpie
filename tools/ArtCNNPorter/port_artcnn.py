import os
import re

def parse_glsl_passes(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Split content by passes
    pass_blocks = content.split('//!DESC')
    header_comment = ""
    # Extract header comments (license, etc.) from the first block
    if pass_blocks:
        first_lines = pass_blocks[0].splitlines()
        license_lines = [l for l in first_lines if l.startswith('//')]
        header_comment = '\n'.join(license_lines)
    
    passes = []
    for block in pass_blocks[1:]:
        lines = block.splitlines()
        desc = lines[0].strip()
        
        directives = {}
        code_lines = []
        in_hook = False
        
        for line in lines[1:]:
            line_str = line.strip()
            if line_str.startswith('//!'):
                parts = line_str[3:].split(maxsplit=1)
                cmd = parts[0]
                val = parts[1] if len(parts) > 1 else ""
                if cmd == 'BIND':
                    if 'BIND' not in directives:
                        directives['BIND'] = []
                    directives['BIND'].append(val)
                else:
                    directives[cmd] = val
            elif 'vec4 hook()' in line or 'void hook()' in line:
                in_hook = True
                code_lines.append(line)
            elif in_hook:
                code_lines.append(line)
        
        hook_code = '\n'.join(code_lines)
        passes.append({
            'desc': desc,
            'directives': directives,
            'hook_code': hook_code
        })
    
    return header_comment, passes

def clean_float(val_str):
    # Standardize float format (e.g. 1.026e-03 -> 1.026e-03f or similar, but HLSL accepts scientific notation)
    return val_str

def translate_matrix_vector(code):
    # Match matrix * vector and convert to mul(matrix, vector)
    # This matches both standard shader (with _texOff vector) and compute shader (with inp vector)
    matrix_pattern = r'\b(mat4|f16mat4|M4|MF4x4)\s*\(([^)]+)\)\s*\*\s*(inp\[[^\]]+\]\[[^\]]+\]\[[^\]]+\]|\w+_texOff\(vec2\([^\)]+\)\)|\(\s*\w+_texOff\(vec2\([^\)]+\)\)\s*\+\s*\w+_texOff\(vec2\([^\)]+\)\)\s*\))'
    code = re.sub(matrix_pattern, r'mul(\3, \1(\2))', code)
    
    # Strip single-argument vector constructors in compute shader loads
    # E.g. inp[0][y][x] = V4(conv2d_mul * texelFetch(...)); -> inp[0][y][x] = conv2d_mul * texelFetch(...);
    code = re.sub(
        r'\b(inp\[\d+\]\[y\]\[x\]\s*=\s*)(?:V4|vec4|f16vec4|MF4)\((.*)\);',
        r'\1\2;',
        code
    )

    # Translate GLSL types to HLSL
    # vec4 -> MF4, mat4 -> MF4x4, vec2 -> float2, ivec2 -> int2, etc.
    code = re.sub(r'\bvec4\b', 'MF4', code)
    code = re.sub(r'\bmat4\b', 'MF4x4', code)
    code = re.sub(r'\bvec2\b', 'float2', code)
    code = re.sub(r'\bivec2\b', 'int2', code)
    code = re.sub(r'\bvec3\b', 'float3', code)
    
    # GLSL explicit arithmetic types float16
    code = re.sub(r'\bf16vec4\b', 'MF4', code)
    code = re.sub(r'\bf16mat4\b', 'MF4x4', code)
    code = re.sub(r'\bfloat16_t\b', 'MF', code)
    code = re.sub(r'\bF\b', 'MF', code)
    code = re.sub(r'\bV4\b', 'MF4', code)
    code = re.sub(r'\bM4\b', 'MF4x4', code)
    
    # GLSL barrier() -> GroupMemoryBarrierWithGroupSync()
    code = re.sub(r'\bbarrier\(\)', 'GroupMemoryBarrierWithGroupSync()', code)
    
    # Strip single-argument vector constructors
    # E.g. MF4(0.0) -> 0.0, float4(0.0) -> 0.0, etc.
    code = re.sub(
        r'\b(?:V4|vec4|f16vec4|MF4|MF3|MF2|float4|float3|float2)\(([-\d.]+)\)',
        r'\1',
        code
    )
    
    # GLSL imageStore -> output texture assignment
    # e.g., imageStore(out_image, store_pos0, result0);
    # In HLSL we can assign directly: OutTex[pos] = val;
    # We will handle imageStore specifically in the generator
    # Translate: resultX += mul(vector, matrix);
    # to: resultX = MulAdd(vector, matrix, resultX);
    muladd_pattern = r'\b(result\d+)\s*\+=\s*mul\(([^,]+),\s*(MF4x4\([^)]+\))\);'
    code = re.sub(muladd_pattern, r'\1 = MulAdd(\2, \3, \1);', code)
    
    return code


def replace_texel_fetch_robust(code):
    pos = 0
    while True:
        idx = code.find('texelFetch(', pos)
        if idx == -1:
            break
        start = idx + len('texelFetch(')
        depth = 1
        i = start
        while i < len(code) and depth > 0:
            if code[i] == '(':
                depth += 1
            elif code[i] == ')':
                depth -= 1
            i += 1
        if depth == 0:
            full_expr = code[idx:i]
            inner = code[start:i-1]
            args = []
            arg_start = 0
            inner_depth = 0
            for j in range(len(inner)):
                if inner[j] == '(':
                    inner_depth += 1
                elif inner[j] == ')':
                    inner_depth -= 1
                elif inner[j] == ',' and inner_depth == 0:
                    args.append(inner[arg_start:j].strip())
                    arg_start = j + 1
            args.append(inner[arg_start:].strip())
            
            if len(args) == 3:
                tex_raw = args[0]
                pos_expr = args[1]
                tex_name = tex_raw
                if tex_name.endswith('_raw'):
                    tex_name = tex_name[:-4]
                if tex_name == 'LUMA':
                    tex_name = 'LUMA'
                # Simplify pos_expr
                pos_expr = re.sub(r'\*\s*int2\(1,\s*1\)', '', pos_expr)
                pos_expr = re.sub(r'\+\s*int2\(0,\s*0\)', '', pos_expr)
                pos_expr = re.sub(r'\*\s*float2\(1,\s*1\)', '', pos_expr)
                pos_expr = re.sub(r'\+\s*float2\(0,\s*0\)', '', pos_expr)
                
                replacement = f"{tex_name}.Load(int3({pos_expr}, 0))"
                code = code[:idx] + replacement + code[i:]
                pos = idx + len(replacement)
            else:
                pos = i
        else:
            break
    return code


def port_standard(glsl_path, hlsl_path):
    header, passes = parse_glsl_passes(glsl_path)
    
    hlsl_content = []
    hlsl_content.append(f"// Generated from {os.path.basename(glsl_path)}")
    hlsl_content.append(header)
    hlsl_content.append("\n//!MAGPIE EFFECT")
    hlsl_content.append("//!VERSION 4")
    hlsl_content.append("//!USE MulAdd")
    hlsl_content.append("//!CAPABILITY FP16\n")
    hlsl_content.append('#include "../StubDefs.hlsli"\n')
    
    # Add static constants and samplers
    hlsl_content.append("//!TEXTURE")
    hlsl_content.append("Texture2D INPUT;\n")
    hlsl_content.append("//!TEXTURE")
    hlsl_content.append("//!WIDTH INPUT_WIDTH * 2")
    hlsl_content.append("//!HEIGHT INPUT_HEIGHT * 2")
    hlsl_content.append("Texture2D OUTPUT;\n")
    hlsl_content.append("//!SAMPLER")
    hlsl_content.append("//!FILTER POINT")
    hlsl_content.append("SamplerState SP;\n")
    hlsl_content.append("//!SAMPLER")
    hlsl_content.append("//!FILTER LINEAR")
    hlsl_content.append("SamplerState SL;\n")
    
    # Define color space matrices
    hlsl_content.append("//!COMMON")
    hlsl_content.append("static const MF3x3 RY = {0.299, 0.587, 0.114, -0.169, -0.331, 0.5, 0.5, -0.419, -0.081};")
    hlsl_content.append("static const MF3x3 YR = {1, -0.00093, 1.401687, 1, -0.3437, -0.71417, 1, 1.77216, 0.00099};\n")
    hlsl_content.append("#define O(t, x, y) t.SampleLevel(SP, pos + float2(x, y) * pt, 0)\n")
    
    # Intermediate textures
    hlsl_content.append("//!TEXTURE")
    hlsl_content.append("//!WIDTH INPUT_WIDTH")
    hlsl_content.append("//!HEIGHT INPUT_HEIGHT")
    hlsl_content.append("//!FORMAT R16G16B16A16_FLOAT")
    hlsl_content.append("Texture2D LUMA;\n")
    
    # We have Layer 0 (conv2d_0 to 7), Layer 1 (conv2d_1_0 to 7), ..., Layer 5 (conv2d_5_0 to 7), Layer 6 (conv2d_6_0)
    for layer in range(6):
        for p in range(8):
            hlsl_content.append("//!TEXTURE")
            hlsl_content.append("//!WIDTH INPUT_WIDTH")
            hlsl_content.append("//!HEIGHT INPUT_HEIGHT")
            hlsl_content.append("//!FORMAT R16G16B16A16_FLOAT")
            hlsl_content.append(f"Texture2D T{layer}_{p};\n")
            
    hlsl_content.append("//!TEXTURE")
    hlsl_content.append("//!WIDTH INPUT_WIDTH")
    hlsl_content.append("//!HEIGHT INPUT_HEIGHT")
    hlsl_content.append("//!FORMAT R16G16B16A16_FLOAT")
    hlsl_content.append("Texture2D T6_0;\n")
    
    # Pass 1: Luma pre-pass
    hlsl_content.append("//!PASS 1")
    hlsl_content.append("//!DESC Luma pre-pass")
    hlsl_content.append("//!BLOCK_SIZE 8")
    hlsl_content.append("//!NUM_THREADS 64")
    hlsl_content.append("//!IN INPUT")
    hlsl_content.append("//!OUT LUMA")
    hlsl_content.append("""void Pass1(uint2 blockStart, uint3 tid) {
	uint2 gxy = Rmp8x8(tid.x) + blockStart;
	uint2 sz = GetInputSize();
	if (gxy.x >= sz.x || gxy.y >= sz.y)
		return;
	float2 pt = float2(GetInputPt());
	float2 pos = (gxy + 0.5) * pt;
	MF3 color = INPUT.SampleLevel(SP, pos, 0).rgb;
	LUMA[gxy] = MF4(dot(MF3(0.299, 0.587, 0.114), color), 0.0, 0.0, 0.0);
}
""")

    # Port all conv passes (from index 0 to 48)
    for idx, p in enumerate(passes[:-1]):
        pass_num = idx + 2
        desc = p['desc']
        directives = p['directives']
        hook_code = p['hook_code']
        
        save_target = directives.get('SAVE', '')
        # Map save target to our intermediate textures
        # e.g., conv2d_0 -> T0_0, conv2d_1_0 -> T1_0, conv2d_6_0 -> T6_0
        out_tex = ""
        if save_target == 'conv2d':
            out_tex = 'T0_0'
        elif save_target.startswith('conv2d_'):
            parts = save_target.split('_')
            if len(parts) == 2:
                # conv2d_X
                out_tex = f"T0_{parts[1]}"
            elif len(parts) == 3:
                # conv2d_X_Y
                out_tex = f"T{parts[1]}_{parts[2]}"
        
        # Get input bindings
        in_bindings = []
        for bind_val in directives.get('BIND', []):
            if bind_val == 'LUMA':
                in_bindings.append('LUMA')
            elif bind_val == 'conv2d':
                in_bindings.append('T0_0')
            elif bind_val.startswith('conv2d_'):
                parts = bind_val.split('_')
                if len(parts) == 2:
                    in_bindings.append(f"T0_{parts[1]}")
                elif len(parts) == 3:
                    in_bindings.append(f"T{parts[1]}_{parts[2]}")
        
        # Ensure unique bindings list
        in_bindings_str = ", ".join(sorted(list(set(in_bindings))))
        
        hlsl_content.append(f"//!PASS {pass_num}")
        hlsl_content.append(f"//!DESC {desc}")
        hlsl_content.append("//!BLOCK_SIZE 8")
        hlsl_content.append("//!NUM_THREADS 64")
        hlsl_content.append(f"//!IN {in_bindings_str}")
        hlsl_content.append(f"//!OUT {out_tex}\n")
        
        # Process hook code
        # Replace texture offset functions with our O(t, x, y) macro
        # e.g., LUMA_texOff(vec2(-1, -1)).x -> O(LUMA, -1, -1).x
        # e.g., conv2d_0_texOff(vec2(-1, -1)) -> O(T0_0, -1, -1)
        # e.g., conv2d_5_5_texOff(vec2(0, -1)) -> O(T5_5, 0, -1)
        translated_code = translate_matrix_vector(hook_code)
        
        # Replace offset function calls
        def repl_tex_off(match):
            tex_name = match.group(1)
            dx = match.group(2)
            dy = match.group(3)
            # Map tex_name to our HLSL texture name
            hlsl_tex = ""
            if tex_name == 'LUMA':
                hlsl_tex = 'LUMA'
            elif tex_name == 'conv2d':
                hlsl_tex = 'T0_0'
            elif tex_name.startswith('conv2d_'):
                parts = tex_name.split('_')
                if len(parts) == 2:
                    hlsl_tex = f"T0_{parts[1]}"
                elif len(parts) == 3:
                    hlsl_tex = f"T{parts[1]}_{parts[2]}"
            return f"O({hlsl_tex}, {dx}, {dy})"
            
        translated_code = re.sub(r'(\b\w+)_texOff\(float2\(([-\d]+),\s*([-\d]+)\)\)', repl_tex_off, translated_code)
        
        # Rewrite the hook function signature to match MagpieFX style
        translated_code = translated_code.replace("MF4 hook() {", f"void Pass{pass_num}(uint2 blockStart, uint3 tid) {{")
        
        # Handle the return statement
        # In GLSL, standard passes return vec4 (MF4). In HLSL CS style, we write to output texture
        # e.g. return max(result, vec4(0.0)); or return result;
        # We need to assign it to out_tex[gxy]
        translated_code = re.sub(r'return\s+max\((result),\s*MF4\(0\.0\)\);', rf'{out_tex}[gxy] = max(\1, 0.0);', translated_code)
        translated_code = re.sub(r'return\s+max\((result),\s*0\.0\);', rf'{out_tex}[gxy] = max(\1, 0.0);', translated_code)
        translated_code = re.sub(r'return\s+(result);', rf'{out_tex}[gxy] = \1;', translated_code)
        
        # Prepend coordinate computation block
        coord_block = """\tuint2 gxy = Rmp8x8(tid.x) + blockStart;
	uint2 sz = GetInputSize();
	if (gxy.x >= sz.x || gxy.y >= sz.y)
		return;
	float2 pt = float2(GetInputPt());
	float2 pos = (gxy + 0.5) * pt;
"""
        # Insert coord_block right after the opening brace of Pass function
        func_start = translated_code.find(f"void Pass{pass_num}(uint2 blockStart, uint3 tid) {{")
        if func_start != -1:
            brace_pos = translated_code.find("{", func_start)
            translated_code = translated_code[:brace_pos+1] + "\n" + coord_block + translated_code[brace_pos+1:]
            
        hlsl_content.append(translated_code)
        hlsl_content.append("\n")
        
    # Last pass: Depth-to-space (Pass 51)
    hlsl_content.append("//!PASS 51")
    hlsl_content.append("//!DESC Depth-To-Space")
    hlsl_content.append("//!BLOCK_SIZE 16")
    hlsl_content.append("//!NUM_THREADS 64")
    hlsl_content.append("//!IN INPUT, T6_0")
    hlsl_content.append("//!OUT OUTPUT\n")
    hlsl_content.append("""void Pass51(uint2 blockStart, uint3 tid) {
	float2 pt = float2(GetInputPt());
	uint2 gxy = (Rmp8x8(tid.x) << 1) + blockStart;
	uint2 sz = GetOutputSize();
	if (gxy.x >= sz.x || gxy.y >= sz.y)
		return;

	MF4 channels = T6_0.Load(int3(gxy >> 1, 0));
	float2 opt = float2(GetOutputPt());

	float2 pos;
	MF3 rgb;
	MF3 yuv;

	// (0, 0)
	pos = (float2(gxy) + float2(0.5, 0.5)) * opt;
	rgb = INPUT.SampleLevel(SL, pos, 0).rgb;
	yuv = mul(RY, rgb);
	yuv.r = saturate(channels.x);
	OUTPUT[gxy + int2(0, 0)] = MF4(mul(YR, yuv), 1.0);

	// (1, 0)
	if (gxy.x + 1 < sz.x) {
		pos = (float2(gxy) + float2(1.5, 0.5)) * opt;
		rgb = INPUT.SampleLevel(SL, pos, 0).rgb;
		yuv = mul(RY, rgb);
		yuv.r = saturate(channels.y);
		OUTPUT[gxy + int2(1, 0)] = MF4(mul(YR, yuv), 1.0);
	}

	// (0, 1)
	if (gxy.y + 1 < sz.y) {
		pos = (float2(gxy) + float2(0.5, 1.5)) * opt;
		rgb = INPUT.SampleLevel(SL, pos, 0).rgb;
		yuv = mul(RY, rgb);
		yuv.r = saturate(channels.z);
		OUTPUT[gxy + int2(0, 1)] = MF4(mul(YR, yuv), 1.0);
	}

	// (1, 1)
	if (gxy.x + 1 < sz.x && gxy.y + 1 < sz.y) {
		pos = (float2(gxy) + float2(1.5, 1.5)) * opt;
		rgb = INPUT.SampleLevel(SL, pos, 0).rgb;
		yuv = mul(RY, rgb);
		yuv.r = saturate(channels.w);
		OUTPUT[gxy + int2(1, 1)] = MF4(mul(YR, yuv), 1.0);
	}
}
""")

    # Write to file
    os.makedirs(os.path.dirname(hlsl_path), exist_ok=True)
    with open(hlsl_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(hlsl_content))
    print(f"Successfully generated {hlsl_path}")

def port_cmp(glsl_path, hlsl_path):
    header, passes = parse_glsl_passes(glsl_path)
    
    hlsl_content = []
    hlsl_content.append(f"// Generated from {os.path.basename(glsl_path)}")
    hlsl_content.append(header)
    hlsl_content.append("\n//!MAGPIE EFFECT")
    hlsl_content.append("//!VERSION 4")
    hlsl_content.append("//!USE MulAdd")
    hlsl_content.append("//!CAPABILITY FP16\n")
    hlsl_content.append('#include "../StubDefs.hlsli"\n')
    
    # Add static constants and samplers
    hlsl_content.append("//!TEXTURE")
    hlsl_content.append("Texture2D INPUT;\n")
    hlsl_content.append("//!TEXTURE")
    hlsl_content.append("//!WIDTH INPUT_WIDTH * 2")
    hlsl_content.append("//!HEIGHT INPUT_HEIGHT * 2")
    hlsl_content.append("Texture2D OUTPUT;\n")
    hlsl_content.append("//!SAMPLER")
    hlsl_content.append("//!FILTER POINT")
    hlsl_content.append("SamplerState SP;\n")
    hlsl_content.append("//!SAMPLER")
    hlsl_content.append("//!FILTER LINEAR")
    hlsl_content.append("SamplerState SL;\n")
    
    # Define color space matrices
    hlsl_content.append("//!COMMON")
    hlsl_content.append("static const MF3x3 RY = {0.299, 0.587, 0.114, -0.169, -0.331, 0.5, 0.5, -0.419, -0.081};")
    hlsl_content.append("static const MF3x3 YR = {1, -0.00093, 1.401687, 1, -0.3437, -0.71417, 1, 1.77216, 0.00099};")
    hlsl_content.append("#define LUMA_mul 1.0f")
    hlsl_content.append("#define conv2d_mul 1.0f")
    hlsl_content.append("#define conv2d_1_mul 1.0f")
    hlsl_content.append("#define conv2d_2_mul 1.0f")
    hlsl_content.append("#define conv2d_3_mul 1.0f")
    hlsl_content.append("#define conv2d_4_mul 1.0f")
    hlsl_content.append("#define conv2d_5_mul 1.0f\n")
    
    # Intermediate textures
    hlsl_content.append("//!TEXTURE")
    hlsl_content.append("//!WIDTH INPUT_WIDTH")
    hlsl_content.append("//!HEIGHT INPUT_HEIGHT")
    hlsl_content.append("//!FORMAT R16G16B16A16_FLOAT")
    hlsl_content.append("Texture2D LUMA;\n")
    
    # conv2d to conv2d_5 are horizontally packed 8x wider
    for layer in range(6):
        name = "conv2d" if layer == 0 else f"conv2d_{layer}"
        hlsl_content.append("//!TEXTURE")
        hlsl_content.append("//!WIDTH INPUT_WIDTH * 8")
        hlsl_content.append("//!HEIGHT INPUT_HEIGHT")
        hlsl_content.append("//!FORMAT R16G16B16A16_FLOAT")
        hlsl_content.append(f"Texture2D {name};\n")
        
    hlsl_content.append("//!TEXTURE")
    hlsl_content.append("//!WIDTH INPUT_WIDTH")
    hlsl_content.append("//!HEIGHT INPUT_HEIGHT")
    hlsl_content.append("//!FORMAT R16G16B16A16_FLOAT")
    hlsl_content.append("Texture2D conv2d_6;\n")
    
    # Pass 1: Luma pre-pass
    hlsl_content.append("//!PASS 1")
    hlsl_content.append("//!DESC Luma pre-pass")
    hlsl_content.append("//!BLOCK_SIZE 8")
    hlsl_content.append("//!NUM_THREADS 64")
    hlsl_content.append("//!IN INPUT")
    hlsl_content.append("//!OUT LUMA")
    hlsl_content.append("""void Pass1(uint2 blockStart, uint3 tid) {
	uint2 gxy = Rmp8x8(tid.x) + blockStart;
	uint2 sz = GetInputSize();
	if (gxy.x >= sz.x || gxy.y >= sz.y)
		return;
	float2 pt = float2(GetInputPt());
	float2 pos = (gxy + 0.5) * pt;
	MF3 color = INPUT.SampleLevel(SP, pos, 0).rgb;
	LUMA[gxy] = MF4(dot(MF3(0.299, 0.587, 0.114), color), 0.0, 0.0, 0.0);
}
""")

    # Port all compute passes (from index 0 to 6)
    for idx, p in enumerate(passes[:-1]):
        pass_num = idx + 2
        desc = p['desc']
        directives = p['directives']
        hook_code = p['hook_code']
        
        save_target = directives.get('SAVE', '')
        
        # Get input bindings
        in_bindings = []
        for bind_val in directives.get('BIND', []):
            if bind_val == 'LUMA':
                in_bindings.append('LUMA')
            else:
                in_bindings.append(bind_val)
        if save_target == 'conv2d_6':
            in_bindings.append('INPUT')
        
        # Ensure unique bindings list
        in_bindings_str = ", ".join(sorted(list(set(in_bindings))))
        
        hlsl_content.append(f"//!PASS {pass_num}")
        hlsl_content.append(f"//!DESC {desc}")
        
        # Parse workgroup dimensions tx, ty from compute directive
        compute_val = directives.get('COMPUTE', '')
        if compute_val:
            comp_parts = compute_val.split()
            tx = comp_parts[2]
            ty = comp_parts[3]
        else:
            tx = '2'
            ty = '16'

        # Override dimensions for conv2d_6 to prevent shared memory overflow (limit is 32KB on cs_5_0)
        if save_target == 'conv2d_6':
            tx = '16'
            ty = '8'

        # Output width and block size depend on whether we are outputting a packed texture or 1x texture
        if save_target == 'conv2d_6':
            # Last convolution pass outputs 1x size texture, but we merge it with depth-to-space (2x output scale)
            hlsl_content.append("//!BLOCK_SIZE 32, 16")
            hlsl_content.append(f"//!NUM_THREADS {tx}, {ty}")
        else:
            # Intermediate packed passes output 8x wider texture, but wait:
            # The block size is specified as 16x16 in output coordinates.
            # In output coordinates, the width is 8x wider.
            hlsl_content.append("//!BLOCK_SIZE 16, 16")
            hlsl_content.append(f"//!NUM_THREADS {tx}, {ty}")
            
        hlsl_content.append(f"//!IN {in_bindings_str}")
        if save_target == 'conv2d_6':
            hlsl_content.append("//!OUT OUTPUT\n")
        else:
            hlsl_content.append(f"//!OUT {save_target}\n")
        
        # Translate gl_WorkGroupID / gl_WorkGroupSize / gl_LocalInvocationID in raw hook_code
        if save_target == 'conv2d_6':
            base_def = "uint2 base = blockStart >> 1;"
        else:
            base_def = "uint2 base = uint2(blockStart.x / 8, blockStart.y);"
            
        hook_code = re.sub(
            r'ivec2\s+base\s+=\s+ivec2\(gl_WorkGroupID\)\s*\*\s*ivec2\(gl_WorkGroupSize\);',
            base_def,
            hook_code
        )
        
        # Translate matrix, vector, and barrier
        translated_code = translate_matrix_vector(hook_code)
        
        # Replace:
        # gl_LocalInvocationID
        # to:
        # tid
        translated_code = re.sub(r'\bgl_LocalInvocationID\b', 'tid', translated_code)
        
        # Replace gl_WorkGroupSize.x and gl_WorkGroupSize.y dynamically
        translated_code = re.sub(r'\bgl_WorkGroupSize\.x\b', tx, translated_code)
        translated_code = re.sub(r'\bgl_WorkGroupSize\.y\b', ty, translated_code)
        translated_code = re.sub(r'\bgl_WorkGroupSize\b', f'uint2({tx}, {ty})', translated_code)
        
        # Replace gl_GlobalInvocationID. In GLSL CMP, gl_GlobalInvocationID is in 1x space (since work group size is 2x16).
        # Wait, is gl_GlobalInvocationID used? Let's check:
        # ivec2 store_pos0 = ivec2(gl_GlobalInvocationID) * ivec2(8, 1) + ivec2(0, 0);
        # Since gl_GlobalInvocationID is base + tid.xy:
        translated_code = re.sub(r'\bgl_GlobalInvocationID\b', '(base + tid.xy)', translated_code)
        
        # Replace texelFetch calls using our robust parser
        translated_code = replace_texel_fetch_robust(translated_code)
        
        # Convert imageStore to output assignments
        # e.g., imageStore(out_image, store_pos0, result0); -> conv2d[store_pos0] = result0;
        if save_target == 'conv2d_6':
            subpixel_code = """
    uint2 dest_1x = base + tid.xy;
    uint2 sz = GetOutputSize();
    uint2 gxy = dest_1x << 1;
    
    if (gxy.x < sz.x && gxy.y < sz.y) {
        float2 opt = float2(GetOutputPt());
        float2 pos;
        MF3 rgb;
        MF3 yuv;

        // (0, 0)
        pos = (float2(gxy) + float2(0.5, 0.5)) * opt;
        rgb = INPUT.SampleLevel(SL, pos, 0).rgb;
        yuv = mul(RY, rgb);
        yuv.r = saturate(result0.x);
        OUTPUT[gxy + int2(0, 0)] = MF4(mul(YR, yuv), 1.0);

        // (1, 0)
        if (gxy.x + 1 < sz.x) {
            pos = (float2(gxy) + float2(1.5, 0.5)) * opt;
            rgb = INPUT.SampleLevel(SL, pos, 0).rgb;
            yuv = mul(RY, rgb);
            yuv.r = saturate(result0.y);
            OUTPUT[gxy + int2(1, 0)] = MF4(mul(YR, yuv), 1.0);
        }

        // (0, 1)
        if (gxy.y + 1 < sz.y) {
            pos = (float2(gxy) + float2(0.5, 1.5)) * opt;
            rgb = INPUT.SampleLevel(SL, pos, 0).rgb;
            yuv = mul(RY, rgb);
            yuv.r = saturate(result0.z);
            OUTPUT[gxy + int2(0, 1)] = MF4(mul(YR, yuv), 1.0);
        }

        // (1, 1)
        if (gxy.x + 1 < sz.x && gxy.y + 1 < sz.y) {
            pos = (float2(gxy) + float2(1.5, 1.5)) * opt;
            rgb = INPUT.SampleLevel(SL, pos, 0).rgb;
            yuv = mul(RY, rgb);
            yuv.r = saturate(result0.w);
            OUTPUT[gxy + int2(1, 1)] = MF4(mul(YR, yuv), 1.0);
        }
    }"""
            translated_code = re.sub(r'imageStore\(out_image,\s*(.*?),\s*(.*?)\);', subpixel_code, translated_code)
        else:
            translated_code = re.sub(r'imageStore\(out_image,\s*(.*?),\s*(.*?)\);', rf'{save_target}[\1] = \2;', translated_code)
        
        # Add global declarations above the function body
        isize_x = int(tx) + 2
        isize_y = int(ty) + 2
        
        if save_target == 'conv2d_6':
            inp_decl = f"groupshared MF4 inp[8][{isize_y}][{isize_x}];"
        elif save_target == 'conv2d':
            inp_decl = f"groupshared MF inp[1][{isize_y}][{isize_x}];"
        else:
            inp_decl = f"groupshared MF4 inp[8][{isize_y}][{isize_x}];"
            
        global_decl = f"static const int2 ksize = int2(3, 3);\nstatic const int2 offset = int2(1, 1);\nstatic const uint2 isize = uint2({isize_x}, {isize_y});\n{inp_decl}\n"
        
        # Rewrite the hook function signature to match MagpieFX style
        func_sig = f"void Pass{pass_num}(uint2 blockStart, uint3 tid) {{"
        translated_code = translated_code.replace("void hook() {", func_sig)
        
        hlsl_content.append(global_decl)
        hlsl_content.append(translated_code)
        hlsl_content.append("\n")
        
    # Write to file
    os.makedirs(os.path.dirname(hlsl_path), exist_ok=True)
    with open(hlsl_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(hlsl_content))
    print(f"Successfully generated {hlsl_path}")

if __name__ == "__main__":
    glsl_cmp = r"C:\Users\xiong\Desktop\APP\mpv\mpv\shaders\Ani4Kv2_ArtCNN_C4F32_i2_CMP.glsl"
    
    # Output path inside workspace
    hlsl_cmp = r"src\Effects\ArtCNN\Ani4Kv2_ArtCNN_C4F32_i2.hlsl"
    
    # Convert CMP
    port_cmp(glsl_cmp, hlsl_cmp)

