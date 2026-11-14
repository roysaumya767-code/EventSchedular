// avl.hpp -- Bridge header forwarding to structB::AvlEventSet
#pragma once
#include "../src/structB/avl.hpp"

// Forward AvlEventSet to global namespace for existing tests and code
using AvlEventSet = structB::AvlEventSet;
