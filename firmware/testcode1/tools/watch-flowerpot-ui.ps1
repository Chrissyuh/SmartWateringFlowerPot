param(
  [int]$PollSeconds = 2
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$opener = Join-Path $scriptDir "open-flowerpot-ui.ps1"
$openedPorts = @{}

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

Write-Host "Watching for Flower Pot ESP32 USB serial ports. Press Ctrl+C to stop."

while ($true) {
  $ports = @(Get-FlowerPotComPorts)
  $present = @{}

  foreach ($port in $ports) {
    $present[$port] = $true
    if (-not $openedPorts.ContainsKey($port)) {
      Write-Host "Detected $port; opening Flower Pot UI..."
      try {
        & $opener -Port $port
        $openedPorts[$port] = $true
      } catch {
        Write-Warning $_.Exception.Message
      }
    }
  }

  foreach ($known in @($openedPorts.Keys)) {
    if (-not $present.ContainsKey($known)) {
      $openedPorts.Remove($known)
    }
  }

  Start-Sleep -Seconds $PollSeconds
}
