module S1_T(
	input [31:0] x,
	output [31:0] out
);

wire r1_output, r2_output, r3_output;

RR r1(.rr_input(x),
	.rr_output(r1_output),
	.rr_bits(6)
);

RR r2(.rr_input(x),
	.rr_output(r2_output),
	.rr_bits(11)
);

RR r3(.rr_input(x),
	.rr_output(r3_output),
	.rr_bits(25)
);

assign out = r1_output ^ r2_output ^ r3_output;

endmodule