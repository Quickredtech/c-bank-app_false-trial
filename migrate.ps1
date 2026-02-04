param(
    [string]$DatabaseUrl = $env:NEON_DATABASE_URL
)

if (-not $DatabaseUrl) {
    Write-Host "NEON_DATABASE_URL is not set."
    Write-Host "Usage: $env:NEON_DATABASE_URL='postgresql://...'; .\\migrate.ps1"
    exit 1
}

if (-not (Get-Command psql -ErrorAction SilentlyContinue)) {
    Write-Host "psql is not available in PATH. Install PostgreSQL client tools."
    exit 1
}

psql "$DatabaseUrl" -f migrations.sql
