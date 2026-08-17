param(
    [string]$PlaneSource = "C:\Users\徘徊\Desktop\43d870bb20f641a5ace2970a125ea835.png",
    [string]$BackgroundSource = "C:\Users\徘徊\Desktop\e7fb749c77ab425cb57fb2228f178ebe.png"
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$projectRoot = Split-Path -Parent $PSScriptRoot
$assetDir = Join-Path $projectRoot "app\src\main\assets"
$rawDir = Join-Path $projectRoot "app\src\main\res\raw"
New-Item -ItemType Directory -Force -Path $assetDir, $rawDir | Out-Null

$backgroundAsset = Join-Path $assetDir "canyon_background.png"
if (Test-Path -LiteralPath $BackgroundSource) {
    Copy-Item -LiteralPath $BackgroundSource -Destination $backgroundAsset -Force
} elseif (-not (Test-Path -LiteralPath $backgroundAsset)) {
    throw "Background source and generated asset are both missing"
}

function New-TransparentBitmap([int]$width, [int]$height) {
    $bitmap = New-Object System.Drawing.Bitmap($width, $height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $bitmap.SetResolution(96, 96)
    return $bitmap
}

function Save-Png($bitmap, [string]$name) {
    $path = Join-Path $assetDir $name
    $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
}

function Fill-Polygon($graphics, [string]$color, [object[]]$points) {
    $brush = New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml($color))
    $converted = foreach ($point in $points) { New-Object System.Drawing.Point($point[0], $point[1]) }
    $graphics.FillPolygon($brush, [System.Drawing.Point[]]$converted)
    $brush.Dispose()
}

function Fill-Rect($graphics, [string]$color, [int]$x, [int]$y, [int]$w, [int]$h) {
    $brush = New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml($color))
    $graphics.FillRectangle($brush, $x, $y, $w, $h)
    $brush.Dispose()
}

# Extract the complete central aircraft. The supplied strip is not a regular sprite grid.
$playerAsset = Join-Path $assetDir "player_ship.png"
if (Test-Path -LiteralPath $PlaneSource) {
    $source = [System.Drawing.Bitmap]::FromFile($PlaneSource)
    $player = New-TransparentBitmap 76 87
    $playerGraphics = [System.Drawing.Graphics]::FromImage($player)
    $playerGraphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
    $sourceRect = New-Object System.Drawing.Rectangle(102, 0, 76, 87)
    $destinationRect = New-Object System.Drawing.Rectangle(0, 0, 76, 87)
    $playerGraphics.DrawImage($source, $destinationRect, $sourceRect, [System.Drawing.GraphicsUnit]::Pixel)
    $playerGraphics.Dispose()
    $source.Dispose()
    Save-Png $player "player_ship.png"
} elseif (-not (Test-Path -LiteralPath $playerAsset)) {
    throw "Plane source and generated player asset are both missing"
}

# Enemy drone: compact warm-metal silhouette with a cyan core to match the supplied ship.
$enemy = New-TransparentBitmap 48 48
$g = [System.Drawing.Graphics]::FromImage($enemy)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
Fill-Polygon $g "#241810" @(@(24,46),@(8,31),@(3,18),@(14,21),@(19,3),@(29,3),@(34,21),@(45,18),@(40,31))
Fill-Polygon $g "#7a2c12" @(@(24,43),@(11,29),@(7,22),@(18,26),@(21,7),@(27,7),@(30,26),@(41,22),@(37,29))
Fill-Polygon $g "#f06a18" @(@(24,39),@(16,28),@(20,12),@(28,12),@(32,28))
Fill-Rect $g "#ffc044" 21 12 6 18
Fill-Rect $g "#102b35" 18 28 12 9
Fill-Rect $g "#35d8ff" 21 29 6 6
Fill-Rect $g "#fff2b0" 23 30 2 3
$g.Dispose()
Save-Png $enemy "enemy_drone.png"

