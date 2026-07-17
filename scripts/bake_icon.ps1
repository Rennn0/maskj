<#
.SYNOPSIS
    Bakes assets/icon.svg into include/av_ui/logo_icon.hpp as raw RGBA bytes,
    at several native sizes for crisp title-bar / taskbar rendering.

.DESCRIPTION
    One-time author-side tool. Rasterizes the app icon ONCE at high resolution
    with headless Edge (Windows built-in; full SVG/CSS/gradient support), then
    downscales to each requested size with high-quality bicubic resampling via
    .NET System.Drawing, and emits a header-only C++ table. The generated header
    has NO dependency on any image library — the pixels are handed straight to
    glfwSetWindowIcon / glTexImage2D at runtime.

    The SVG markup below is the dark-theme-resolved form of assets/icon.svg:
    the class/@media rules are inlined to the dark palette and the window frame
    is fill:none (outline mark). Edit here to change the baked look, then re-run.

.EXAMPLE
    pwsh scripts/bake_icon.ps1                        # 16,24,32,48,64
    pwsh scripts/bake_icon.ps1 -Sizes 16,32,64,256    # custom set
#>
param(
    [int[]]$Sizes = @(16, 24, 32, 48, 64)
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$repo   = Split-Path -Parent $PSScriptRoot
$out    = Join-Path $repo 'include/av_ui/logo_icon.hpp'
$tmp    = [System.IO.Path]::GetTempPath()
$html   = Join-Path $tmp 'arvis_icon.html'
$png    = Join-Path $tmp 'arvis_icon.png'
$render = 256   # rasterize once at this size, then downscale

@"
<!DOCTYPE html><html><head><meta charset="utf-8">
<style>html,body{margin:0;padding:0;background:transparent}</style></head><body>
<svg width="$render" height="$render" viewBox="0 0 120 120" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <linearGradient id="av-accent" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0" stop-color="#38bdf8"/><stop offset="1" stop-color="#6366f1"/>
    </linearGradient>
  </defs>
  <g transform="translate(14 14)">
    <rect x="1" y="1" width="90" height="90" rx="20" fill="none" stroke="#475569" stroke-width="3"/>
    <circle cx="20" cy="22" r="3.5" fill="#475569"/>
    <circle cx="33" cy="22" r="3.5" fill="#475569"/>
    <circle cx="46" cy="22" r="3.5" fill="#475569"/>
    <path d="M22 50 H62" stroke="url(#av-accent)" stroke-width="6" stroke-linecap="round"/>
    <path d="M55 41 L66 50 L55 59" stroke="url(#av-accent)" stroke-width="6" stroke-linecap="round" stroke-linejoin="round" fill="none"/>
    <path d="M70 70 H30" stroke="#22c55e" stroke-width="6" stroke-linecap="round"/>
    <path d="M37 61 L26 70 L37 79" stroke="#22c55e" stroke-width="6" stroke-linecap="round" stroke-linejoin="round" fill="none"/>
  </g>
</svg></body></html>
"@ | Set-Content -Path $html -Encoding UTF8

$edge = @(
    "$env:ProgramFiles (x86)\Microsoft\Edge\Application\msedge.exe",
    "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $edge) { throw "Microsoft Edge not found; install it or use another headless browser." }

$fileUrl = "file:///" + ($html -replace '\\', '/')
& $edge --headless=new --disable-gpu --hide-scrollbars --no-first-run --force-device-scale-factor=1 `
    --user-data-dir="$tmp\arvis_edge" --default-background-color=00000000 `
    --window-size="$render,$render" --screenshot="$png" $fileUrl 2>$null | Out-Null

$srcRaw = [System.Drawing.Bitmap]::new($png)
# Normalize to a known 32bpp ARGB surface.
$src = [System.Drawing.Bitmap]::new($srcRaw.Width, $srcRaw.Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g0  = [System.Drawing.Graphics]::FromImage($src)
$g0.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
$g0.DrawImage($srcRaw, 0, 0, $srcRaw.Width, $srcRaw.Height)
$g0.Dispose(); $srcRaw.Dispose()

function ConvertTo-Rgba([System.Drawing.Bitmap]$bmp) {
    $w = $bmp.Width; $h = $bmp.Height
    $rect = [System.Drawing.Rectangle]::new(0, 0, $w, $h)
    $d = $bmp.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $stride = $d.Stride
    $buf = [byte[]]::new($stride * $h)
    [System.Runtime.InteropServices.Marshal]::Copy($d.Scan0, $buf, 0, $buf.Length)
    $bmp.UnlockBits($d)
    $out = [byte[]]::new($w * $h * 4)
    $o = 0
    for ($y = 0; $y -lt $h; $y++) {
        $ro = $y * $stride
        for ($x = 0; $x -lt $w; $x++) {
            $p = $ro + $x * 4         # BGRA in memory -> RGBA out
            $out[$o++] = $buf[$p + 2]; $out[$o++] = $buf[$p + 1]
            $out[$o++] = $buf[$p];     $out[$o++] = $buf[$p + 3]
        }
    }
    return , $out
}

$sb = [System.Text.StringBuilder]::new()
[void]$sb.AppendLine('#pragma once')
[void]$sb.AppendLine('// AUTO-GENERATED from assets/icon.svg by scripts/bake_icon.ps1 — do not hand-edit.')
[void]$sb.AppendLine('// Raw RGBA8 pixels (straight alpha), row-major, top row first. No decoder needed.')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('namespace avUi')
[void]$sb.AppendLine('{')
[void]$sb.AppendLine('    struct LogoImage { int width; int height; unsigned char *pixels; };')
[void]$sb.AppendLine('')

$sorted = $Sizes | Sort-Object -Unique
foreach ($s in $sorted) {
    $dst = [System.Drawing.Bitmap]::new($s, $s, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($dst)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode   = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingMode   = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
    $g.DrawImage($src, [System.Drawing.Rectangle]::new(0, 0, $s, $s))
    $g.Dispose()
    $rgba = ConvertTo-Rgba $dst
    $dst.Dispose()

    [void]$sb.AppendLine("    inline unsigned char logo_icon_rgba_$s[] = {")
    $line = '        '; $col = 0
    foreach ($byte in $rgba) {
        $line += "$byte,"
        if (++$col -eq 16) { [void]$sb.AppendLine($line); $line = '        '; $col = 0 }
    }
    if ($col -gt 0) { [void]$sb.AppendLine($line) }
    [void]$sb.AppendLine('    };')
    [void]$sb.AppendLine('')
}
$src.Dispose()

[void]$sb.AppendLine('    // All native sizes — pass the whole table to glfwSetWindowIcon and let')
[void]$sb.AppendLine('    // GLFW/Windows pick the best match per surface (title bar, taskbar, Alt-Tab).')
[void]$sb.AppendLine('    inline LogoImage logo_icon_images[] = {')
foreach ($s in $sorted) {
    [void]$sb.AppendLine("        {$s, $s, logo_icon_rgba_$s},")
}
[void]$sb.AppendLine('    };')
[void]$sb.AppendLine("    inline constexpr int logo_icon_count = $($sorted.Count);")
[void]$sb.AppendLine('')
$max = $sorted[-1]
[void]$sb.AppendLine('    // Largest image — convenient for an in-UI ImGui logo texture.')
[void]$sb.AppendLine("    inline constexpr int logo_icon_width = $max;")
[void]$sb.AppendLine("    inline constexpr int logo_icon_height = $max;")
[void]$sb.AppendLine("    inline unsigned char *const logo_icon_rgba = logo_icon_rgba_$max;")
[void]$sb.AppendLine('}')

[System.IO.File]::WriteAllText($out, $sb.ToString())
Write-Host "Wrote $out : sizes $($sorted -join ', ')"
