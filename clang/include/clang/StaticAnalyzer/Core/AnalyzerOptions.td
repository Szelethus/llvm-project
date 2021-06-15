//===--- CheckerBase.td - Checker TableGen classes ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file defines the TableGen core definitions for checkers
//
//===----------------------------------------------------------------------===//

include "../AnalyzerFlagsBase.td"

// Make these individual types.
class IntegerType : CmdLineOptionTypeEnum<0> {}
class StringType : CmdLineOptionTypeEnum<1> {}
class BooleanType : CmdLineOptionTypeEnum<2> {}

class Config<string name> {
  string Name = name;
  bits<2> Type = ?;
  string  HelpText;
  string  DefaultVal;
  bits<1> DevelopmentStage;
  bit     Hidden;
}

def ShouldIncludeImplicitDtorsInCFG : Config<"cfg-implicit-dtors">,
  BooleanType, HelpText<"yo">, DefaultVal<"
