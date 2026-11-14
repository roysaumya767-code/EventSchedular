// heap.hpp -- Bridge header forwarding to structA::HeapEventSet
#pragma once
#include "../src/structA/heap.hpp"

// Forward HeapEventSet to global namespace for existing tests and code
using HeapEventSet = structA::HeapEventSet;
