#!/usr/bin/env bash

set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$repo_dir"

if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "Error: $repo_dir is not a git repository." >&2
  exit 1
fi

branch="$(git symbolic-ref --quiet --short HEAD || true)"
if [[ -z "$branch" ]]; then
  branch="main"
fi

remote="origin"
message="${1:-}"

if ! git remote get-url "$remote" >/dev/null 2>&1; then
  echo "Error: remote '$remote' does not exist." >&2
  exit 1
fi

if [[ -z "$message" ]]; then
  message="Vault sync $(date '+%Y-%m-%d %H:%M:%S')"
fi

echo "Syncing branch '$branch' with remote '$remote'..."

git add -A

if ! git diff --cached --quiet; then
  git commit -m "$message"
else
  echo "No local changes to commit."
fi

git pull --rebase --autostash "$remote" "$branch"
git push "$remote" "$branch"

echo "Done."
