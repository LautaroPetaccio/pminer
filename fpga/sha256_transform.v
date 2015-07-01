/* Defines a way to calculate 32 bit positions */
`define IDX(x) (((x)+1)*(32)-1):((x)*(32))

module sha256_transform(
	input clk, reset,
	input [511:0] data,
	input [255:0] state,
	output [255:0] out,
	output finished,
);

/* K Constants */
localparam Ks = {
	32'h428a2f98, 32'h71374491, 32'hb5c0fbcf, 32'he9b5dba5,
	32'h3956c25b, 32'h59f111f1, 32'h923f82a4, 32'hab1c5ed5,
	32'hd807aa98, 32'h12835b01, 32'h243185be, 32'h550c7dc3,
	32'h72be5d74, 32'h80deb1fe, 32'h9bdc06a7, 32'hc19bf174,
	32'he49b69c1, 32'hefbe4786, 32'h0fc19dc6, 32'h240ca1cc,
	32'h2de92c6f, 32'h4a7484aa, 32'h5cb0a9dc, 32'h76f988da,
	32'h983e5152, 32'ha831c66d, 32'hb00327c8, 32'hbf597fc7,
	32'hc6e00bf3, 32'hd5a79147, 32'h06ca6351, 32'h14292967,
	32'h27b70a85, 32'h2e1b2138, 32'h4d2c6dfc, 32'h53380d13,
	32'h650a7354, 32'h766a0abb, 32'h81c2c92e, 32'h92722c85,
	32'ha2bfe8a1, 32'ha81a664b, 32'hc24b8b70, 32'hc76c51a3,
	32'hd192e819, 32'hd6990624, 32'hf40e3585, 32'h106aa070,
	32'h19a4c116, 32'h1e376c08, 32'h2748774c, 32'h34b0bcb5,
	32'h391c0cb3, 32'h4ed8aa4a, 32'h5b9cca4f, 32'h682e6ff3,
	32'h748f82ee, 32'h78a5636f, 32'h84c87814, 32'h8cc70208,
	32'h90befffa, 32'ha4506ceb, 32'hbef9a3f7, 32'hc67178f2};

/* Copies data (16 32bits uint in big endian) */
wire [511:0] big_endian_data;
data_to_be(.in(data), .out(big_endian_data));

reg [511:0] w_reg;
wire [511:0] w_next;
wire [31:0] wo_next;

/* Local state */
reg [31:0] a_reg;
wire [31:0] a_next;
reg [31:0] b_reg;
wire [31:0] b_next;
reg [31:0] c_reg;
wire [31:0] c_next;
reg [31:0] d_reg;
wire [31:0] d_next;
reg [31:0] e_reg;
wire [31:0] e_next;
reg [31:0] f_reg;
wire [31:0] f_next;
reg [31:0] g_reg;
wire [31:0] g_next;
reg [31:0] h_reg;
wire [31:0] h_next;

/* T1 & T2 */
wire [31:0] t1;
wire [31:0] t2;
wire [31:0] s0_out;
wire [31:0] s1_out;
wire [31:0] maj_out;

/* States */

localparam [1:0]
	initial = 2'b00,
	mainloop = 2'b01,
	ending = 2'b10;
	ready = 2'b11;

reg [2:0] state_reg, state_next;
reg [7:0] counter_reg, counter_next;

/* Clock updates */
always @(posedge clk)
begin
	if(~reset) 
		begin
		state_reg <= initial;
		end
	else
		begin
			a_reg <= a_next;
			b_reg <= b_next;
			c_reg <= c_next;
			d_reg <= d_next;
			e_reg <= e_next;
			f_reg <= f_next;
			g_reg <= g_next;
			h_reg <= h_next;
			state_reg <= state_next;
			counter_reg <= counter_next;
			w_reg <= w_next;
		end
end


/* Main loop */
always @*
begin
	state_next = state_reg;

	case(state_reg)
		initial:
			begin
				/* Copies last state data */
				a_next = state[`IDX(0)];
				b_next = state[`IDX(1)];
				c_next = state[`IDX(2)];
				d_next = state[`IDX(3)];
				e_next = state[`IDX(4)];
				f_next = state[`IDX(5)];
				g_next = state[`IDX(6)];
				h_next = state[`IDX(7)];
				state_next = mainloop;
				w_next = big_endian_data;
				counter_next = 0;
			end
		mainloop:
			begin
				/* Common functions */
				Ch ch(.x(e_reg), .y(f_reg), .z(g_reg), .out(ch_out));
				S1_T S1(.x(e_reg), .out(s1_out));
				S0_T S0(.x(a_reg), .out(s0_out));
				Maj maj(.x(a_reg), .y(b_reg), .z(c_reg), .out(maj_out));
				// TODO VER SI FUNCIONA IDX CON counter_reg
				t1 = h_reg + s1_out + ch_out + Ks[`IDX(counter_reg)] + w_reg[31:0];
				t2 = s0_out + maj_out;

				/* State update */
				h_next = g_reg;
				g_next = f_reg;
				f_next = e_reg;
				e_next = d_reg + t1_out;
				d_next = c_reg;
				c_next = b_reg;
				b_next = a_reg;
				a_next = t1_out + t2_out;

				/* Updates W */
				s_extend ext(.wi16(`IDX(0)),
							.wi15(`IDX(1)),
							.wi7(`IDX(8)),
							.wi2(`IDX(13)),
							.wo(wo_next));
				w_next = {w_reg[479:0], wo_next}

				/* Updates iteration number */
				counter_next = counter_reg + 1;
			end
		ending:
			begin
				/* Sets the output */
				out[`IDX(0)] = a_reg;
				out[`IDX(1)] = b_reg;
				out[`IDX(2)] = c_reg;
				out[`IDX(3)] = d_reg;
				out[`IDX(4)] = e_ref;
				out[`IDX(5)] = f_reg;
				out[`IDX(6)] = g_reg;
				state_next = ready;
			end
		ready:
			finished = 1'b1;



endmodule // sha256_transform