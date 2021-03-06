------------------------------------------------------------------------------
-- Title      : Wishbone pSRAM / cellular RAM core
-- Project    : General Cores
------------------------------------------------------------------------------
-- File       : psram_pkg.vhd
-- Author     : Wesley W. Terpstra
-- Company    : GSI
-- Created    : 2014-12-05
-- Last update: 2014-12-05
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Maps an pSRAM chip to wishbone memory
-------------------------------------------------------------------------------
-- Copyright (c) 2013 GSI
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author          Description
-- 2014-12-05  1.0      terpstra        Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.wishbone_pkg.all;

package psram_pkg is

  function f_psram_sdb(g_bits : natural) return t_sdb_device;
  
  component psram is
    generic(
      g_bits     : natural := 24;
      g_row_bits : natural := 8);
    port(
      clk_i     : in    std_logic; -- Must be <= 66MHz, modify c_bcr_* otherwise
      rstn_i    : in    std_logic;
      slave_i   : in    t_wishbone_slave_in;
      slave_o   : out   t_wishbone_slave_out;
      ps_clk    : out   std_logic;
      ps_addr   : out   std_logic_vector(g_bits-1 downto 0);
      ps_data   : inout std_logic_vector(15 downto 0);
      ps_seln   : out   std_logic_vector(1 downto 0);
      ps_cen    : out   std_logic;
      ps_oen    : out   std_logic;
      ps_wen    : out   std_logic;
      ps_cre    : out   std_logic;
      ps_advn   : out   std_logic;
      ps_wait   : in    std_logic);
  end component;
  
end package;

package body psram_pkg is

  function f_psram_sdb(g_bits : natural) return t_sdb_device
  is
    variable result : t_sdb_device;
  begin
    result.abi_class     := x"0001"; -- Ram device
    result.abi_ver_major := x"01";
    result.abi_ver_minor := x"00";
    result.wbd_width     := x"7"; -- 8/16/32-bit port granularity
    result.wbd_endian    := c_sdb_endian_big;
    
    result.sdb_component.addr_first := (others => '0');
    result.sdb_component.addr_last  := std_logic_vector(to_unsigned(2**(g_bits+1)-1, 64));
    
    result.sdb_component.product.vendor_id := x"0000000000000651";
    result.sdb_component.product.device_id := x"169edcb7";
    result.sdb_component.product.version   := x"00000001";
    result.sdb_component.product.date      := x"20141219";
    result.sdb_component.product.name      := "Pseudo SRAM        ";
    
    return result;
  end f_psram_sdb;

end psram_pkg;
