// RUN: %check_clang_tidy %s bugprone-tagged-union-member-count %t

enum union_tag1 {
	union_tag1,
	union_tag2,
	union_tag3,
};

struct tagged_union1 {
	enum union_tag1 tag;
  // TODO: check the actual warning msg
	// CHECK-MESSAGES: :[[@LINE+1]]:2: warning:
	union {
		short *shorts;
		int *ints;
		float *floats;
		double *doubles;
	} data;
};

