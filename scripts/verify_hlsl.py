import os
import sys
import ctypes
from ctypes import wintypes
import re

# Load d3dcompiler_47.dll
try:
    d3d = ctypes.windll.d3dcompiler_47
except Exception as e:
    try:
        d3d = ctypes.CDLL("d3dcompiler_47.dll")
    except Exception as e2:
        print(f"Error loading d3dcompiler_47.dll: {e2}")
        d3d = None

if d3d:
    d3d.D3DCompile.argtypes = [
        ctypes.c_void_p,       # pSrcData
        ctypes.c_size_t,       # SrcDataSize
        ctypes.c_char_p,       # pSourceName
        ctypes.c_void_p,       # pDefines
        ctypes.c_void_p,       # pInclude
        ctypes.c_char_p,       # pEntrypoint
        ctypes.c_char_p,       # pTarget
        ctypes.c_uint,         # Flags1
        ctypes.c_uint,         # Flags2
        ctypes.POINTER(ctypes.c_void_p),  # ppCode
        ctypes.POINTER(ctypes.c_void_p),  # ppErrorMsgs
    ]
    d3d.D3DCompile.restype = ctypes.c_int32

class ID3DBlob:
    def __init__(self, ptr):
        self.ptr = ptr
        # Dereference the pointer to get the vtable
        self.vtbl = ctypes.cast(ptr, ctypes.POINTER(ctypes.c_void_p))
        
    def get_buffer_pointer(self):
        func_ptr = ctypes.cast(self.vtbl[0], ctypes.POINTER(ctypes.c_void_p))[3]
        func = ctypes.WINFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p)(func_ptr)
        return func(self.ptr)
        
    def get_buffer_size(self):
        func_ptr = ctypes.cast(self.vtbl[0], ctypes.POINTER(ctypes.c_void_p))[4]
        func = ctypes.WINFUNCTYPE(ctypes.c_size_t, ctypes.c_void_p)(func_ptr)
        return func(self.ptr)
        
    def release(self):
        func_ptr = ctypes.cast(self.vtbl[0], ctypes.POINTER(ctypes.c_void_p))[2]
        func = ctypes.WINFUNCTYPE(ctypes.c_uint32, ctypes.c_void_p)(func_ptr)
        return func(self.ptr)

# HLSL Type mapping for formats
FORMAT_MAP = {
    "R32G32B32A32_FLOAT": ("float4", "float4"),
    "R16G16B16A16_FLOAT": ("MF4", "MF4"),
    "R16G16B16A16_UNORM": ("MF4", "unorm MF4"),
    "R16G16B16A16_SNORM": ("MF4", "snorm MF4"),
    "R32G32_FLOAT": ("float2", "float2"),
    "R10G10B10A2_UNORM": ("MF4", "unorm MF4"),
    "R11G11B10_FLOAT": ("MF3", "MF3"),
    "R8G8B8A8_UNORM": ("MF4", "unorm MF4"),
    "R8G8B8A8_SNORM": ("MF4", "snorm MF4"),
    "R16G16_FLOAT": ("MF2", "MF2"),
    "R16G16_UNORM": ("MF2", "unorm MF2"),
    "R16G16_SNORM": ("MF2", "snorm MF2"),
    "R32_FLOAT": ("float", "float"),
    "R8G8_UNORM": ("MF2", "unorm MF2"),
    "R8G8_SNORM": ("MF2", "snorm MF2"),
    "R16_FLOAT": ("MF", "MF"),
    "R16_UNORM": ("MF", "unorm MF"),
    "R16_SNORM": ("MF", "snorm MF"),
    "R8_UNORM": ("MF", "unorm MF"),
    "R8_SNORM": ("MF", "snorm MF"),
}

