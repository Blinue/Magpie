$DIR = $PSScriptRoot

$anti_ringing_strength = 0.8

function gen_nnedi3 {
    foreach($nns in @(16, 32, 64, 128, 256)) {
        foreach($win in @('8x4', '8x6')) {
            $file_name = "nnedi3-nns$nns-win$win.hlsl"
            python.exe "$DIR\nnedi3.py" --nns "$nns" --win "$win" --use-compute-shader --use-magpie | Out-File -Encoding ASCII "$file_name"
        }
    }
}

function gen_ravu {
    param (
        $float_format
    )

    foreach($target in @('luma', 'rgb')) {
        $suffix = "-$target"
        if ($target -eq "luma") {
            $suffix = ""
        }
        foreach($radius in @(2, 3, 4)) {
            $file_name = "ravu-r$radius$suffix.hlsl"
            $weights_file = "$DIR\weights\ravu_weights-r$radius.py"
            python.exe "$DIR\ravu.py" --target "$target" --weights-file "$weights_file" --float-format "$float_format" --use-compute-shader --use-magpie --overwrite | Out-File -Encoding ASCII "$file_name"
        }
    }

    foreach($radius in @(2, 3, 4)) {
        $file_name = "ravu-lite-r$radius.hlsl"
        $file_name_ar = "ravu-lite-ar-r$radius.hlsl"
        $weights_file = "$DIR\weights\ravu-lite_weights-r$radius.py"
        python.exe "$DIR\ravu-lite.py" --weights-file "$weights_file" --float-format "$float_format" --use-compute-shader --use-magpie --overwrite | Out-File -Encoding ASCII "$file_name"
        python.exe "$DIR\ravu-lite.py" --weights-file "$weights_file" --float-format "$float_format" --use-compute-shader --anti-ringing "$anti_ringing_strength" --use-magpie --overwrite | Out-File -Encoding ASCII "$file_name_ar"
    }

    foreach($target in @('luma', 'rgb')) {
        $suffix = "-$target"
        if ($target -eq "luma") {
            $suffix = ""
        }
        foreach($radius in @(2, 3, 4)) {
            $file_name = "ravu-3x-r$radius$suffix.hlsl"
            $weights_file = "$DIR\weights\ravu-3x_weights-r$radius.py"
            python.exe "$DIR\ravu-3x.py" --target "$target" --weights-file "$weights_file" --float-format "$float_format" --use-magpie --overwrite | Out-File -Encoding ASCII "$file_name"
        }
    }

    foreach($target in @('luma', 'rgb')) {
        $suffix = "-$target"
        if ($target -eq "luma") {
            $suffix = ""
        }
        foreach($radius in @(2, 3)) {
            $file_name = "ravu-zoom-r$radius$suffix.hlsl"
            $file_name_ar = "ravu-zoom-ar-r$radius$suffix.hlsl"
            $weights_file = "$DIR\weights\ravu-zoom_weights-r$radius.py"
            python.exe "$DIR\ravu-zoom.py" --target "$target" --weights-file "$weights_file" --float-format "$float_format" --use-compute-shader --use-magpie --overwrite | Out-File -Encoding ASCII "$file_name"
            python.exe "$DIR\ravu-zoom.py" --target "$target" --weights-file "$weights_file" --float-format "$float_format" --use-compute-shader --anti-ringing "$anti_ringing_strength" --use-magpie --overwrite | Out-File -Encoding ASCII "$file_name_ar"
        }
    }
}

New-Item NNEDI3 -ItemType Directory -ea 0 | Out-Null
Push-Location NNEDI3
gen_nnedi3
Pop-Location

New-Item RAVU -ItemType Directory -ea 0 | Out-Null
Push-Location RAVU
gen_ravu float16dx
Pop-Location
