<#
.SYNOPSIS
    Checks the language files in resources/strings against the en-US reference.

.DESCRIPTION
    For every expected language file the script verifies:
      - the file exists, is valid UTF-8 and has no byte order mark
      - the first line is '#pragma code_page(65001)'
      - the LANGUAGE statement matches the file name
      - there is exactly one STRINGTABLE block and every entry can be parsed
      - the set of string IDs equals the reference (no missing, extra or duplicate IDs)
      - no string is empty
      - placeholders (%s, %d, %1, {0}, ...) match the reference

    Exit code 0 on success, 1 on any error. Runs on Windows PowerShell 5.1 and PowerShell 7.
    This file must stay ASCII-only so that Windows PowerShell 5.1 reads it correctly.

.PARAMETER StringsDir
    Directory containing <tag>.rc files. Defaults to resources/strings next to this script.
#>
[CmdletBinding()]
param(
    [string]$StringsDir = ''
)

Set-StrictMode -Version 3
$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrEmpty($StringsDir)) {
    # $PSScriptRoot is not available in param() defaults on Windows PowerShell 5.1.
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    $StringsDir = Join-Path (Join-Path $scriptDir '..') 'resources/strings'
}

$Reference = 'en-US'
$Languages = [ordered]@{
    'en-US' = 'LANG_ENGLISH, SUBLANG_ENGLISH_US'
    'de-DE' = 'LANG_GERMAN, SUBLANG_GERMAN'
    'es-ES' = 'LANG_SPANISH, SUBLANG_SPANISH_MODERN'
    'fr-FR' = 'LANG_FRENCH, SUBLANG_FRENCH'
    'zh-CN' = 'LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED'
    'ja-JP' = 'LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN'
    'ko-KR' = 'LANG_KOREAN, SUBLANG_KOREAN'
}

$script:ErrorCount = 0

function Write-CheckError([string]$File, [string]$Message) {
    $script:ErrorCount++
    # Write-Host instead of Write-Output: messages must not become part of function return values.
    if ($env:GITHUB_ACTIONS -eq 'true') {
        Write-Host "::error file=${File}::$Message"
    } else {
        Write-Host "ERROR ${File}: $Message"
    }
}

function Get-Placeholders([string]$Text) {
    $withoutEscapes = $Text.Replace('%%', '')
    $pattern = '%[0-9]+(![^!]*!)?|%[-+ #0]*[0-9]*(\.[0-9]+)?(hh|h|ll|l|L|w|I64|I32|I)?[sSdiuxXoceEfgGp]|\{[0-9]+\}'
    $found = @([regex]::Matches($withoutEscapes, $pattern) | ForEach-Object { $_.Value })
    return @($found | Sort-Object)
}

