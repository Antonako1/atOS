#!/bin/bash

# Usage:
#   ./SCRIPTS/SYNC_UPSTREAM_DEVELOPMENT.sh           # fetch, fast-forward local development, push to origin
#   ./SCRIPTS/SYNC_UPSTREAM_DEVELOPMENT.sh --no-push  # same, but skip push

set -euo pipefail

UPSTREAM_REMOTE="upstream"
DEFAULT_UPSTREAM_URL="https://github.com/Antonako1/atOS.git"
PUSH=1

if [[ "${1-}" == "--no-push" ]]; then
  PUSH=0
fi

# Ensure we're inside a git repo
if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "Error: not inside a git repository. Run from the repo root." >&2
  exit 1
fi

echo "[sync] Ensuring upstream remote exists..."
if git remote get-url "$UPSTREAM_REMOTE" >/dev/null 2>&1; then
  UPSTREAM_URL=$(git remote get-url "$UPSTREAM_REMOTE")
  echo "[sync] Using existing upstream: $UPSTREAM_URL"
else
  echo "[sync] Adding upstream -> $DEFAULT_UPSTREAM_URL"
  git remote add "$UPSTREAM_REMOTE" "$DEFAULT_UPSTREAM_URL"
fi

echo "[sync] Fetching from upstream..."
git fetch "$UPSTREAM_REMOTE" --prune

# Verify upstream/development exists
if ! git show-ref --verify --quiet "refs/remotes/$UPSTREAM_REMOTE/development"; then
  echo "Error: $UPSTREAM_REMOTE/development not found. Did the original repo rename or remove it?" >&2
  exit 1
fi

# Create local development if missing
if ! git show-ref --verify --quiet refs/heads/development; then
  echo "[sync] Creating local branch 'development' tracking $UPSTREAM_REMOTE/development"
  git branch --track development "$UPSTREAM_REMOTE/development"
fi

echo "[sync] Switching to local 'development'"
git switch development

echo "[sync] Fast-forwarding to $UPSTREAM_REMOTE/development"
if ! git merge --ff-only "$UPSTREAM_REMOTE/development"; then
  echo "\nError: local 'development' has diverged. Resolve manually (rebase/merge), then rerun." >&2
  exit 1
fi

if [[ "$PUSH" -eq 1 ]]; then
  echo "[sync] Pushing 'development' to origin"
  git push origin development
else
  echo "[sync] Skipping push (--no-push)"
fi

echo "[sync] Done. 'development' is up to date with $UPSTREAM_REMOTE/development."

