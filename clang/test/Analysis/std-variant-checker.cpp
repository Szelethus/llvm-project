// RUN: %clang %s -Xclang -verify --analyze \
// RUN:   -Xclang -analyzer-checker=core \
// RUN:   -Xclang -analyzer-checker=debug.ExprInspection \
// RUN:   -Xclang -analyzer-checker=core,alpha.core.StdVariant

// use // FIXME when a warning is expected, but is not present

#include <variant>

class Foo{};

//helper functions
void changeVariantType(std::variant<int, char> &v) {
  v = 25;
}

void changesToInt(std::variant<int, char> &v);
void changesToInt(std::variant<int, char> *v);

void cannotChnageRef(const std::variant<int, char> &v);
void cannotChnagePtr(const std::variant<int, char> *v);

char getUnknownChar();

void swap(std::variant<int, char> &v1, std::variant<int, char> &v2) {
  std::variant<int, char> tmp = v1;
  v1 = v2;
  v2 = tmp;
}

void cantDo(const std::variant<int, char>& v) {
  std::variant<int, char> vtmp = v;
  vtmp = 5;
  int a = std::get<int> (vtmp);
  (void*) a;
}

void changeVariantPtr(std::variant<int, char> *v) {
  *v = 'c';
}

using var_t = std::variant<int, char>;
using var_tt = var_t;
using int_t = int;
using char_t = char;


//----------------------------------------------------------------------------//
// std::get
//----------------------------------------------------------------------------//
// Verify that we warn when we try to get the wrong type out of variant by
// passing the index of the type and we do not warn when we try to get
// the stored type
void stdGetIntegral() {
  std::variant<int, char> v = 25;
  // variant t is here to see wether we can distinguish between two variants
  // variants are identifieb by their memory region
  std::variant<int, char> t = 'c';
  int a = std::get<0>(v);
  char c = std::get<1>(v); // expected-warning {{variant 'v' held a(n) int not a(n) char}}
  (void*)a;
  (void*)c;
}

void stdGetType() {
  std::variant<int, char> v = 25;
  int a = std::get<int>(v);
  char c = std::get<char>(v); // expected-warning {{variant 'v' held a(n) int not a(n) char}}
  (void*)a;
  (void*)c;
}

void stdGetPointer() {
  std::variant<int*, char> v = new int;
  int *a = std::get<int*>(v);
  char c = std::get<char>(v); // expected-warning {{variant 'v' held a(n) int * not a(n) char}}
  (void**)a;
  (void*)c;
}

void stdGetObject() {
  std::variant<int, char, Foo> v = Foo{};
  Foo f = std::get<Foo>(v);
  int i = std::get<int>(v); // expected-warning {{variant 'v' held a(n) Foo not a(n) int}}
  (void*)i;
}

void stdGetPointerAndPointee() {
  int a = 5;
  std::variant<int, int*> v = &a;
  int *b = std::get<int*>(v);
  int c = std::get<int>(v); // expected-warning {{variant 'v' held a(n) int * not a(n) int}}
  (void*)c;
  (void**)b;
}

//----------------------------------------------------------------------------//
// Constructors and assignments
//----------------------------------------------------------------------------//
void copyConstructor() {
  std::variant<int, char> v = 25;
  std::variant<int, char> t(v);
  int a = std::get<int> (t);
  char c = std::get<char> (t); // expected-warning {{variant 't' held a(n) int not a(n) char}}
  (void*)a;
  (void*)c;
}

void copyAssignemntOperator() {
  std::variant<int, char> v = 25;
  std::variant<int, char> t = 'c';
  t = v;
  int a = std::get<int> (t);
  char c = std::get<char> (t); // expected-warning {{variant 't' held a(n) int not a(n) char}}
  (void*)a;
  (void*)c;
}

void assignemntOperator() {
  std::variant<int, char> v = 25;
  int a = std::get<int> (v);
  (void*)a;
  v = 'c';
  char c = std::get<char>(v);
  a = std::get<int>(v); // expected-warning {{variant 'v' held a(n) char not a(n) int}} 
  (void*)a;
  (void*)c;
}

void defaultConstructor() {
  std::variant<int, char> v;
  int i = std::get<int>(v);
  char c = std::get<char>(v); // expected-warning {{variant 'v' held a(n) int not a(n) char}}
  (void*)i;
  (void*)c;
}

void temporaryObjectsConstructor() {
  std::variant<int, char> v(std::variant<int, char>('c'));
  char c = std::get<char>(v);
  int a = std::get<int>(v); // expected-warning {{variant 'v' held a(n) char not a(n) int}}
  (void*)a;
  (void*)c;
}

void temporaryObjectsAssignment() {
  std::variant<int, char> v = std::variant<int, char>('c');
  char c = std::get<char>(v);
  int a = std::get<int>(v); // expected-warning {{variant 'v' held a(n) char not a(n) int}}
  (void*)a;
  (void*)c;
}

void pointerTypeHeld() {
  std::variant<int*, char> v = new int;
  int *a = std::get<int*>(v);
  char c = std::get<char>(v); // expected-warning {{variant 'v' held a(n) int * not a(n) char}}
  (void**)a;
  (void*)c;
}

int idk() {return 5;}
void justToSee () {
  std::variant<int, char> v = 0;
  int jts = idk();
  jts = idk();
  jts = std::get<int>(v);
  int res = 5/jts;

}

