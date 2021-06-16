//===-- AnalyzerFlagBase.td - Static Analyzer Flags TableGen base classes -===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/// Describes ain option type. This is important for validating user supplied
/// inputs.
/// New option types can be added by modifying this enum. Note that this
/// requires changes in the TableGen emitter files found in
/// clang/utils/TableGen.
class CmdLineOptionTypeEnum<bits<2> val> {
  bits<2> Type = val;
}
def Integer : CmdLineOptionTypeEnum<0>;
def String : CmdLineOptionTypeEnum<1>;
def Boolean : CmdLineOptionTypeEnum<2>;

/// A description. May be displayed to the user when clang is invoked with
/// a '-help'-like command line option.
class HelpText<string text> { string HelpText = text; }
