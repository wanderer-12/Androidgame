Add-Type -AssemblyName System.Drawing

function New-PointArray([object[]]$coordinates) {
    $points = [System.Drawing.Point[]]::new($coordinates.Count)
    for ($index = 0; $index -lt $coordinates.Count; $index++) {
        $points[$index] = [System.Drawing.Point]::new($coordinates[$index][0], $coordinates[$index][1])
    }
    return ,$points
}

function Add-Polygon($graphics, [string]$fill, [string]$stroke, [int]$strokeWidth, [object[]]$coordinates) {
    $points = New-PointArray $coordinates
    $brush = [System.Drawing.SolidBrush]::new([System.Drawing.ColorTranslator]::FromHtml($fill))
    $pen = [System.Drawing.Pen]::new([System.Drawing.ColorTranslator]::FromHtml($stroke), $strokeWidth)
    $pen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Miter
    $graphics.FillPolygon($brush, $points)
    $graphics.DrawPolygon($pen, $points)
    $brush.Dispose()
    $pen.Dispose()
}

function Add-Rect($graphics, [string]$fill, [int]$x, [int]$y, [int]$width, [int]$height) {
    $brush = [System.Drawing.SolidBrush]::new([System.Drawing.ColorTranslator]::FromHtml($fill))
    $graphics.FillRectangle($brush, $x, $y, $width, $height)
    $brush.Dispose()
}

function Save-Hunter([string]$path) {
    $bitmap = [System.Drawing.Bitmap]::new(128, 112, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.Clear([System.Drawing.Color]::Transparent)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None

    Add-Polygon $graphics '#20242E' '#111319' 5 @(@(64,3),@(77,15),@(84,34),@(111,25),@(125,41),@(115,64),@(91,72),@(81,98),@(64,109),@(47,98),@(37,72),@(13,64),@(3,41),@(17,25),@(44,34),@(51,15))
    Add-Polygon $graphics '#7B2CBF' '#3B1761' 4 @(@(64,10),@(73,22),@(77,43),@(105,34),@(116,43),@(108,56),@(83,60),@(76,88),@(64,99),@(52,88),@(45,60),@(20,56),@(12,43),@(23,34),@(51,43),@(55,22))
    Add-Polygon $graphics '#D8E1E8' '#55606D' 3 @(@(64,15),@(72,45),@(87,58),@(72,65),@(68,91),@(64,99),@(60,91),@(56,65),@(41,58),@(56,45))
    Add-Polygon $graphics '#FF9F1C' '#8C4300' 3 @(@(64,26),@(70,48),@(64,62),@(58,48))
    Add-Rect $graphics '#A7F432' 56 66 6 18
    Add-Rect $graphics '#A7F432' 66 66 6 18
    Add-Rect $graphics '#E9FFB8' 59 69 10 8
    Add-Rect $graphics '#FF4D8D' 20 42 16 7
    Add-Rect $graphics '#FF4D8D' 92 42 16 7
    Add-Rect $graphics '#F7B2D0' 25 44 6 3
    Add-Rect $graphics '#F7B2D0' 97 44 6 3

    $graphics.Dispose()
    $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
}

function Save-Prism([string]$path) {
    $bitmap = [System.Drawing.Bitmap]::new(112, 112, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.Clear([System.Drawing.Color]::Transparent)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None

    Add-Polygon $graphics '#172E38' '#0B171D' 5 @(@(56,3),@(78,15),@(96,33),@(108,56),@(96,79),@(78,97),@(56,109),@(34,97),@(16,79),@(4,56),@(16,33),@(34,15))
    Add-Polygon $graphics '#18A6A6' '#0B5C66' 4 @(@(56,12),@(74,23),@(89,40),@(98,56),@(89,72),@(74,89),@(56,100),@(38,89),@(23,72),@(14,56),@(23,40),@(38,23))
    Add-Polygon $graphics '#F4C95D' '#8B641C' 4 @(@(56,20),@(68,36),@(88,42),@(78,58),@(84,80),@(62,74),@(56,94),@(50,74),@(28,80),@(34,58),@(24,42),@(44,36))
    Add-Polygon $graphics '#D7F7FF' '#397F99' 3 @(@(56,29),@(72,47),@(65,72),@(56,84),@(47,72),@(40,47))
    Add-Polygon $graphics '#E83F8C' '#7A174B' 3 @(@(56,39),@(68,53),@(62,69),@(56,76),@(50,69),@(44,53))
    Add-Rect $graphics '#FFFFFF' 52 48 8 15
    Add-Rect $graphics '#7CF5FF' 26 51 9 9
    Add-Rect $graphics '#7CF5FF' 77 51 9 9
    Add-Rect $graphics '#FFF3AF' 52 18 8 8
    Add-Rect $graphics '#FFF3AF' 52 86 8 8

    $graphics.Dispose()
    $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
}

$assetDirectory = Join-Path $PSScriptRoot '..\app\src\main\assets'
Save-Hunter (Join-Path $assetDirectory 'boss_hunter.png')
Save-Prism (Join-Path $assetDirectory 'boss_prism.png')
