-- Name: Rishabh C. Patel
-- GTID: 903163046

-- My Changes: in this module, the stall signal stall_signal from
-- the pervious stage has been brought in to update the PC only
-- when the stall_signal is not asserted

--
-- IF/ID stage pipeline register
--

Library IEEE;
use IEEE.std_logic_1164.all;
 

entity pipe_reg1 is
port (	if_PC4 : in std_logic_vector(31 downto 0);
	if_instruction: in std_logic_vector( 31 downto 0);
	clk, reset : in std_logic;
	id_PC4 : out std_logic_vector(31 downto 0);
	id_instruction: out std_logic_vector( 31 downto 0);

	-- New Signals
	stall_signal : in std_logic 	-- From Control
);
end pipe_reg1;

architecture behavioral of pipe_reg1 is
begin
process
begin
wait until (rising_edge(clk));
if reset = '1' then 
id_PC4 <= x"00000000";
id_instruction <= x"00000000";
elsif stall_signal = '0' then	-- Add condition to Update PC based on the stall signal
id_PC4 <= if_PC4;
id_instruction <= if_instruction;
end if;
end process;
end behavioral;