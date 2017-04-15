#!/bin/bash

clang-format -style=Google -i include/megabit/*.hpp
clang-format -style=Google -i src/*.cpp
