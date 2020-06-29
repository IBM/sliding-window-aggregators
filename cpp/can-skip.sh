#!/bin/bash

# Determines if we can skip build and test based on the provided paths we care
# about. If the provided paths DO NOT appear in the latest commits, then we can
# skip build and test. We exit with 0 to indicate we can skip tests and 1 if 
# we need to run tests. Based on:
#     https://fant.io/p/circleci-early-exit/
set -e

PATHS_TO_SEARCH="$*"

# an empty path trivially means we can skip build and tests
if [ -z "$PATHS_TO_SEARCH" ]; then
    exit 0
fi

LATEST_COMMIT=$(git rev-parse HEAD)
LATEST_COMMIT_IN_PATH=$(git log -1 --format=format:%H --full-diff $PATHS_TO_SEARCH)

# if the given path is not in the latest commit, then we can skip build and tests
if [ $LATEST_COMMIT != $LATEST_COMMIT_IN_PATH ]; then
    exit 0
fi

# we need to run the tests
exit 1