def parse_magpie_fx(file_path):
    with open(file_path, "r", encoding="utf-8") as f:
        lines = f.readlines()
        
    textures = {"INPUT": ("R8G8B8A8_UNORM", "INPUT_WIDTH", "INPUT_HEIGHT"), "OUTPUT": ("R8G8B8A8_UNORM", None, None)}
    samplers = []
    common_blocks = []
    pass_blocks = []
    
    current_block_type = None
    current_block_lines = []
    
    def end_current_block():
        nonlocal current_block_type, current_block_lines
        if current_block_type is None:
            return
        block_text = "".join(current_block_lines)
        if current_block_type == "COMMON":
            common_blocks.append(block_text)
        elif current_block_type == "PASS":
            pass_blocks.append(block_text)
        elif current_block_type == "TEXTURE":
            # Extract texture name and format
            name = None
            fmt = "R8G8B8A8_UNORM"
            for line in current_block_lines:
                line_str = line.strip()
                if line_str.startswith("//!FORMAT"):
                    fmt = line_str.split()[1]
                elif not line_str.startswith("//!") and "Texture2D" in line_str:
                    match = re.search(r"Texture2D\s+(\w+)\s*;", line_str)
                    if match:
                        name = match.group(1)
            if name:
                textures[name] = (fmt, None, None)
        elif current_block_type == "SAMPLER":
            name = None
            for line in current_block_lines:
                line_str = line.strip()
                if not line_str.startswith("//!") and "SamplerState" in line_str:
                    match = re.search(r"SamplerState\s+(\w+)\s*;", line_str)
                    if match:
                        name = match.group(1)
            if name:
                samplers.append(name)
        current_block_lines = []
        current_block_type = None
        
    for line in lines:
        line_str = line.strip()
        if line_str.startswith("//!"):
            parts = line_str[3:].split()
            if not parts:
                current_block_lines.append(line)
                continue
            cmd = parts[0].upper()
            if cmd in ["TEXTURE", "SAMPLER", "COMMON", "PASS", "PARAMETER"]:
                end_current_block()
                current_block_type = cmd
                current_block_lines.append(line)
            else:
                current_block_lines.append(line)
        else:
            if current_block_type:
                current_block_lines.append(line)
                
    end_current_block()
    return textures, samplers, common_blocks, pass_blocks

