# Load the production functions while the script's direct-execution guard prevents an actual publication attempt.
$scriptPath = Join-Path $PSScriptRoot "../../.github/scripts/Publish-GitHubRelease.ps1"
. $scriptPath

Describe "Publish-GitHubRelease" {
    BeforeEach {
        $env:GH_TOKEN = "test-token"
        $env:GITHUB_STEP_SUMMARY = ""
    }

    Context "GitHub CLI failures" {
        It "returns null only for an allowed HTTP 404" {
            Mock Invoke-GhNative {
                [PSCustomObject]@{ ExitCode = 1; Output = "gh: Not Found (HTTP 404)" }
            }

            Invoke-Gh -Arguments @("api", "missing") -AllowNotFound | Should BeNullOrEmpty
        }

        It "throws for non-404 API failures" {
            Mock Invoke-GhNative {
                [PSCustomObject]@{ ExitCode = 1; Output = "gh: API rate limit exceeded (HTTP 403)" }
            }

            { Invoke-Gh -Arguments @("api", "forbidden") -AllowNotFound } | Should Throw "HTTP 403"
        }
    }

    Context "tag validation" {
        It "resolves annotated tags to their commit" {
            Mock Invoke-Gh {
                param($Arguments)
                # The first lookup returns an annotated tag object; the second returns its peeled commit.
                if ($Arguments[1] -like "*/git/ref/tags/*") {
                    return '{"object":{"type":"tag","sha":"tag-object"}}'
                }
                return '{"object":{"type":"commit","sha":"commit-sha"}}'
            }

            Get-TagCommitSha -Repository "owner/repository" -Tag "OIV-1.2.3" | Should Be "commit-sha"
            Assert-MockCalled Invoke-Gh 2
        }

        It "stops before mutation when the tag targets another commit" {
            Mock Get-TagCommitSha { "other-sha" }
            # These fail loudly if publication proceeds past the immutable-tag guard.
            Mock Get-GitHubRelease { throw "Release lookup must not run." }
            Mock Invoke-Gh { throw "GitHub mutation must not run." }

            {
                Invoke-ReleasePublication -Repository "owner/repository" -Tag "OIV-1.2.3" `
                    -TargetSha "expected-sha" -Title "OIV-1.2.3" -Prerelease $false `
                    -RuntimeArchive "OIV-runtime.7z" -SymbolsArchive "OIV-symbols.7z"
            } | Should Throw "already targets"
            Assert-MockCalled Invoke-Gh 0 -ParameterFilter { $Arguments[0] -eq "release" }
        }
    }

    Context "release lookup" {
        It "returns null when the release tag is absent" {
            Mock Invoke-Gh { $null }

            Get-GitHubRelease -Repository "owner/repository" -Tag "OIV-1.2.3" | Should BeNullOrEmpty
        }

        It "normalizes the REST release response" {
            Mock Invoke-Gh {
                '{"prerelease":true,"assets":[{"name":"runtime.7z"}]}'
            }

            $release = Get-GitHubRelease -Repository "owner/repository" -Tag "OIV-1.2.3"
            $release.isPrerelease | Should Be $true
            $release.assets.Count | Should Be 1
            $release.assets[0].name | Should Be "runtime.7z"
        }
    }

    Context "asset replacement" {
        BeforeEach {
            Mock Get-TagCommitSha { "expected-sha" }
            Mock Get-GitHubRelease {
                [PSCustomObject]@{
                    isPrerelease = $false
                    assets = @(
                        [PSCustomObject]@{ name = "OIV-runtime.7z" },
                        [PSCustomObject]@{ name = "OIV-symbols.7z" },
                        [PSCustomObject]@{ name = "obsolete.7z" }
                    )
                }
            }
        }

        It "does not edit or delete assets when upload fails" {
            Mock Invoke-Gh {
                param($Arguments)
                if ($Arguments[1] -eq "upload") {
                    throw "upload failed"
                }
            }

            {
                Invoke-ReleasePublication -Repository "owner/repository" -Tag "OIV-1.2.3" `
                    -TargetSha "expected-sha" -Title "OIV-1.2.3" -Prerelease $false `
                    -RuntimeArchive "OIV-runtime.7z" -SymbolsArchive "OIV-symbols.7z"
            } | Should Throw "upload failed"
            # Upload failure must leave the existing release metadata and asset set untouched.
            Assert-MockCalled Invoke-Gh 0 -ParameterFilter { $Arguments[1] -eq "edit" }
            Assert-MockCalled Invoke-Gh 0 -ParameterFilter { $Arguments[1] -eq "delete-asset" }
        }

        It "removes only obsolete assets after successful upload" {
            Mock Invoke-Gh {}

            Invoke-ReleasePublication -Repository "owner/repository" -Tag "OIV-1.2.3" `
                -TargetSha "expected-sha" -Title "OIV-1.2.3" -Prerelease $false `
                -RuntimeArchive "OIV-runtime.7z" -SymbolsArchive "OIV-symbols.7z"

            Assert-MockCalled Invoke-Gh 1 -ParameterFilter { $Arguments[1] -eq "upload" }
            Assert-MockCalled Invoke-Gh 1 -ParameterFilter {
                $Arguments[1] -eq "delete-asset" -and $Arguments[3] -eq "obsolete.7z"
            }
            # Assets replaced through --clobber are expected and must not be removed by stale-asset cleanup.
            Assert-MockCalled Invoke-Gh 0 -ParameterFilter {
                $Arguments[1] -eq "delete-asset" -and $Arguments[3] -in @("OIV-runtime.7z", "OIV-symbols.7z")
            }
        }
    }
}
