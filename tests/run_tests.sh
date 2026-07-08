#!/bin/bash
set -e

# Change directory to project root if script is run from elsewhere
cd "$(dirname "$0")/.."

echo "Compiling Tests..."
g++ -std=c++17 -Imock_arduino -I. tests/test_runner.cpp magnetic_variation.cpp -x c++ main.ino -o tests/run_tests

echo "Running Tests..."
./tests/run_tests
