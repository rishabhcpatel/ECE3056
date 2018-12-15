-- Name: Rishabh C. Patel
-- GTID: 903163046

-- My Changes: this module implements the logic to stall a load word dependency. 
-- A new signal stall_signal has been added and other signals from from other stages
-- have been brought in. The opcode for the execute stage execute_stage_opcode will 
-- be propogated through the pipeline registers.  

--
-- control unit. simply implements the truth table for a small set of
-- instructions 
--
--

Library IEEE;
use IEEE.std_logic_1164.all;

entity control is
port(opcode: in std_logic_vector(5 downto 0);
     RegDst, MemRead, MemToReg, MemWrite :out  std_logic;
     ALUSrc, RegWrite, Branch: out std_logic;
     ALUOp: out std_logic_vector(1 downto 0);

	-- New Input Signals
execute_stage_opcode: in std_logic_vector(5 downto 0); 		-- New Created
ex_wreg_addr : in std_logic_vector(4 downto 0);
instruction : in std_logic_vector(31 downto 0);		-- From Fetch
	-- New Output Signals
stall_signal :out  std_logic			-- New Created For Stall

);	
end control;

architecture behavioral of control is

signal rformat, lw, sw, beq  :std_logic; -- define local signals
				    -- corresponding to instruction
				    -- type 
	-- New Local
	signal localLoad :std_logic;

 begin 
--
-- recognize opcode for each instruction type
-- these variable should be inferred as wires	 

	rformat 	<=  '1'  WHEN  Opcode = "000000"  ELSE '0';
	Lw          <=  '1'  WHEN  Opcode = "100011"  ELSE '0';
 	Sw          <=  '1'  WHEN  Opcode = "101011"  ELSE '0';
   	Beq         <=  '1'  WHEN  Opcode = "000100"  ELSE '0';

	-- Implement the Stall logic Here:
	stall_signal <= '1' when (opcode ="000000" and execute_stage_opcode = "100011" and localLoad ='1') else
 	  '0';
	localLoad <= '1' when (instruction(25 downto 21) = ex_wreg_addr or instruction(20 downto 16) = ex_wreg_addr) else
	'0';

--
-- implement each output signal as the column of the truth
-- table  which defines the control
--

RegDst <= rformat;
ALUSrc <= (lw or sw) ;

MemToReg <= lw ;
RegWrite <= (rformat or lw);
MemRead <= lw ;
MemWrite <= sw;	   
Branch <= beq;

ALUOp(1 downto 0) <=  rformat & beq; -- note the use of the concatenation operator
				     -- to form  2 bit signal

end behavioral;