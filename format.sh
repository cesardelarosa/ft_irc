#!/bin/bash

echo "Formatting C++ files..."
clang-format -i include/*.hpp src/*.cpp
echo "Formatting complete!"
