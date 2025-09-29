## GitHub Workflows Overview

Brief purpose & triggers for each workflow in this directory.

### build.yml – Build Firmware
Purpose: Builds all firmware targets using the provided ARM toolchain container.
Triggers: Pushes to `main` and `develop`; all pull requests. Runs `make all` after installing Python deps from `common/requirements.txt`.
Output: Build logs (artifacts not explicitly uploaded here).

### lint.yml – Static Analysis (Cppcheck/MISRA)
Purpose: Runs `cppcheck` with MISRA addon against each component (`common`, `BMU`, `PDU`, `VCU`, `WSB`).
Triggers: Every pull request (any branch).
Notes: Uses a container image with toolchain + cppcheck. Matrix strategy iterates components.

### devcontainer-prebuild.yml – Produce Base Devcontainer Image
Purpose: Builds and pushes a multi-arch (amd64 + arm64) devcontainer image (`ghcr.io/<owner>/uwfe-dev`). Tags it with the commit SHA and `latest`; maintains build cache.
Triggers: Pushes to `2025_prebuilt_container` and `2025_test_prebuilt`, weekly cron (Mon 07:00 UTC), manual dispatch.
Key Steps: QEMU + Buildx setup, Docker build from `.devcontainer/Dockerfile`, cache-from latest, cache-to registry.

### devcontainer-exp.yml – Experimental Devcontainer Images
Purpose: Builds experimental/dev iteration images for feature or experimental branches under `exp_cont/**` or related PRs to `main`.
Triggers: Push to branches matching `exp_cont/**`; PRs targeting `main` that touch devcontainer-related paths; manual dispatch.
Tag Scheme: `exp-<branch>` and `exp-<branch>-<shortsha>`.
Notes: Uses `devcontainers/ci` action for multi-arch image build & push.

### devcontainer-promote.yml – Promote Prebuilt to Stable
Purpose: Re-tags an already-built devcontainer image (by SHA or existing tag) as `stable` and optionally as a semantic version (e.g., `v1.2.3`) without rebuilding.
Triggers: Manual dispatch only (requires specifying `source_tag`; optional `version_tag`).
Mechanism: Uses `docker buildx imagetools create` to add new manifest tags pointing to the existing image digest.

## Typical Flow
1. Developer updates devcontainer or dependencies → push to a prebuild branch → `devcontainer-prebuild.yml` produces `latest` + SHA tags.
2. For experimental features, push under `exp_cont/` → `devcontainer-exp.yml` creates ephemeral `exp-*` tags.
3. Once validated, manually run `devcontainer-promote.yml` to assign `stable` (and optionally a version tag) to the chosen SHA.
4. Firmware code changes on regular branches/PRs trigger `build.yml` and `lint.yml` for validation.

## Tag Meanings
- latest: Most recent successful prebuild from designated branches.
- <sha>: Immutable reference to the exact image build for traceability.
- exp-*: Experimental/feature branch images (not guaranteed stable).
- stable: Current blessed / recommended image for general development.
- vX.Y.Z: Optional semantic release tag for long-term reproducibility.

## Maintenance Tips
- Retire unused `exp-*` tags periodically to reduce registry clutter.
- Always promote by digest (source_tag = SHA) when possible to avoid ambiguity.
- If a devcontainer change is urgent, you can manual-dispatch the prebuild workflow outside normal branches (temporarily) or cherry-pick into a prebuild branch.

