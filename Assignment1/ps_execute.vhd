-- Name: Rishabh C. Patel
-- GTID: 903163046

-- My changes: many control signals have been brought in and the Outhput from the second 
-- alusrc mux FwdBOut has been computed. Based on the control signals required, the forwarding
-- logic has been implemented and the input for both muxes are selected based on the forwarding
-- logic

---- ECE 3056: Architecture, Concurrency and Energy in Computation
-- Sudhakar Yalamanchili
-- Pipelined MIPS Processor VHDL Behavioral Mode--
--
--
-- execution unit. only a subset of instructions are supported in this
-- model, specifically add, sub, lw, sw, beq, and, or
--

Library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_arith.all;
use IEEE.std_logic_signed.all;

entity execute is
port(
--
-- inputs
-- 
     PC4 : in std_logic_vector(31 downto 0);
     register_rs, register_rt :in std_logic_vector (31 downto 0);
     Sign_extend :in std_logic_vector(31 downto 0);
     ALUOp: in std_logic_vector(1 downto 0);
     ALUSrc, RegDst : in std_logic;
     wreg_rd, wreg_rt : in std_logic_vector(4 downto 0);

-- outputs
--
     alu_result, Branch_PC :out std_logic_vector(31 downto 0);
     wreg_address : out std_logic_vector(4 downto 0);
     zero: out std_logic;

	
	
	-- New Input Signals for implement forwarding
	mem_alu_result, wb_memory_data, wb_alu_result :in std_logic_vector (31 downto 0);
	wb_RegWrite, mem_RegWrite,  wb_MemToReg : in std_logic;
	rs_field_write: in std_logic_vector(4 downto 0);	-- From Decode stage
	mem_wreg_addr, wb_wreg_addr: in std_logic_vector(4 downto 0);	-- From pipe_reg3 and pipe_reg4 stage

	-- New Output Signals
	FwdBOut :out std_logic_vector(31 downto 0)		-- Newly Created signal


);    
     end execute;

	
	
	


architecture behavioral of execute is 
SIGNAL Ainput, Binput	: STD_LOGIC_VECTOR( 31 DOWNTO 0 ); 
signal ALU_Internal : std_logic_vector (31 downto 0);
Signal Function_opcode : std_logic_vector (5 downto 0);

	-- Add New Signals
	SIGNAL BForward: STD_LOGIC_VECTOR( 31 DOWNTO 0 ); 	-- third BCondition
	SIGNAL ForwardA, ForwardB : STD_LOGIC_VECTOR( 1 DOWNTO 0 ); 	-- Output signals from the forwarding unit

SIGNAL ALU_ctl	: STD_LOGIC_VECTOR( 2 DOWNTO 0 );

BEGIN



	-- Inplement Forwarding Logic Based on the slides and textbook
	ForwardA <= "10" when (mem_wreg_addr /= X"000000" and mem_RegWrite = '1' and mem_wreg_addr = rs_field_write) else
		"01" when ((wb_wreg_addr /= X"000000" and wb_RegWrite = '1') and not(mem_wreg_addr /= X"00000" and mem_RegWrite = '1' and mem_wreg_addr = rs_field_write) and (wb_wreg_addr = rs_field_write)) else
		"00";
	ForwardB <= "10" when (mem_wreg_addr /= X"000000" and mem_RegWrite = '1' and mem_wreg_addr = wreg_rt) else
		"01" when ((wb_wreg_addr /= X"000000" and wb_RegWrite = '1') and not(mem_wreg_addr /= X"00000" and mem_RegWrite = '1' and mem_wreg_addr = wreg_rt) and (wb_wreg_addr = wreg_rt)) else
		 "00";
	
    -- compute the two ALU inputs Select Ainput abd Binput based on the forwarding selection
	Ainput <= register_rs when ForwardA = "00" else
		wb_alu_result when (wb_MemToReg = '0' and ForwardA = "01") else
		wb_memory_data when (wb_MemToReg ='1' and ForwardA = "01") else
		mem_alu_result when ForwardA = "10";
	BForward <= register_rt WHEN ForwardB ="00" else
		wb_alu_result when (wb_MemToReg = '0' and ForwardB = "01") else
		wb_memory_data when (wb_MemToReg ='1' and ForwardB = "01") else
		mem_alu_result when (ForwardB = "10") else
	        X"BBBBBBBB";
	Binput <= Sign_extend(31 downto 0) when (ALUSrc ='1') else BForward;
	         
	 Branch_PC <= PC4 + (Sign_extend(29 downto 0) & "00");
	 
	 -- Get the function field. This will be the least significant
	 -- 6 bits of  the sign extended offset
	 
	 Function_opcode <= Sign_extend(5 downto 0);
	         
		-- Generate ALU control bits
		
	ALU_ctl( 0 ) <= ( Function_opcode( 0 ) OR Function_opcode( 3 ) ) AND ALUOp(1 );
	ALU_ctl( 1 ) <= ( NOT Function_opcode( 2 ) ) OR (NOT ALUOp( 1 ) );
	ALU_ctl( 2 ) <= ( Function_opcode( 1 ) AND ALUOp( 1 )) OR ALUOp( 0 );
		
		-- Generate Zero Flag
	Zero <= '1' WHEN ( ALU_internal = X"00000000"  )
		         ELSE '0';    	
		         
	-- Write the register_rt data
	FwdBOut <= BForward;

	
-- implement the RegDst mux in this pipeline stage
--
wreg_address <= wreg_rd when RegDst = '1' else wreg_rt;
		         			   
  ALU_result <= ALU_internal;					

PROCESS ( ALU_ctl, Ainput, Binput )
	BEGIN
					-- Select ALU operation
 	CASE ALU_ctl IS
						-- ALU performs ALUresult = A_input AND B_input
		WHEN "000" 	=>	ALU_internal 	<= Ainput AND Binput; 
						-- ALU performs ALUresult = A_input OR B_input
     	WHEN "001" 	=>	ALU_internal 	<= Ainput OR Binput;
						-- ALU performs ALUresult = A_input + B_input
	 	WHEN "010" 	=>	ALU_internal 	<= Ainput + Binput;
						-- ALU performs ?
 	 	WHEN "011" 	=>	ALU_internal <= X"00000000";
						-- ALU performs ?
 	 	WHEN "100" 	=>	ALU_internal 	<= X"00000000";
						-- ALU performs ?
 	 	WHEN "101" 	=>	ALU_internal 	<=  X"00000000";
						-- ALU performs ALUresult = A_input -B_input
 	 	WHEN "110" 	=>	ALU_internal 	<= (Ainput - Binput);
						-- ALU performs SLT
  	 	WHEN "111" 	=>	ALU_internal 	<= (Ainput - Binput) ;
 	 	WHEN OTHERS	=>	ALU_internal 	<= X"FFFFFFFF" ;
  	END CASE;
  END PROCESS;
end behavioral;
