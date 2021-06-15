//class ConfigType<bits<2> val> {
//  bits<2> Type = val;
//}
//def Integer : ConfigType<0>;
//def String : ConfigType<1>;
//def Boolean : ConfigType<2>;
//
///// Describes the state of the entry. We wouldn't like to display, for example,
///// developer only entries for a list meant for end users.
//class DevelopmentStageEnum<bits<1> val> {
//  bits<1> Val = val;
//}
//
///// Alpha entries are under development, might be incomplet, inkorrekt and
///// unstable.
//def InAlpha : DevelopmentStageEnum<0>;
//
///// Released entries are stable, produce minimal, if any false positives,
///// and emits reports that explain the occurance of the bug understandably and
///// thoroughly.
//def Released : DevelopmentStageEnum<1>;
//
///// Marks the entry hidden. Hidden entries won't be displayed in
///// -analyzer-checker-option-help.
//class HiddenEnum<bit val> {
//  bit Val = val;
//}
//def DontHide : HiddenEnum<0>;
//def Hide : HiddenEnum<1>;

class Config {
  string Name;
}

def ShouldIncludeImplicitDtorsInCFG : Config {
  let Name = "cfg-implicit-dtors";
}
