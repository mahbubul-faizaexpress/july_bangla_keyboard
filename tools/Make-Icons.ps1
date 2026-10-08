<#
.SYNOPSIS
    Generates every app icon from the logo artwork (resources/logo.jpg).

.DESCRIPTION
    Outputs:
      resources/JulyBangla.ico                    Windows program and installer
      resources/JulyTip.ico                       Windows text service (keyboard list)
      android/.../drawable-xxxhdpi/ic_launcher_art.jpg   Android adaptive icon layer
      android/.../drawable-xhdpi/ic_launcher.png         Android icon before 8.0

    The logo is a rounded tile on a light background; the tile is cut out and its corners
    are redrawn with transparency. Run again whenever resources/logo.jpg changes.
#>
param([string]$Source = (Join-Path $PSScriptRoot '..\resources\logo.jpg'))

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$androidRes = Join-Path $root 'android\app\src\main\res'

$art = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Source).Path)
# The tile inside the 1024x1024 artwork, measured on the image.
$scale = $art.Width / 1024.0
$tile = New-Object System.Drawing.Rectangle ([int](190 * $scale)), ([int](190 * $scale)), ([int](645 * $scale)), ([int](645 * $scale))
$cornerRatio = 0.19   # corner radius / tile size; slightly larger than the artwork's own
$navy = [System.Drawing.Color]::FromArgb(255, 27, 29, 78)

function New-Canvas([int]$size) {
    $bitmap = New-Object System.Drawing.Bitmap $size, $size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bitmap)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    return $bitmap, $g
}

# The tile scaled to $size x $size, with anti-aliased transparent rounded corners.
function Get-Tile([int]$size) {
    $plain, $g = New-Canvas $size
    $g.DrawImage($art, (New-Object System.Drawing.Rectangle 0, 0, $size, $size), $tile, [System.Drawing.GraphicsUnit]::Pixel)
    $g.Dispose()

    $rounded, $g = New-Canvas $size
    $d = [single]($size * $cornerRatio * 2)
    $e = [single]($size - $d)
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $path.AddArc(0, 0, $d, $d, 180, 90)
    $path.AddArc($e, 0, $d, $d, 270, 90)
    $path.AddArc($e, $e, $d, $d, 0, 90)
    $path.AddArc(0, $e, $d, $d, 90, 90)
    $path.CloseFigure()
    $brush = New-Object System.Drawing.TextureBrush $plain
    $g.FillPath($brush, $path)
    $brush.Dispose(); $path.Dispose(); $g.Dispose(); $plain.Dispose()
    return $rounded
}

function Get-PngBytes($bitmap) {
    $stream = New-Object System.IO.MemoryStream
    $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
    return , $stream.ToArray()
}

# --- Windows .ico files (PNG-compressed entries) ---------------------------------------
function Write-Ico([int[]]$sizes, [string]$path) {
    $images = foreach ($s in $sizes) { $t = Get-Tile $s; , (Get-PngBytes $t); $t.Dispose() }
    $ico = New-Object System.IO.MemoryStream
    $w = New-Object System.IO.BinaryWriter $ico
    $w.Write([uint16]0); $w.Write([uint16]1); $w.Write([uint16]$sizes.Count)
    $offset = 6 + 16 * $sizes.Count
    for ($i = 0; $i -lt $sizes.Count; $i++) {
        $dim = if ($sizes[$i] -ge 256) { 0 } else { $sizes[$i] }
        $w.Write([byte]$dim); $w.Write([byte]$dim); $w.Write([byte]0); $w.Write([byte]0)
        $w.Write([uint16]1); $w.Write([uint16]32)
        $w.Write([uint32]$images[$i].Length); $w.Write([uint32]$offset)
        $offset += $images[$i].Length
    }
    foreach ($img in $images) { $w.Write($img) }
    $w.Flush()
    [System.IO.File]::WriteAllBytes($path, $ico.ToArray())
}

# The program and installer get every size up to 256 (Explorer's large icons); the text
# service DLL, loaded into every app, only needs the small sizes of the keyboard list.
Write-Ico @(16, 20, 24, 32, 40, 48, 64, 256) (Join-Path $root 'resources\JulyBangla.ico')
Write-Ico @(16, 20, 24, 32, 40, 48) (Join-Path $root 'resources\JulyTip.ico')

# --- Android ----------------------------------------------------------------------------
$xxxhdpi = Join-Path $androidRes 'drawable-xxxhdpi'
New-Item -ItemType Directory -Force $xxxhdpi | Out-Null

# Adaptive icon layer: 108dp at 4x. Launchers show only the middle 72dp (in their own
# shape), so the tile is drawn 85dp wide: its artwork stays inside the 66dp safe zone.
$layer, $g = New-Canvas 432
$g.Clear($navy)
$t = Get-Tile 340
$g.DrawImage($t, 46, 46, 340, 340)
$t.Dispose(); $g.Dispose()
$jpeg = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq 'image/jpeg' }
$quality = New-Object System.Drawing.Imaging.EncoderParameters 1
$quality.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter ([System.Drawing.Imaging.Encoder]::Quality), 90L
$layer.Save((Join-Path $xxxhdpi 'ic_launcher_art.jpg'), $jpeg, $quality)
$layer.Dispose()

# Android 7.x (before adaptive icons): the rounded tile itself, 48dp at 2x (scaled for
# other densities; only about 1% of phones still run 7.x).
$xhdpi = Join-Path $androidRes 'drawable-xhdpi'
New-Item -ItemType Directory -Force $xhdpi | Out-Null
$t = Get-Tile 96
$t.Save((Join-Path $xhdpi 'ic_launcher.png'), [System.Drawing.Imaging.ImageFormat]::Png)
$t.Dispose()

# The remembrance artwork for the Android splash (same file as the Windows splash).
New-Item -ItemType Directory -Force (Join-Path $androidRes 'drawable-nodpi') | Out-Null
Copy-Item (Join-Path $root 'resources\splash.jpg') (Join-Path $androidRes 'drawable-nodpi\july_splash.jpg')

# --- Website ----------------------------------------------------------------------------
$web = Join-Path $root 'website\assets'
New-Item -ItemType Directory -Force $web | Out-Null
$t = Get-Tile 256
$t.Save((Join-Path $web 'logo.png'), [System.Drawing.Imaging.ImageFormat]::Png)
$t.Dispose()
$t = Get-Tile 64
$t.Save((Join-Path $web 'favicon.png'), [System.Drawing.Imaging.ImageFormat]::Png)
$t.Dispose()
Copy-Item (Join-Path $root 'resources\splash.jpg') (Join-Path $web 'july36.jpg')

$art.Dispose()
Write-Host 'Icons written.'
