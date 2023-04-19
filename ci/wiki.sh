#!/bin/bash

set -euo pipefail

if [ -z "$GH_PERSONAL_ACCESS_TOKEN" ]; then
    exit 1
fi

GIT_REPOSITORY_URL="https://${GH_PERSONAL_ACCESS_TOKEN}@${GITHUB_SERVER_URL#https://}/$GITHUB_REPOSITORY.wiki.git"

tmp_dir=$(mktemp -d -t ci-XXXXXXXXXX)
(
    cd "$tmp_dir" || exit 1
    git init
    git config user.name "$GITHUB_ACTOR"
    git config user.email "$GITHUB_ACTOR@users.noreply.github.com"
    git pull "$GIT_REPOSITORY_URL"
) || exit 1

for file in docs/*.md; do
    cp "$file" "$tmp_dir"
done

(
    cd "$tmp_dir" || exit 1
    git add .
    git commit -m "Automatically publish wiki"
    git push --set-upstream "$GIT_REPOSITORY_URL" master
) || exit 1

rm -rf "$tmp_dir"
exit 0
