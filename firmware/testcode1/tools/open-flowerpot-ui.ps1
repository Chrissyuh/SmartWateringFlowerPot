param(
  [string]$Port = "",
  [int]$Baud = 115200,
  [switch]$NoBrowser
)

$ErrorActionPreference = "Stop"

function Get-FlowerPotComPorts {
  $devices = Get-PnpDevice -PresentOnly |
    Where-Object {
      $_.InstanceId -match "VID_303A&PID_1001" -or
      $_.FriendlyName -match "ESP|Espressif|USB Serial Device"
    }

  foreach ($device in $devices) {
    if ($device.FriendlyName -match "\((COM\d+)\)") {
      $matches[1]
    }
  }
}

if ([string]::IsNullOrWhiteSpace($Port)) {
  $Port = Get-FlowerPotComPorts | Select-Object -First 1
}

if ([string]::IsNullOrWhiteSpace($Port)) {
  throw "No Flower Pot ESP32 COM port found. Plug in the board with a USB data cable first."
}

$serial = [System.IO.Ports.SerialPort]::new($Port, $Baud, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$serial.ReadTimeout = 500
$serial.WriteTimeout = 500
$serial.DtrEnable = $false
$serial.RtsEnable = $false

$lines = New-Object System.Collections.Generic.List[string]

try {
  $serial.Open()
  Start-Sleep -Milliseconds 900
  $serial.WriteLine("ui")

  $deadline = (Get-Date).AddSeconds(8)
  while ((Get-Date) -lt $deadline) {
    try {
      $line = $serial.ReadLine().Trim()
      if ($line.Length -gt 0) {
        $lines.Add($line)
      }
      if ($line -eq "FLOWERPOT_UI_END") {
        break
      }
    } catch [System.TimeoutException] {
      # Keep waiting until the deadline.
    }
  }
} finally {
  if ($serial.IsOpen) {
    $serial.Close()
  }
}

$values = @{}
foreach ($line in $lines) {
  $idx = $line.IndexOf("=")
  if ($idx -gt 0) {
    $key = $line.Substring(0, $idx)
    $value = $line.Substring($idx + 1)
    $values[$key] = $value
  }
}

$url = $null
if ($values["STA_STATUS"] -eq "connected" -and $values["STA_URL"] -match "^http://") {
  $url = $values["STA_URL"]
} elseif ($values["UI_URL"] -match "^http://") {
  $url = $values["UI_URL"]
} elseif ($values["AP_URL"] -match "^http://") {
  $url = $values["AP_URL"]
}

if (-not $url) {
  Write-Host ($lines -join [Environment]::NewLine)
  throw "The board answered on $Port, but did not provide a UI URL."
}

Write-Host "Flower Pot UI: $url"
if (-not $NoBrowser) {
  Start-Process $url
}