//----------------------------------------------------------------------------//
// typedef
//----------------------------------------------------------------------------//

void typefdefedVariant() {
  var_t v = 25;
  int a = std::get<int>(v);
  char c = std::get<char>(v); // expected-warning {{variant 'v' held a(n) int not a(n) char}}
  (void*)a;
  (void*)c;
}

void typedefedTypedfefedVariant() {
  var_tt v = 25;
  int a = std::get<int>(v);
  char c = std::get<char>(v); // expected-warning {{variant 'v' held a(n) int not a(n) char}}
  (void*)a;
  (void*)c;
}

void typdefedGet() {
  std::variant<char, int> v = 25;
  int a = std::get<int_t>(v);
  char c = std::get<char_t>(v); // expected-warning {{variant 'v' held a(n) int not a(n) char}}
  (void*)a;
  (void*)c;
}

void typedefedPack() {
  std::variant<int_t, char_t> v = 25;
  int a = std::get<int>(v);
  char c = std::get<char>(v); // expected-warning {{variant 'v' held a(n) int not a(n) char}}
  (void*)a;
  (void*)c;
}

void fromVarianble() {
  char o = 'c';
  std::variant<int, char> v(o);
  char c = std::get<char>(v);
  int a = std::get<int>(v); // expected-warning {{variant 'v' held a(n) char not a(n) int}}
  (void*)a;
  (void*)c;
}

void unknowValueButKnownType() {
  char o = getUnknownChar();
  std::variant<int, char> v(o);
  char c = std::get<char>(v);
  int a = std::get<int>(v); // expected-warning {{variant 'v' held a(n) char not a(n) int}}
  (void*)a;
  (void*)c;
}

void createPointer() {
  std::variant<int, char> *v = new std::variant<int, char>(15);
  int a = std::get<int>(*v);
  //PROBLEM W VAR NAMES
  char c = std::get<char>(*v); // expected-warning {{variant  held a(n) int not a(n) char}}
  (void*)a;
  (void*)c;
}

//----------------------------------------------------------------------------//
// Passing std::variants to functions
//----------------------------------------------------------------------------//

// Verifying that we are not invalidating the memory region of a variant if
// a non inlined or inlined funtion takes it as a constant reference or pointer
void constNonInlineRef() {
  std::variant<int, char> v = 'c';
  cannotChnageRef(v);
  char c = std::get<char>(v); 
  int a = std::get<int>(v); // expected-warning {{variant 'v' held a(n) char not a(n) int}}
  (void*)a;
  (void*)c;
}

void contNonInlinePtr() {
  std::variant<int, char> v = 'c';
  cannotChnagePtr(&v);
  char c = std::get<char>(v); 
  int a = std::get<int>(v); // expected-warning {{variant 'v' held a(n) char not a(n) int}}
  (void*)a;
  (void*)c;
}

void copyInAFunction() {
  std::variant<int, char> v = 'c';
  cantDo(v);
  char c = std::get<char>(v); 
  int a = std::get<int>(v); // expected-warning {{variant 'v' held a(n) char not a(n) int}}
  (void*)a;
  (void*)c;

}

// Verifying that we can keep track of the type stored in std::variant when
// it is passed to an inlined funtion as a reference or pointer
void changeThruPointers() {
  std::variant<int, char> v = 15;
  changeVariantPtr(&v);
  char c = std::get<char> (v);
  int a = std::get<int> (v); // expected-warning {{variant 'v' held a(n) char not a(n) int}}
  (void*)a;
  (void*)c;
}

void functionCallWithCopyAssignment() {
  var_t v1 = 15;
  var_t v2 = 'c';
  swap(v1, v2);
  int a = std::get<int> (v2);
  (void*)a;
  char c = std::get<char> (v1);
  a = std::get<int> (v1); // expected-warning {{variant 'v1' held a(n) char not a(n) int}}
  (void*)a;
  (void*)c;
}

void inlineFunctionCall() {
  std::variant<int, char> v = 'c';
  changeVariantType(v);
  int a = std::get<int> (v);
  char c = std::get<char> (v); // expected-warning {{variant 'v' held a(n) int not a(n) char}}
  (void*)a;
  (void*)c;
}

// Verifying that we invalidate the mem region of std::variant when it is
// passed as a referenece or a pointer to a non inlined function
void nonInleneFunctionCall() {
  std::variant<int, char> v = 'c';
  changesToInt(v);
  int a = std::get<int> (v);
  char c = std::get<char> (v);
  (void*)a;
  (void*)c;
}

void nonInleneFunctionCallPtr() {
  std::variant<int, char> v = 'c';
  changesToInt(&v);
  int a = std::get<int> (v);
  char c = std::get<char> (v);
  (void*)a;
  (void*)c;
}


//What we do not report on, but we should
void valueHeld() {
  std::variant<int, char> v = 0;
  int a = std::get<int>(v);
  int div = 10/a; // we should report a divison by 0 here
  (void*)div;
  (void*)a;
}

void stdGetIf() {
  std::variant<int, char> v = 'c';
  int* i = std::get_if<int>(&v);
  (*i)++; //we should report a dereference of a null pointer here
  (void**)i;
}

//move constructor
//move assignment
// Temporary objects
