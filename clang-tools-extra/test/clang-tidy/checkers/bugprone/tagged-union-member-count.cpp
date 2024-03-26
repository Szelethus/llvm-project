// RUN: %check_clang_tidy %s bugprone-tagged-union-member-count %t

enum tags3 {
	tags3_1,
	tags3_2,
	tags3_3,
};

enum tags4 {
	tags4_1,
	tags4_2,
	tags4_3,
	tags4_4,
};

enum tags5 {
	tags5_1,
	tags5_2,
	tags5_3,
	tags5_4,
	tags5_5,
};

union union3 {
	short *shorts;
	int *ints;
	float *floats;
};

union union4 {
	short *shorts;
	double *doubles;
	int *ints;
	float *floats;
};

// ----- Warnings -----

// TODO: Implement check messages

struct taggedunion1 { // CHECK-MESSAGES: :[[@LINE]]:8: warning: Tagged union has more data members than tags! Data members: 4 Tags: 3 [bugprone-tagged-union-member-count]
	enum tags3 tag;
    union union4 data;
};

struct taggedunion5 { // CHECK-MESSAGES: :[[@LINE]]:8: warning: Tagged union has more data members than tags! Data members: 4 Tags: 3 [bugprone-tagged-union-member-count]
	enum tags3 tag;
    union {
		int *ints;
		char characters[13];
		struct {
			double re;
			double im;
		} complex;
		long l;
    } data;
};

struct taggedunion7 { // CHECK-MESSAGES: :[[@LINE]]:8: warning: Tagged union has more data members than tags! Data members: 4 Tags: 3 [bugprone-tagged-union-member-count]
	enum {
		tag1,
		tag2,
		tag3,
	} tag;
	union union4 data;
};

struct taggedunion8 { // CHECK-MESSAGES: :[[@LINE]]:8: warning: Tagged union has more data members than tags! Data members: 4 Tags: 3 [bugprone-tagged-union-member-count]
	enum {
		tag1,
		tag2,
		tag3,
	} tag;
	union {
		int *ints;
		char characters[13];
		struct {
			double re;
			double im;
		} complex;
		long l;
	} data;
};

struct nested1 { // CHECK-MESSAGES: :[[@LINE]]:8: warning: Tagged union has more data members than tags! Data members: 4 Tags: 3 [bugprone-tagged-union-member-count]
	enum tags3 tag;
	union {
		char c;
		short s;
		int i;
		struct {
			enum tags5 tag;
			union union4 data;
		} inner;
	} data;
};

struct nested2 {
	enum tags3 tag;
	union {
		float f;
		int i;
		struct { // CHECK-MESSAGES: :[[@LINE]]:3: warning: Tagged union has more data members than tags! Data members: 4 Tags: 3 [bugprone-tagged-union-member-count]
			enum tags3 tag;
			union union4 data;
		} inner;
	} data;
};

struct nested3 {
	enum tags3 tag;
	union {
		float f;
		int i;
		struct innerdecl { // CHECK-MESSAGES: :[[@LINE]]:10: warning: Tagged union has more data members than tags! Data members: 4 Tags: 3 [bugprone-tagged-union-member-count]
			enum tags3 tag;
			union union4 data;
		}; 
	} data;
};

// ----- No warnings -----

// Which enum is meant to be the tag for the data?
struct taggedunion2 {
	enum tags3 tagA;
	enum tags4 tagB;
	union union4 data;
};

// What if the tag is used for both unions?
struct taggedunion3 {
	enum tags4 tag;
	union union3 dataA;
	union union4 dataB;
};

// What if the valid member of union3 specifies the valid member of union4?
struct taggedunion4 {
	enum tags3 tag;
	union union4 dataB;
	union union3 dataA;
};

// There are valid use cases for more enum values, than data members.
struct taggedunion6 {
	enum tags5 tag;
	union union4 data;
};

