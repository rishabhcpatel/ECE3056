-- Name: Rishabh C. Patel
-- GTID: 903163046

-- My changes: The stall signal stall_signal generated in the control stage
-- has been brought in to. The stall signal will determine whether to update
-- PC. Several test cases have also been added to this module. 

-- ECE 3056: Architecture, Concurrency and Energy in Computation
-- Sudhakar Yalamanchili
-- Pipelined MIPS Processor VHDL Behavioral Mode--
--
--
-- Instruction fetch behavioral model. Instruction memory is
-- provided within this model. IF increments the PC,  
-- and writes the appropriate output signals. 

Library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.Std_logic_arith.all;
USE IEEE.STD_LOGIC_UNSIGNED.ALL;


entity fetch is 
--

port(instruction  : out std_logic_vector(31 downto 0);
	  PC_out       : out std_logic_vector (31 downto 0);
	  Branch_PC    : in std_logic_vector(31 downto 0);
	  clock, reset, PCSource:  in std_logic;

	-- New Signals
	stall_signal:  in std_logic	-- Signal created in Control
);
end fetch;

architecture behavioral of fetch is 
TYPE INST_MEM IS ARRAY (0 to 7) of STD_LOGIC_VECTOR (31 DOWNTO 0);
   SIGNAL iram : INST_MEM := (
	-- Default
--      X"8c070004",   --  lw $7, 4($0)
--      X"8C080008",   --  lw $8, 8($0) 
--      X"01074820",   --  add $9, $7, $8
--      X"ac09000c",   --  sw $9, 12($0)
--      X"1000FFFB",   --  beq $0, $0, -5 (branch back 5 words)
--      X"00000000",   --  nop
--      X"00000000",   --  nop
--      X"00000000"    --	 nop

	-- Branch
--X"00008020", -- add $s0 $zero $zero
--X"02008820", -- add $s1 $s0 $zero
--X"1000FFFD", -- beq $zero $zero -2
--X"01090020", -- add $zero $t0 $t1
--X"012A8020", -- add $s0 $t1 $t2
--X"8C10000C", -- lw $s0 12($zero)
--X"00000000",   --  nop
--X"00000000"    --	 nop
 
	-- Complex Branch with forwarding
--X"01084020", -- add $t0 $t0 $t0
--X"01284820", -- add $t1 $t1 $t0
--X"01285020", -- add $t2 $t1 $t0
--X"02008820", -- add $s1 $s0 $zero
--X"1000FFFD", -- beq $zero $zero -2
--X"01090020", -- add $zero $t0 $t1
--X"012A8020", -- add $s0 $t1 $t2
--X"00008020" -- add $s0 $zero $zero    

	-- Load to use stall and flushing
X"8C10000C", -- lw $s0 12($zero)
X"8C110004", -- lw $s1 4($zero)
X"02308820", -- add $s1 $s1 $s0
X"00008020", -- add $s0 $zero $zero
X"1000FFFD", -- beq $zero $zero -2
X"01090020", -- add $zero $t0 $t1
X"012A8020", -- add $s0 $t1 $t2
X"ac09000c"   --  sw $9, 12($0)               
   );
   
   SIGNAL PC, Next_PC : STD_LOGIC_VECTOR( 31 DOWNTO 0 );

BEGIN 						
-- access instruction pointed to by current PC
-- and increment PC by 4. This is combinational
		             
Instruction <=  iram(CONV_INTEGER(PC(4 downto 2)));  -- since the instruction
                                                     -- memory is indexed by integer
PC_out<= (PC + 4) when stall_signal = '0' else
		PC;					-- Add condition to update PC depending the Stall signal
   
-- compute value of next PC
	-- Add Logic for Next_PC depending on the Stall signal
Next_PC <=  (PC + 4)    when PCSource = '0' and stall_signal = '0' else
            Branch_PC    when PCSource = '1' and stall_signal = '0' else
		PC when stall_signal = '1' else
            X"CCCCCCCC";
			   
-- update the PC on the next clock			   
	PROCESS
		BEGIN
			WAIT UNTIL (rising_edge(clock));
			IF (reset = '1') THEN
				PC<= X"00000000" ;
			ELSE 
				PC <= Next_PC;    -- cannot read/write a port hence need to duplicate info
			 end if; 
			 
	END PROCESS; 

 end behavioral;