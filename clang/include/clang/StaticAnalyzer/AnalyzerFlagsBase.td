
/// Describes a checker or package option type. This is important for validating
/// user supplied inputs.
/// New option types can be added by modifying this enum. Note that this
/// requires changes in the TableGen emitter file ClangSACheckersEmitter.cpp.
class CmdLineOptionTypeEnum<bits<2> val> {
  bits<2> Type = val;
}
def Integer : CmdLineOptionTypeEnum<0>;
def String : CmdLineOptionTypeEnum<1>;
def Boolean : CmdLineOptionTypeEnum<2>;

/// Describes the state of the entry. We wouldn't like to display, for example,
/// developer only entries for a list meant for end users.
class DevelopmentStageEnum<bits<1> val> {
  bits<1> Val = val;
}

/// Alpha entries are under development, might be incomplet, inkorrekt and
/// unstable.
def InAlpha : DevelopmentStageEnum<0>;

/// Released entries are stable, produce minimal, if any false positives,
/// and emits reports that explain the occurance of the bug understandably and
/// thoroughly.
def Released : DevelopmentStageEnum<1>;

/// Marks the entry hidden. Hidden entries won't be displayed in
/// -analyzer-checker-option-help.
class HiddenEnum<bit val> {
  bit Val = val;
}
def DontHide : HiddenEnum<0>;
def Hide : HiddenEnum<1>;


