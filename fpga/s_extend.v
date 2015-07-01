module s_extend (
	input [31:0] wi16,
	input [31:0] wi15,
	input [31:0] wi7,
	input [31:0] wi2,
	output [31:0] wo
);

w[i] = w[i-16] + s0(w[i-15]) + w[i-7] + s1(w[i-2]);

wire s0_output, s1_output;
s0 s0(s0_input(wi15), s0_output(s0_output));
s1 s1(s1_input(wi2), s1_output(s1_output));
wo = wi16 + s0_output + wi7 +s1_output;

endmodule