# Scout: narrow, fast interceptor.
$scout = New-TransparentBitmap 40 44
$g = [System.Drawing.Graphics]::FromImage($scout)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
Fill-Polygon $g "#27130f" @(@(20,43),@(5,22),@(1,12),@(15,18),@(17,2),@(23,2),@(25,18),@(39,12),@(35,22))
Fill-Polygon $g "#b72d19" @(@(20,39),@(9,22),@(6,16),@(17,21),@(19,6),@(21,6),@(23,21),@(34,16),@(31,22))
Fill-Polygon $g "#ff7b20" @(@(20,34),@(15,22),@(20,9),@(25,22))
Fill-Rect $g "#ffe36a" 18 13 4 12
Fill-Rect $g "#42e8ff" 17 28 6 5
$g.Dispose()
Save-Png $scout "enemy_scout.png"

# Snake fighter: twin-fin silhouette with a bright tracking sensor.
$snake = New-TransparentBitmap 52 48
$g = [System.Drawing.Graphics]::FromImage($snake)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
Fill-Polygon $g "#18172d" @(@(26,47),@(7,37),@(1,21),@(14,25),@(10,8),@(22,17),@(26,1),@(30,17),@(42,8),@(38,25),@(51,21),@(45,37))
Fill-Polygon $g "#5a347f" @(@(26,42),@(11,34),@(7,25),@(18,29),@(15,14),@(24,21),@(26,7),@(28,21),@(37,14),@(34,29),@(45,25),@(41,34))
Fill-Polygon $g "#bd54d7" @(@(26,38),@(19,29),@(23,14),@(29,14),@(33,29))
Fill-Rect $g "#ffeb73" 23 25 6 7
Fill-Rect $g "#ffffff" 25 26 2 3
$g.Dispose()
Save-Png $snake "enemy_snake.png"

# Turret fighter: broad armored body and three visible cannon ports.
$turret = New-TransparentBitmap 60 52
$g = [System.Drawing.Graphics]::FromImage($turret)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
Fill-Polygon $g "#131b1c" @(@(30,51),@(4,42),@(0,24),@(12,17),@(18,4),@(42,4),@(48,17),@(60,24),@(56,42))
Fill-Polygon $g "#3b6260" @(@(30,46),@(9,38),@(6,26),@(16,21),@(21,9),@(39,9),@(44,21),@(54,26),@(51,38))
Fill-Rect $g "#75a89b" 19 14 22 21
Fill-Rect $g "#e7cf70" 27 17 6 12
Fill-Rect $g "#1a2728" 11 34 7 10
Fill-Rect $g "#1a2728" 27 35 6 12
Fill-Rect $g "#1a2728" 42 34 7 10
Fill-Rect $g "#4ff1d0" 25 29 10 5
$g.Dispose()
Save-Png $turret "enemy_turret.png"

# Boss: oversized canyon carrier with a central reactor and side batteries.
$boss = New-TransparentBitmap 112 88
$g = [System.Drawing.Graphics]::FromImage($boss)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
Fill-Polygon $g "#160e12" @(@(56,87),@(15,74),@(1,48),@(8,22),@(29,28),@(36,5),@(76,5),@(83,28),@(104,22),@(111,48),@(97,74))
Fill-Polygon $g "#69212a" @(@(56,80),@(20,69),@(8,48),@(14,31),@(34,36),@(40,12),@(72,12),@(78,36),@(98,31),@(104,48),@(92,69))
Fill-Polygon $g "#bc3e35" @(@(56,74),@(32,61),@(29,42),@(43,35),@(47,17),@(65,17),@(69,35),@(83,42),@(80,61))
Fill-Rect $g "#2a171d" 13 45 22 14
Fill-Rect $g "#2a171d" 77 45 22 14
Fill-Rect $g "#ff9d2e" 19 50 10 5
Fill-Rect $g "#ff9d2e" 83 50 10 5
Fill-Rect $g "#251d29" 44 42 24 23
Fill-Rect $g "#2ad8ef" 48 46 16 15
Fill-Rect $g "#dfffff" 52 49 8 6
Fill-Rect $g "#ffcf4a" 53 19 6 19
$g.Dispose()
Save-Png $boss "boss_carrier.png"

