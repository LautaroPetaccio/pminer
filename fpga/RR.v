module RR (
	input [31:0] rr_input,
	input [8:0] rr_bits,
	output [31:0] rr_output
);

assign rr_output = (rr_input << rr_bits) | (A_in >> ~rr_bits);

endmodule