def verify_shader(file_path):
    if not d3d:
        print("D3DCompiler dll not loaded, cannot verify.")
        return False
        
    print(f"\nVerifying {file_path}...")
    textures, samplers, common_blocks, pass_blocks = parse_magpie_fx(file_path)
    
    # Check capability FP16
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()
    use_fp16 = "//!CAPABILITY FP16" in content
    use_mul_add = "//!USE MulAdd" in content
    
    # Base macro definitions
    macros = []
    if use_fp16:
        macros.extend([
            ("MP_FP16", ""),
            ("MF", "min16float"),
            ("MF2", "min16float2"),
            ("MF3", "min16float3"),
            ("MF4", "min16float4"),
            ("MF2x2", "min16float2x2"),
            ("MF3x3", "min16float3x3"),
            ("MF4x4", "min16float4x4"),
            ("MF2x3", "min16float2x3"),
            ("MF2x4", "min16float2x4"),
            ("MF3x2", "min16float3x2"),
            ("MF3x4", "min16float3x4"),
            ("MF4x2", "min16float4x2"),
            ("MF4x3", "min16float4x3"),
        ])
    else:
        macros.extend([
            ("MF", "float"),
            ("MF2", "float2"),
            ("MF3", "float3"),
            ("MF4", "float4"),
            ("MF2x2", "float2x2"),
            ("MF3x3", "float3x3"),
            ("MF4x4", "float4x4"),
            ("MF2x3", "float2x3"),
            ("MF2x4", "float2x4"),
            ("MF3x2", "float3x2"),
            ("MF3x4", "float3x4"),
            ("MF4x2", "float4x2"),
            ("MF4x3", "float4x3"),
        ])
        
    all_success = True
    
    for idx, pass_block in enumerate(pass_blocks):
        pass_idx = idx + 1
        
        # Parse pass directives
        directives = {}
        pass_lines = pass_block.splitlines()
        code_lines = []
        for line in pass_lines:
            line_str = line.strip()
            if line_str.startswith("//!"):
                parts = line_str[3:].split(maxsplit=1)
                if len(parts) == 1:
                    directives[parts[0].upper()] = ""
                elif len(parts) == 2:
                    directives[parts[0].upper()] = parts[1]
            else:
                code_lines.append(line)
                
        desc = directives.get("DESC", f"Pass {pass_idx}")
        in_str = directives.get("IN", "")
        out_str = directives.get("OUT", "")
        num_threads_str = directives.get("NUM_THREADS", "64")
        block_size_str = directives.get("BLOCK_SIZE", "8")
        
        inputs = [i.strip() for i in in_str.split(",") if i.strip()]
        outputs = [o.strip() for o in out_str.split(",") if o.strip()]
        
        # Determine block size
        bs_parts = [int(x) for x in block_size_str.split(",") if x.strip()]
        if len(bs_parts) == 1:
            block_width = bs_parts[0]
            block_height = bs_parts[0]
        else:
            block_width = bs_parts[0]
            block_height = bs_parts[1]
            
        # Determine num threads
        nt_parts = [int(x) for x in num_threads_str.split(",") if x.strip()]
        if len(nt_parts) == 1:
            nt_x = nt_parts[0]
            nt_y = 1
            nt_z = 1
        elif len(nt_parts) == 2:
            nt_x = nt_parts[0]
            nt_y = nt_parts[1]
            nt_z = 1
        else:
            nt_x = nt_parts[0]
            nt_y = nt_parts[1]
            nt_z = nt_parts[2]
            
        # Generate pass HLSL source
        pass_source = []
        
        # CB1
        pass_source.append("""cbuffer __CB1 : register(b0) {
	uint2 __inputSize;
	uint2 __outputSize;
	float2 __inputPt;
	float2 __outputPt;
	float2 __scale;
};""")
        
        # SRVs (inputs)
        for i, in_name in enumerate(inputs):
            fmt = textures.get(in_name, ("R8G8B8A8_UNORM",))[0]
            srv_type = FORMAT_MAP.get(fmt, ("MF4", "MF4"))[0]
            pass_source.append(f"Texture2D<{srv_type}> {in_name} : register(t{i});")
            
        # UAVs (outputs)
        for i, out_name in enumerate(outputs):
            fmt = textures.get(out_name, ("R8G8B8A8_UNORM",))[0]
            uav_type = FORMAT_MAP.get(fmt, ("MF4", "MF4"))[1]
            pass_source.append(f"RWTexture2D<{uav_type}> {out_name} : register(u{i});")
            
        # Samplers
        for i, sampler_name in enumerate(samplers):
            pass_source.append(f"SamplerState {sampler_name} : register(s{i});")
            
        # Built-in functions
        pass_source.append("""uint __Bfe(uint src, uint off, uint bits) { uint mask = (1u << bits) - 1; return (src >> off) & mask; }
uint __BfiM(uint src, uint ins, uint bits) { uint mask = (1u << bits) - 1; return (ins & mask) | (src & (~mask)); }
uint2 Rmp8x8(uint a) { return uint2(__Bfe(a, 1u, 3u), __BfiM(__Bfe(a, 3u, 3u), a, 1u)); }
uint2 GetInputSize() { return __inputSize; }
float2 GetInputPt() { return __inputPt; }
uint2 GetOutputSize() { return __outputSize; }
float2 GetOutputPt() { return __outputPt; }
float2 GetScale() { return __scale; }
""")

        if use_mul_add:
            pass_source.append("""
MF2 MulAdd(MF2 x, MF2x2 y, MF2 a) {
	MF2 result = a;
	result = mad(x.x, y._m00_m01, result);
	result = mad(x.y, y._m10_m11, result);
	return result;
}
MF3 MulAdd(MF2 x, MF2x3 y, MF3 a) {
	MF3 result = a;
	result = mad(x.x, y._m00_m01_m02, result);
	result = mad(x.y, y._m10_m11_m12, result);
	return result;
}
MF4 MulAdd(MF2 x, MF2x4 y, MF4 a) {
	MF4 result = a;
	result = mad(x.x, y._m00_m01_m02_m03, result);
	result = mad(x.y, y._m10_m11_m12_m13, result);
	return result;
}
MF2 MulAdd(MF3 x, MF3x2 y, MF2 a) {
	MF2 result = a;
	result = mad(x.x, y._m00_m01, result);
	result = mad(x.y, y._m10_m11, result);
	result = mad(x.z, y._m20_m21, result);
	return result;
}
MF3 MulAdd(MF3 x, MF3x3 y, MF3 a) {
	MF3 result = a;
	result = mad(x.x, y._m00_m01_m02, result);
	result = mad(x.y, y._m10_m11_m12, result);
	result = mad(x.z, y._m20_m21_m22, result);
	return result;
}
MF4 MulAdd(MF3 x, MF3x4 y, MF4 a) {
	MF4 result = a;
	result = mad(x.x, y._m00_m01_m02_m03, result);
	result = mad(x.y, y._m10_m11_m12_m13, result);
	result = mad(x.z, y._m20_m21_m22_m23, result);
	return result;
}
MF2 MulAdd(MF4 x, MF4x2 y, MF2 a) {
	MF2 result = a;
	result = mad(x.x, y._m00_m01, result);
	result = mad(x.y, y._m10_m11, result);
	result = mad(x.z, y._m20_m21, result);
	result = mad(x.w, y._m30_m31, result);
	return result;
}
MF3 MulAdd(MF4 x, MF4x3 y, MF3 a) {
	MF3 result = a;
	result = mad(x.x, y._m00_m01_m02, result);
	result = mad(x.y, y._m10_m11_m12, result);
	result = mad(x.z, y._m20_m21_m22, result);
	result = mad(x.w, y._m30_m31_m32, result);
	return result;
}
MF4 MulAdd(MF4 x, MF4x4 y, MF4 a) {
	MF4 result = a;
	result = mad(x.x, y._m00_m01_m02_m03, result);
	result = mad(x.y, y._m10_m11_m12_m13, result);
	result = mad(x.z, y._m20_m21_m22_m23, result);
	result = mad(x.w, y._m30_m31_m32_m33, result);
	return result;
}
""")
        
        # Append common blocks
        for common_block in common_blocks:
            pass_source.append(common_block)
            
        # Append pass body
        pass_source.append("\n".join(code_lines))
        
        # Wrap entry point
        # Big block start logic:
        import math
        if block_width == block_height and (block_width & (block_width - 1) == 0):
            n_shift = int(math.log2(block_width))
            block_start_expr = f"(gid.xy << {n_shift})"
        else:
            block_start_expr = f"gid.xy * uint2({block_width}, {block_height})"
            
        pass_source.append(f"""
[numthreads({nt_x}, {nt_y}, {nt_z})]
void __M(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID) {{
	Pass{pass_idx}({block_start_expr}, tid);
}}
""")
        
        full_source = "\n".join(pass_source)
        
        # Prepare COM call to D3DCompile
        source_bytes = full_source.encode("utf-8")
        source_name = f"Pass{pass_idx}".encode("utf-8")
        entry_point = b"__M"
        target = b"cs_5_0"
        
        # Build macro structure
        MacroArrayType = ctypes.c_char_p * (2 * (len(macros) + 1))
        macro_strings = []
        for name, value in macros:
            macro_strings.append(name.encode("utf-8"))
            macro_strings.append(value.encode("utf-8"))
        macro_strings.extend([None, None])
        
        # We need a D3D_SHADER_MACRO struct array
        class D3D_SHADER_MACRO(ctypes.Structure):
            _fields_ = [("Name", ctypes.c_char_p), ("Definition", ctypes.c_char_p)]
            
        macro_array = (D3D_SHADER_MACRO * (len(macros) + 1))()
        for i, (name, value) in enumerate(macros):
            macro_array[i].Name = name.encode("utf-8")
            macro_array[i].Definition = value.encode("utf-8")
        macro_array[len(macros)].Name = None
        macro_array[len(macros)].Definition = None
        
        # We use standard file include handler
        p_include = ctypes.c_void_p(1)
        
        flags1 = 0x8000  # D3DCOMPILE_ENABLE_STRICTNESS
        # Add FP16 or warnings/errors options if needed
        
        p_code = ctypes.c_void_p()
        p_errors = ctypes.c_void_p()
        
        hr = d3d.D3DCompile(
            source_bytes,
            len(source_bytes),
            source_name,
            ctypes.byref(macro_array),
            p_include,
            entry_point,
            target,
            flags1,
            0,
            ctypes.byref(p_code),
            ctypes.byref(p_errors)
        )
        
        if hr < 0:
            all_success = False
            print(f"\n[ERROR] Pass {pass_idx} ({desc}) failed to compile!")
            # Save the failed pass source code to a file for debugging
            failed_file = f"Pass{pass_idx}_failed.hlsl"
            with open(failed_file, "w", encoding="utf-8") as ff:
                ff.write(full_source)
            print(f"Saved failed HLSL source to {failed_file}")
            if p_errors.value:
                errors_blob = ID3DBlob(p_errors)
                err_ptr = errors_blob.get_buffer_pointer()
                err_size = errors_blob.get_buffer_size()
                err_msg = ctypes.string_at(err_ptr, err_size).decode("utf-8", errors="replace")
                print(err_msg)
                errors_blob.release()
                
                # Print code lines around error
                # Parse lines with numbers
                err_lines = re.findall(r"Pass\d+(?:\.hlsl)?\((\d+),", err_msg)
                if err_lines:
                    line_num = int(err_lines[0])
                    print(f"Code around error (line {line_num}):")
                    source_lines = full_source.splitlines()
                    start_l = max(1, line_num - 5)
                    end_l = min(len(source_lines), line_num + 5)
                    for l in range(start_l, end_l + 1):
                        marker = "-> " if l == line_num else "   "
                        print(f"{marker}{l}: {source_lines[l-1]}")
            else:
                print(f"Unknown HRESULT error: {hex(hr & 0xffffffff)}")
        else:
            if p_code.value:
                code_blob = ID3DBlob(p_code)
                code_blob.release()
            if p_errors.value:
                errors_blob = ID3DBlob(p_errors)
                errors_blob.release()
            print(f"[OK] Pass {pass_idx} ({desc}): OK")
            
    return all_success

if __name__ == "__main__":
    if len(sys.argv) > 1:
        verify_shader(sys.argv[1])
    else:
        # Default verification for ArtCNN shaders
        verify_shader(r"src\Effects\ArtCNN\ArtCNN_C4F32_i2.hlsl")
        verify_shader(r"src\Effects\ArtCNN\ArtCNN_C4F32_i2_CMP.hlsl")
