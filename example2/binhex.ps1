# Convert a hex dump to a binary file, via a PowerShell script.

$inFile = ".\w32load.txt"
$outFile = ".\w32load.exe"

foreach($line in Get-Content $inFile) {
  $mybytes = $line -split " "
  $bchunk = ""
  foreach($mybyte in $mybytes) {
    $ec = mybyte[-1]
    if ($ec -eq '-' -or $ec -eq ':') {
      Continue
    }
    $bval = [System.Convert]::ToInt16($mybyte, 16)
    $cval = [char]$bval
    $bchunk += $cval
  }
  Add-Content -Path $outFile -value $bchunk -NoNewline
}