$missile = New-TransparentBitmap 16 32
$g = [System.Drawing.Graphics]::FromImage($missile)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
Fill-Polygon $g "#2b1712" @(@(8,31),@(2,21),@(4,5),@(8,0),@(12,5),@(14,21))
Fill-Polygon $g "#c43b16" @(@(8,28),@(5,20),@(6,6),@(8,3),@(10,6),@(11,20))
Fill-Rect $g "#ffd05a" 7 8 2 13
Fill-Polygon $g "#ff8a20" @(@(5,24),@(8,31),@(11,24))
$g.Dispose()
Save-Png $missile "missile.png"

$playerBullet = New-TransparentBitmap 8 18
$g = [System.Drawing.Graphics]::FromImage($playerBullet)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
Fill-Rect $g "#147a8e" 1 4 6 12
Fill-Rect $g "#42e9ff" 2 1 4 15
Fill-Rect $g "#efffff" 3 0 2 12
$g.Dispose()
Save-Png $playerBullet "bullet_player.png"

$boostBullet = New-TransparentBitmap 10 22
$g = [System.Drawing.Graphics]::FromImage($boostBullet)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
Fill-Polygon $g "#68120e" @(@(5,0),@(9,7),@(8,18),@(5,22),@(2,18),@(1,7))
Fill-Rect $g "#ff321f" 2 5 6 14
Fill-Rect $g "#ff9a2e" 3 2 4 16
Fill-Rect $g "#fff1a3" 4 1 2 12
$g.Dispose()
Save-Png $boostBullet "bullet_player_boost.png"

$enemyBullet = New-TransparentBitmap 12 12
$g = [System.Drawing.Graphics]::FromImage($enemyBullet)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
$outer = New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml("#7a1714"))
$middle = New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml("#ff4c24"))
$inner = New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml("#fff08a"))
$g.FillEllipse($outer, 0, 0, 12, 12); $g.FillEllipse($middle, 2, 2, 8, 8); $g.FillEllipse($inner, 4, 4, 4, 4)
$outer.Dispose(); $middle.Dispose(); $inner.Dispose(); $g.Dispose()
Save-Png $enemyBullet "bullet_enemy.png"

$core = New-TransparentBitmap 24 24
$g = [System.Drawing.Graphics]::FromImage($core)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
Fill-Polygon $g "#12394b" @(@(12,0),@(23,12),@(12,23),@(0,12))
Fill-Polygon $g "#1e9fc4" @(@(12,3),@(20,12),@(12,20),@(3,12))
Fill-Polygon $g "#53eaff" @(@(12,6),@(17,12),@(12,17),@(6,12))
Fill-Rect $g "#e9ffff" 10 8 4 4
$g.Dispose()
Save-Png $core "energy_core.png"

$shield = New-TransparentBitmap 20 20
$g = [System.Drawing.Graphics]::FromImage($shield)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
Fill-Polygon $g "#102b35" @(@(10,0),@(18,4),@(17,13),@(10,20),@(3,13),@(2,4))
Fill-Polygon $g "#29bedd" @(@(10,3),@(15,5),@(14,12),@(10,16),@(6,12),@(5,5))
Fill-Polygon $g "#d9ffff" @(@(10,5),@(13,6),@(12,10),@(10,13),@(8,10),@(7,6))
$g.Dispose()
Save-Png $shield "shield.png"

$heal = New-TransparentBitmap 24 24
$g = [System.Drawing.Graphics]::FromImage($heal)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
Fill-Polygon $g "#143525" @(@(12,1),@(22,6),@(22,18),@(12,23),@(2,18),@(2,6))
Fill-Polygon $g "#35c66c" @(@(12,3),@(20,7),@(20,17),@(12,21),@(4,17),@(4,7))
Fill-Rect $g "#eaffdf" 9 6 6 12
Fill-Rect $g "#eaffdf" 6 9 12 6
$g.Dispose()
Save-Png $heal "pickup_heal.png"

