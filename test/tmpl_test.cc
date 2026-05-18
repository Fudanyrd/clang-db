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

TEST_F(TestHelper, DependentSizedArray) {
  PrepareParsingCXX("template <typename T, int N> struct array { T buf[N]; };");

  const char *Member[3] = {
      /* array<template::typename::T, template::decltype::i> */
      "5arrayIN8template8typename1TEN8template8decltype1iEE",
      "3buf",
      /* public: T buf[]; */
      "6public24A_N8template8typename1TE",
  };

  RunAction;
  auto &Actual = GetClasses(DB);
  CompareTuple(Actual[0], Member);
}

TEST_F(TestHelper, TemplateBaseClass) {
  PrepareParsingCXX(
      "template <typename T, typename U>\n"
      "struct A {  struct Base { virtual U val() = 0; }; };\n"
      "struct B : public A<int,int>::Base { int val() override; }; \n");

  const char *GobalRecord[3] = {
      "",
      "1B",
      "6struct"
      "7virtual"
      "6public13N1AIiiE4BaseE" /* public: A<int, int>::Base */,
  };

  RunAction;
  auto &Actual = GetNamespaces(DB);
  ASSERT_EQ(Actual.size(), 2U);
  std::sort(Actual.begin(), Actual.end());
  CompareTuple(Actual[1], GobalRecord);
}

} // namespace clang
