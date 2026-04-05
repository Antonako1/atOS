# AstraC Compiler — AC Syntax & Usage reference

Complete reference manual for the AstraC compiler and its input language, AC.

## Table of Contents

---

1. [Overview](#overview)
2. [Pipeline](#pipeline)
3. [Command-Line Usage](#command-line-usage)
4. [Source File Structure](#source-file-structure)
5. [Preprocessor](#preprocessor)
6. [Directives](#directives)
8. [Data Variables](#data-variables)
9. [Number Literals](#number-literals)
10. [String Literals](#string-literals)
11. [Expressions](#expressions)
12. [Functions](#functions)
13. [Control Flow](#control-flow)
14. [Registers](#registers)
15. [Macros](#macros)
16. [Info Queries](#info-queries)
17. [Error Codes](#error-codes)
18. [Syntax](#syntax)
19. [Error Reference](#error-reference)
20. [Examples](#examples)

## Overview

AstraC (AC) is a C-like language designed for low-level programming of the atOS operating system and its hardware. It provides a familiar syntax while allowing direct access to CPU registers, memory, and hardware features. The ASTRAC compiler translates AC source files into assembly code, which can then be assembled into binary object files for execution on atOS.