$attack = New-TransparentBitmap 24 24
$g = [System.Drawing.Graphics]::FromImage($attack)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
Fill-Polygon $g "#401a0b" @(@(14,0),@(5,13),@(11,13),@(8,24),@(20,9),@(14,9))
Fill-Polygon $g "#ff9d1c" @(@(14,4),@(8,11),@(13,11),@(11,19),@(17,8),@(13,8))
Fill-Polygon $g "#fff176" @(@(13,7),@(11,10),@(14,10),@(13,14),@(16,9),@(13,9))
$g.Dispose()
Save-Png $attack "pickup_attack.png"

# Dedicated shield pickup is larger than the HUD shield icon.
$shieldPickup = New-TransparentBitmap 24 24
$g = [System.Drawing.Graphics]::FromImage($shieldPickup)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
Fill-Polygon $g "#102b35" @(@(12,0),@(22,5),@(20,16),@(12,24),@(4,16),@(2,5))
Fill-Polygon $g "#29bedd" @(@(12,3),@(18,6),@(17,14),@(12,19),@(7,14),@(6,6))
Fill-Polygon $g "#d9ffff" @(@(12,6),@(15,7),@(14,12),@(12,15),@(10,12),@(9,7))
$g.Dispose()
Save-Png $shieldPickup "pickup_shield.png"

# White-alpha effect textures are tinted by the renderer for each pickup type.
$effectRing = New-TransparentBitmap 64 64
$g = [System.Drawing.Graphics]::FromImage($effectRing)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
$ringPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(230, 255, 255, 255), 3)
$g.DrawEllipse($ringPen, 5, 5, 53, 53)
Fill-Rect $g "#ffffff" 29 1 6 5
Fill-Rect $g "#ffffff" 29 58 6 5
Fill-Rect $g "#ffffff" 1 29 5 6
Fill-Rect $g "#ffffff" 58 29 5 6
$ringPen.Dispose(); $g.Dispose()
Save-Png $effectRing "effect_ring.png"

$shieldAura = New-TransparentBitmap 72 82
$g = [System.Drawing.Graphics]::FromImage($shieldAura)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
$auraFill = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(34, 255, 255, 255))
$auraPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(210, 255, 255, 255), 3)
$auraPoints = [System.Drawing.Point[]]@(
    (New-Object System.Drawing.Point(36, 2)), (New-Object System.Drawing.Point(62, 13)),
    (New-Object System.Drawing.Point(70, 39)), (New-Object System.Drawing.Point(61, 66)),
    (New-Object System.Drawing.Point(36, 80)), (New-Object System.Drawing.Point(11, 66)),
    (New-Object System.Drawing.Point(2, 39)), (New-Object System.Drawing.Point(10, 13)),
    (New-Object System.Drawing.Point(36, 2)))
$g.FillPolygon($auraFill, $auraPoints)
$g.DrawLines($auraPen, $auraPoints)
Fill-Rect $g "#ffffff" 33 0 6 4
Fill-Rect $g "#ffffff" 33 78 6 4
$auraFill.Dispose(); $auraPen.Dispose(); $g.Dispose()
Save-Png $shieldAura "shield_aura.png"

function New-UpgradeIcon([string]$name, [string]$kind) {
    $bitmap = New-TransparentBitmap 32 32
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    Fill-Rect $graphics "#142528" 1 1 30 30
    Fill-Rect $graphics "#58d7d0" 3 3 26 26
    Fill-Rect $graphics "#102023" 6 6 20 20
    if ($kind -eq "damage") {
        Fill-Polygon $graphics "#ffb12e" @(@(16,4),@(12,13),@(5,16),@(12,19),@(16,28),@(20,19),@(27,16),@(20,13))
        Fill-Rect $graphics "#fff2a0" 14 10 4 12
    } elseif ($kind -eq "fire") {
        Fill-Polygon $graphics "#ff6a2e" @(@(17,4),@(9,15),@(14,15),@(10,27),@(24,12),@(18,12))
        Fill-Polygon $graphics "#fff176" @(@(17,8),@(13,14),@(17,14),@(14,21),@(21,13),@(17,13))
    } else {
        Fill-Rect $graphics "#63e480" 13 7 6 18
        Fill-Rect $graphics "#63e480" 7 13 18 6
        Fill-Rect $graphics "#efffe9" 15 9 2 14
        Fill-Rect $graphics "#efffe9" 9 15 14 2
    }
    $graphics.Dispose()
    Save-Png $bitmap $name
}
New-UpgradeIcon "upgrade_damage.png" "damage"
New-UpgradeIcon "upgrade_fire.png" "fire"
New-UpgradeIcon "upgrade_health.png" "health"

