//===--- AnalyzerOptions.td - Analyzer configs ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file defines the analyzer options avaible with -analyzer-config.
//  TODO: Copy documentation from AnalyzerOptions.def before deleting it.
//
//===----------------------------------------------------------------------===//

include "../AnalyzerFlagsBase.td"


class Config<string name, CmdLineOptionTypeEnum type> {
  string Name = name;
  bits<2> Type = type.Type;
  string  HelpText;
  string  DefaultVal;
}

class DefaultVal<string val> { string DefaultVal = val; }
//===----------------------------------------------------------------------===//

class BooleanConfig<string name> : Config<name, Boolean>;
//===----------------------------------------------------------------------===//

class IntegerConfig<string name> : Config<name, Integer> {
  list<int> Values = ?;
}

class IntValues<list<int> Vals> {
  list<int> Values = Vals;
}
//===----------------------------------------------------------------------===//

class StringConfig<string name> : Config<name, String> {
  bit NeedsNoValidation = 0;
}
//===----------------------------------------------------------------------===//

class FileNameConfig<string name> : Config<name, String>;
//===----------------------------------------------------------------------===//

class EnumVal<string cmdFlag, string enumName> {
  string CmdFlag = cmdFlag;
  string EnumName = enumName;
}

class EnumConfig<string name> : Config<name, String> {
  list<EnumVal> Values;
  string EnumPrefix;
}

class EnumValues<list<EnumVal> Vals> {
  list<EnumVal> Values = Vals;
}

class EnumPrefix<string enumPrefix> {
  string EnumPrefix = enumPrefix;
}
//===----------------------------------------------------------------------===//


def ShouldIncludeImplicitDtorsInCFG : BooleanConfig<"cfg-implicit-dtors">,
  HelpText<"Whether or not implicit destructors for C++ objects should be "
           "included in the CFG.">,
  DefaultVal<"true">;

def AlwaysInlineSize : IntegerConfig<"ipa-always-inline-size">,
  HelpText<"The size of the functions (in basic blocks), which should be "
           "considered to be small enough to always inline.">,
  DefaultVal<"3">;

def RawSilencedCheckersAndPackages : StringConfig<"silence-checkers">,
  HelpText<"A semicolon separated list of checker and package names to "
           "silence. Silenced checkers will not emit reports, but the modeling "
           "remain enabled.">,
  DefaultVal<"">;

def CXXMemberInliningMode : EnumConfig<"c++-inlining">,
  HelpText<"Controls which C++ member functions will be considered for "
           "inlining.">,
  EnumPrefix<"CIMK">,
  EnumValues<[
    EnumVal<"none", "None">,
    EnumVal<"constructors", "Constructors">,
    EnumVal<"destructors", "Destructors">,
    EnumVal<"methods", "Memberfunctions">
  ]>,
  DefaultVal<"destructors">;

