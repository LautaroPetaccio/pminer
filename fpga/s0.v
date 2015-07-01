module s0 (
	input [31:0] s0_input,
	output [31:0] s0_output
);

wire r1_output, r2_output, r3_output;

RR r1(.rr_input(s0_input),
	.rr_output(r1_output),
	.rr_bits(7)
);

RR r2(.rr_input(s0_input),
	.rr_output(r2_output),
	.rr_bits(18)
);

RR r3(.rr_input(s0_input),
	.rr_output(r3_output),
	.rr_bits(3)
);

assign s0_output = r1_output ^ r2_output ^ r3_output;

endmodule