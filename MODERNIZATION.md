# SpecusGL – C++17 Modernization Plan

## Overview

This codebase is derived from Mesa 7.0.4, implementing a software OpenGL 2.0
renderer.  The goal is to migrate it to idiomatic C++17 while preserving its
complete API and behaviour.

An initial shift of the C code to C++ with minimal changes is now complete.
What we want to do next is start migrating the "C++" code to become more
idiomatic C++ rather than "technically C++" C code - i.e. take advantage
of C++ language features to make the code more readable, well organized,
less verbose, readily debuggable and performant.

Large, cross-cutting changes are expected and not something to shy away
from - add testing code as appropriate to verify you're not breaking things,
but we're looking to achieve a major refactoring of this codebase rather
than incrementally inching forward slowly. Given the nature of this code base,
there will need to be some high risk changes at some point if we're going to
meaningfully shift the code towards modern best practices. Mitigation will be
adding tests to exercise the functionality thoroughly and be able to spot
breakage, but we must accept some risk to make substantial change.

**Preserve the public C API.**  `OSMesa/osmesa.h`, `OSMesa/gl.h`, etc.
remain unchanged so that existing C consumers compile without modification.
Internal implementation code structure and organization should be updated to
reflect modern best practice C++ design.

