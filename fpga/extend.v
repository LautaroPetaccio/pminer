module extend (
	input [511:0] w_input,
	output [2047:0] w_output
);

for (i = 16; i < 63; i = i +1)
begin
	wire s0_output, s1_output;
	s0 s0(s0_input(w_input[(i - 15) * 32]), s0_output(s0_output));
	s1 s1(s1_input(w_input[(i - 2) * 32]), s1_output(s1_output));
	w_input[i * 32] = w_input[(i * 32) - 16] + s0_output + w_input[(i * 32) - 7] + s1_output;
end

endmodule