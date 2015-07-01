module s1 (
	input [31:0] s1_input,
	output [31:0] s1_output
);

wire r1_output, r2_output, r3_output;

RR r1(.rr_input(s1_input),
	.rr_output(r1_output),
	.rr_bits(17)
);

RR r2(.rr_input(s1_input),
	.rr_output(r2_output),
	.rr_bits(19)
);

RR r3(.rr_input(s1_input),
	.rr_output(r3_output),
	.rr_bits(10)
);

assign s0_output = r1_output ^ r2_output ^ r3_output;

endmodule