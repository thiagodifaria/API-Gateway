# service-assembly

Assembly routines used by `service-api/service-cpp`.

This directory owns low-level acceleration code only: crypto primitives, HTTP scanning, validation, compression, memory operations and network encoding helpers. Business behavior must stay in C++.

Changes here should include focused tests or benchmark evidence.
