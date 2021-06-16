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


class Config<string name, CmdLineOptionTypeEnum type> {
  string Name = name;
  bits<2> Type = type.Type;
  string  HelpText;
  string  DefaultVal;
}

class IntegerConfig<string name> : Config<name, Integer>;
class BooleanConfig<string name> : Config<name, Boolean>;
class StringConfig<string name> : Config<name, String>;


def ShouldIncludeImplicitDtorsInCFG : BooleanConfig<"cfg-implicit-dtors">,
  HelpText<"yo">, DefaultVal<"true">;