$warning = New-TransparentBitmap 24 24
$g = [System.Drawing.Graphics]::FromImage($warning)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
Fill-Polygon $g "#40130a" @(@(12,0),@(24,22),@(0,22))
Fill-Polygon $g "#ffb21c" @(@(12,4),@(20,19),@(4,19))
Fill-Rect $g "#40130a" 10 8 4 7
Fill-Rect $g "#40130a" 10 17 4 3
$g.Dispose()
Save-Png $warning "warning.png"

$explosion = New-TransparentBitmap 192 32
$g = [System.Drawing.Graphics]::FromImage($explosion)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
$sizes = @(4, 8, 13, 15, 11, 5)
for ($frame = 0; $frame -lt 6; $frame++) {
    $cx = $frame * 32 + 16
    $radius = $sizes[$frame]
    $outer = New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml("#d83a12"))
    $middle = New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml("#ff8a18"))
    $inner = New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml("#fff18a"))
    $g.FillEllipse($outer, $cx - $radius, 16 - $radius, $radius * 2, $radius * 2)
    if ($radius -gt 4) { $g.FillEllipse($middle, $cx - $radius + 3, 19 - $radius, ($radius - 3) * 2, ($radius - 3) * 2) }
    if ($radius -gt 7) { $g.FillEllipse($inner, $cx - 4, 12, 8, 8) }
    $outer.Dispose(); $middle.Dispose(); $inner.Dispose()
}
$g.Dispose()
Save-Png $explosion "explosion_strip.png"

