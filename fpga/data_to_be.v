/* Defines a way to calculate 32 bit positions */
`define IDX(x) (((x)+1)*(32)-1):((x)*(32))

module data_to_be (
	input [511:0] data,
	output [511:0] be_data,
);

wire [31:0] out0;
wire [31:0] out1;
wire [31:0] out2;
wire [31:0] out3;
wire [31:0] out4;
wire [31:0] out5;
wire [31:0] out6;
wire [31:0] out7;
wire [31:0] out8;

swap_32bit_endian swap0(.in(data[`IDX(0)]), .out(out0));
swap_32bit_endian swap1(.in(data[`IDX(0)]), .out(out1));
swap_32bit_endian swap2(.in(data[`IDX(0)]), .out(out2));
swap_32bit_endian swap3(.in(data[`IDX(0)]), .out(out3));
swap_32bit_endian swap4(.in(data[`IDX(0)]), .out(out4));
swap_32bit_endian swap5(.in(data[`IDX(0)]), .out(out5));
swap_32bit_endian swap6(.in(data[`IDX(0)]), .out(out6));
swap_32bit_endian swap7(.in(data[`IDX(0)]), .out(out7));

endmodule