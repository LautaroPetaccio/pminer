module Ch(
	input [31:0] x,
	input [31:0] y,
	input [31:0] z,
	output [31:0] out
);
	assign out = ((x & (y ^ z)) ^ z);

endmodule