# Compact 5x7 uppercase bitmap font: 0-9, A-Z, colon, dash, slash, period.
$fontRows = @{
    '0'=@('01110','10001','10011','10101','11001','10001','01110'); '1'=@('00100','01100','00100','00100','00100','00100','01110');
    '2'=@('01110','10001','00001','00010','00100','01000','11111'); '3'=@('11110','00001','00001','01110','00001','00001','11110');
    '4'=@('00010','00110','01010','10010','11111','00010','00010'); '5'=@('11111','10000','10000','11110','00001','00001','11110');
    '6'=@('01110','10000','10000','11110','10001','10001','01110'); '7'=@('11111','00001','00010','00100','01000','01000','01000');
    '8'=@('01110','10001','10001','01110','10001','10001','01110'); '9'=@('01110','10001','10001','01111','00001','00001','01110');
    'A'=@('01110','10001','10001','11111','10001','10001','10001'); 'B'=@('11110','10001','10001','11110','10001','10001','11110');
    'C'=@('01111','10000','10000','10000','10000','10000','01111'); 'D'=@('11110','10001','10001','10001','10001','10001','11110');
    'E'=@('11111','10000','10000','11110','10000','10000','11111'); 'F'=@('11111','10000','10000','11110','10000','10000','10000');
    'G'=@('01111','10000','10000','10111','10001','10001','01111'); 'H'=@('10001','10001','10001','11111','10001','10001','10001');
    'I'=@('11111','00100','00100','00100','00100','00100','11111'); 'J'=@('00111','00010','00010','00010','10010','10010','01100');
    'K'=@('10001','10010','10100','11000','10100','10010','10001'); 'L'=@('10000','10000','10000','10000','10000','10000','11111');
    'M'=@('10001','11011','10101','10101','10001','10001','10001'); 'N'=@('10001','11001','10101','10011','10001','10001','10001');
    'O'=@('01110','10001','10001','10001','10001','10001','01110'); 'P'=@('11110','10001','10001','11110','10000','10000','10000');
    'Q'=@('01110','10001','10001','10001','10101','10010','01101'); 'R'=@('11110','10001','10001','11110','10100','10010','10001');
    'S'=@('01111','10000','10000','01110','00001','00001','11110'); 'T'=@('11111','00100','00100','00100','00100','00100','00100');
    'U'=@('10001','10001','10001','10001','10001','10001','01110'); 'V'=@('10001','10001','10001','10001','10001','01010','00100');
    'W'=@('10001','10001','10001','10101','10101','10101','01010'); 'X'=@('10001','10001','01010','00100','01010','10001','10001');
    'Y'=@('10001','10001','01010','00100','00100','00100','00100'); 'Z'=@('11111','00001','00010','00100','01000','10000','11111');
    ':'=@('00000','00100','00100','00000','00100','00100','00000'); '-'=@('00000','00000','00000','11111','00000','00000','00000');
    '/'=@('00001','00010','00010','00100','01000','01000','10000'); '.'=@('00000','00000','00000','00000','00000','00100','00100')
}
$glyphs = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ:-/."
$font = New-TransparentBitmap 96 24
$g = [System.Drawing.Graphics]::FromImage($font)
$white = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
for ($index = 0; $index -lt $glyphs.Length; $index++) {
    $glyph = [string]$glyphs[$index]
    $cellX = ($index % 16) * 6
    $cellY = [Math]::Floor($index / 16) * 8
    for ($y = 0; $y -lt 7; $y++) {
        for ($x = 0; $x -lt 5; $x++) {
            if ($fontRows[$glyph][$y][$x] -eq '1') { $g.FillRectangle($white, $cellX + $x, $cellY + $y, 1, 1) }
        }
    }
}
$white.Dispose(); $g.Dispose()
Save-Png $font "font.png"

function Write-Tone([string]$name, [double]$frequency, [double]$duration, [double]$decay, [bool]$noise = $false) {
    $sampleRate = 22050
    $sampleCount = [int]($sampleRate * $duration)
    $path = Join-Path $rawDir $name
    $stream = [System.IO.File]::Create($path)
    $writer = New-Object System.IO.BinaryWriter($stream)
    $dataSize = $sampleCount * 2
    $writer.Write([Text.Encoding]::ASCII.GetBytes('RIFF')); $writer.Write(36 + $dataSize)
    $writer.Write([Text.Encoding]::ASCII.GetBytes('WAVEfmt ')); $writer.Write(16); $writer.Write([int16]1); $writer.Write([int16]1)
    $writer.Write($sampleRate); $writer.Write($sampleRate * 2); $writer.Write([int16]2); $writer.Write([int16]16)
    $writer.Write([Text.Encoding]::ASCII.GetBytes('data')); $writer.Write($dataSize)
    $random = [System.Random]::new(73)
    for ($i = 0; $i -lt $sampleCount; $i++) {
        $t = $i / $sampleRate
        $envelope = [Math]::Pow([Math]::Max(0, 1 - $t / $duration), $decay)
        $wave = if ($noise) { $random.NextDouble() * 2 - 1 } else { [Math]::Sin(2 * [Math]::PI * $frequency * $t) }
        $writer.Write([int16]($wave * $envelope * 15000))
    }
    $writer.Dispose(); $stream.Dispose()
}

Write-Tone "collect.wav" 880 0.12 1.4
Write-Tone "hit.wav" 110 0.20 1.0 $true
Write-Tone "explode.wav" 70 0.35 1.8 $true
Write-Tone "victory.wav" 660 0.45 0.5
Write-Tone "shoot.wav" 720 0.055 1.8
Write-Tone "boss_warning.wav" 180 0.55 0.35
Write-Tone "boss_defeat.wav" 95 0.75 1.4 $true
Write-Tone "upgrade.wav" 1120 0.24 0.55

Write-Host "Generated Canyon Breakout assets in $assetDir and $rawDir"
