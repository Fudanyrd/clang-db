/**
 * Thorough test of C++ template definition, specialization, and instantiation.
 */

#include "test.h"

namespace clang {

/** Non-type function template. */
TEST_F(TestHelper, FnNonTypeParm) {
  PrepareParsingCXX("template <int N, char ...Args> void fn();");

  RunAction;

  const char *GlobalRecord[3] = {
      "",
      "2fn"
      "IN8template8decltype1iEN8template8decltype2czEE"
      "vv", /* void fn<template::decltype::i, template::decltype::cz>() */
      "v"};

  auto &Actual = GetNamespaces(DB);
  CompareTuple(Actual[0], GlobalRecord);
}

/** Non-type class template. */
TEST_F(TestHelper, ClsNonTypeParm) {
  PrepareParsingCXX("template <int N, short ...Args> struct foo { };");

  RunAction;

  const char *GlobalRecord[3] = {
      "",
      "3fooIN8template8decltype1iEN8template8decltype2szEE"
      "", /* foo<template::decltype::i, template::decltype::sz> */
      "6struct"};

  auto &Actual = GetNamespaces(DB);
  CompareTuple(Actual[0], GlobalRecord);
}

} // namespace clang