function Read-StringFile([string]$Path, [string]$Tag) {
    $result = [ordered]@{}
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        Write-CheckError $Path 'file starts with a UTF-8 byte order mark; save as UTF-8 without BOM'
        $bytes = $bytes[3..($bytes.Length - 1)]
    }
    try {
        $text = (New-Object System.Text.UTF8Encoding($false, $true)).GetString($bytes)
    } catch {
        Write-CheckError $Path 'file is not valid UTF-8'
        return $result
    }

    $lines = $text -split "`r?`n"
    if ($lines.Count -eq 0 -or $lines[0].Trim() -ne '#pragma code_page(65001)') {
        Write-CheckError $Path "first line must be '#pragma code_page(65001)'"
    }

    $languageLines = @($lines | Where-Object { $_ -match '^\s*LANGUAGE\s+' })
    if ($languageLines.Count -ne 1) {
        Write-CheckError $Path "expected exactly one LANGUAGE statement, found $($languageLines.Count)"
    } else {
        $actual = ($languageLines[0] -replace '^\s*LANGUAGE\s+', '' -replace '\s+', ' ').Trim()
        $expected = $Languages[$Tag]
        if ($actual -ne $expected) {
            Write-CheckError $Path "LANGUAGE is '$actual', expected '$expected'"
        }
    }

    $tableCount = @($lines | Where-Object { $_ -match '^\s*STRINGTABLE\b' }).Count
    if ($tableCount -ne 1) {
        Write-CheckError $Path "expected exactly one STRINGTABLE block, found $tableCount"
    }

    $state = 'outside'
    $lineNumber = 0
    foreach ($line in $lines) {
        $lineNumber++
        $trimmed = $line.Trim()
        switch ($state) {
            'outside' { if ($trimmed -match '^STRINGTABLE\b') { $state = 'header' } }
            'header' {
                if ($trimmed -eq 'BEGIN' -or $trimmed -eq '{') { $state = 'inside' }
                elseif ($trimmed -ne '') { Write-CheckError $Path "line ${lineNumber}: expected BEGIN after STRINGTABLE" }
            }
            'inside' {
                if ($trimmed -eq 'END' -or $trimmed -eq '}') { $state = 'done'; continue }
                if ($trimmed -eq '' -or $trimmed.StartsWith('//')) { continue }
                $m = [regex]::Match($trimmed, '^(IDS_[A-Z0-9_]+)\s*,?\s*"((?:[^"]|"")*)"$')
                if (-not $m.Success) {
                    Write-CheckError $Path "line ${lineNumber}: cannot parse string entry: $trimmed"
                    continue
                }
                $id = $m.Groups[1].Value
                $value = $m.Groups[2].Value.Replace('""', '"')
                if ($result.Contains($id)) {
                    Write-CheckError $Path "line ${lineNumber}: duplicate ID $id"
                    continue
                }
                $result[$id] = $value
            }
        }
    }
    if ($state -eq 'header' -or $state -eq 'inside') {
        Write-CheckError $Path 'STRINGTABLE block is not closed with END'
    }
    return $result
}

$StringsDir = (Resolve-Path $StringsDir).Path
$tables = [ordered]@{}
foreach ($tag in $Languages.Keys) {
    $path = Join-Path $StringsDir "$tag.rc"
    if (-not (Test-Path $path)) {
        Write-CheckError $path 'language file is missing'
        continue
    }
    $tables[$tag] = Read-StringFile $path $tag
}

$unknown = @(Get-ChildItem -Path $StringsDir -Filter '*.rc' | Where-Object { -not $Languages.Contains($_.BaseName) })
foreach ($file in $unknown) {
    Write-CheckError $file.FullName 'unexpected language file (not in the list of supported languages)'
}

if ($tables.Contains($Reference)) {
    # Note: PowerShell variable names are case-insensitive; do not reuse $Reference.
    $refTable = $tables[$Reference]
    if ($refTable.Count -eq 0) {
        Write-CheckError (Join-Path $StringsDir "$Reference.rc") 'reference file contains no strings'
    }
    foreach ($tag in $tables.Keys) {
        $path = Join-Path $StringsDir "$tag.rc"
        $table = $tables[$tag]
        foreach ($id in $refTable.Keys) {
            if (-not $table.Contains($id)) {
                Write-CheckError $path "missing $id"
                continue
            }
            if ([string]::IsNullOrWhiteSpace($table[$id])) {
                Write-CheckError $path "$id is empty"
                continue
            }
            $expected = (Get-Placeholders $refTable[$id]) -join ' '
            $actual = (Get-Placeholders $table[$id]) -join ' '
            if ($expected -ne $actual) {
                Write-CheckError $path "$id placeholders '$actual' differ from reference '$expected'"
            }
        }
        foreach ($id in $table.Keys) {
            if (-not $refTable.Contains($id)) {
                Write-CheckError $path "$id is not defined in $Reference.rc"
            }
        }
    }
}

if ($script:ErrorCount -gt 0) {
    Write-Host "Check-Strings: $($script:ErrorCount) error(s)."
    exit 1
}

$count = if ($tables.Contains($Reference)) { $tables[$Reference].Count } else { 0 }
Write-Host "Check-Strings: OK ($($tables.Count) languages, $count strings each)."
exit 0
