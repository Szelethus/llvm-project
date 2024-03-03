// RUN: %check_clang_tidy %s bugprone-tagged-union-member-count %t --

enum union_tag1 {
	union_tag1,
	union_tag2,
	union_tag3,
};

struct tagged_union1 {
	enum union_tag1 tag;
	// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: struct 'tagged_union1' has more data members in it's union, than the number of distinct tags [bugprone-tagged-union-member-count]
	union {
		short *shorts;
		int *ints;
		float *floats;
		double *doubles;
	} data;
};

