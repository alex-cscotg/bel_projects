library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.monster_pkg.all;

entity microtca_control is
  port(
    clk_20m_vcxo_i    : in std_logic;  -- 20MHz VCXO clock
    clk_125m_wrpll_i  : in std_logic_vector (1 downto 0);  -- 125 MHz PLL reference
    clk_osc_i         : in std_logic_vector (1 downto 0);  -- local clk from 100MHz or 125Mhz oscillator
    
    -----------------------------------------
    -- PCI express pins
    -----------------------------------------
    pcie_clk_i     : in  std_logic;
    pcie_l0_rx_i   : in  std_logic_vector;
    pcie_l0_tx_o   : out std_logic_vector;

--    nPCI_RESET     : in std_logic;
    
--    pe_smdat        : inout std_logic; -- !!!
--    pe_snclk        : out std_logic;   -- !!!
--    pe_waken        : out std_logic;   -- !!!
    
    ------------------------------------------------------------------------
    -- WR DAC signals
    ------------------------------------------------------------------------
    wr_dac_sclk_o  : out std_logic;
    wr_dac_din_o   : out std_logic;
    wr_ndac_cs_o   : out std_logic_vector(2 downto 1);
    
    -----------------------------------------------------------------------
    -- OneWire
    -----------------------------------------------------------------------
    rom_data        : inout std_logic;
    
    -----------------------------------------------------------------------
    -- lcd display
    -----------------------------------------------------------------------
    dis_di_o        : out std_logic_vector(6 downto 0);
    dis_ai_i        : in  std_logic_vector(1 downto 0);
    dis_do_i        : in  std_logic;
    dis_wr_o        : out std_logic := '0';
    dis_rst_o       : out std_logic := '1';
    
    -----------------------------------------------------------------------
    -- connector cpld
    -----------------------------------------------------------------------
    con             : out std_logic_vector(5 downto 1);
    
    -----------------------------------------------------------------------
    -- io
    -----------------------------------------------------------------------
    fpga_res        : in std_logic;
    nres            : in std_logic;
    pbs_f_i         : in std_logic;

    hpwck           : out   std_logic;
    hpw             : inout std_logic_vector(15 downto 0) := (others => 'Z'); -- logic analyzer
    
    -----------------------------------------------------------------------
    -- lvds/lvttl lemos on front panel
    -----------------------------------------------------------------------
    lvtio_in_n_i     : in  std_logic_vector(5 downto 1);
    lvtio_in_p_i     : in  std_logic_vector(5 downto 1);
    lvtio_out_n_o    : out std_logic_vector(5 downto 1);
    lvtio_out_p_o    : out std_logic_vector(5 downto 1);
    lvtio_oen_o      : out std_logic_vector(5 downto 1);
    lvtio_term_en_o  : out std_logic_vector(5 downto 1);
    lvtio_act_led_o  : out std_logic_vector(5 downto 1);
    lvtio_dir_led_o  : out std_logic_vector(5 downto 1);

    -- clock input
    lvtclk_n_i       : in  std_logic;
    lvtclk_p_i       : in  std_logic;
    lvtclk_in_en_o   : out std_logic;

    -----------------------------------------------------------------------
    -- lvds/lvds libera triggers on backplane
    -----------------------------------------------------------------------
    lib_trig_n_o        : out std_logic_vector(3 downto 0);
    lib_trig_p_o        : out std_logic_vector(3 downto 0);
    lib_trig_oe_o       : out std_logic;

    -----------------------------------------------------------------------
    -- lvds/m-lvds microTCA.4 triggers, gates, clocks on backplane
    -----------------------------------------------------------------------
    mlvdio_in_n_i     : in  std_logic_vector(8 downto 1);
    mlvdio_in_p_i     : in  std_logic_vector(8 downto 1);
    mlvdio_out_n_o    : out std_logic_vector(8 downto 1);
    mlvdio_out_p_o    : out std_logic_vector(8 downto 1);
    mlvdio_oe_o       : out std_logic_vector(8 downto 1);
    mlvdio_fsen_o     : out std_logic_vector(8 downto 1);
    mlvdio_pdn_o      : out std_logic; -- output buffer powerdown, active low

    -----------------------------------------------------------------------
    -- lvds/lvds microTCA.4 backplane clocks
    -----------------------------------------------------------------------
    tclk_in_n_i      : in  std_logic_vector(4 downto 1);
    tclk_in_p_i      : in  std_logic_vector(4 downto 1);
    tclk_out_n_o     : out std_logic_vector(4 downto 1);
    tclk_out_p_o     : out std_logic_vector(4 downto 1);
    tclk_oe_o        : out std_logic_vector(4 downto 1);

    -----------------------------------------------------------------------
    -- mmc
    -----------------------------------------------------------------------
 		mmc_pcie_en_i	        : in  std_logic;
		fpga2mmc_int_o	      : out std_logic;
		mmc2fpga_usr_1_i	    : in  std_logic;
		mmc2fpga_usr_2_i	    : in  std_logic;
		mmc_spi0_sck_i	      : in  std_logic;
		mmc_spi0_miso_o 	    : out std_logic;
		mmc_spi0_mosi_i 	    : in  std_logic;
		mmc_spi0_sel_fpga_n_i : in  std_logic;


    -----------------------------------------------------------------------
    -- usb
    -----------------------------------------------------------------------
    slrd            : out   std_logic;
    slwr            : out   std_logic;
    fd              : inout std_logic_vector(7 downto 0) := (others => 'Z');
    pa              : inout std_logic_vector(7 downto 0) := (others => 'Z');
    ctl             : in    std_logic_vector(2 downto 0);
    uclk            : in    std_logic;
    ures            : out   std_logic;
    ifclk           : out   std_logic;
    
    -----------------------------------------------------------------------
    -- leds (6 LEDs for WR and FTRN status)
    -----------------------------------------------------------------------
    led_status      : out std_logic_vector(6 downto 1) := (others => '1');
    led_user        : out std_logic_vector(8 downto 1) := (others => '1');
    
    -----------------------------------------------------------------------
    -- leds SFPs
    -----------------------------------------------------------------------
    ledsfpr          : out std_logic_vector(4 downto 1);
    ledsfpg          : out std_logic_vector(4 downto 1);
    sfp_ref_clk_i : in  std_logic;

    -----------------------------------------------------------------------
    -- SFP 
    -----------------------------------------------------------------------
    
    sfp_tx_dis_o     : out std_logic := '0';
    sfp_tx_fault_i   : in std_logic;
    sfp_los_i        : in std_logic;
    
    sfp_txp_o        : out std_logic;
    sfp_rxp_i        : in  std_logic;
    
    sfp_mod0         : in    std_logic; -- grounded by module
    sfp_mod1         : inout std_logic; -- SCL
    sfp_mod2         : inout std_logic); -- SDA
    
end microtca_control;

architecture rtl of microtca_control is

  -- white rabbits leds
  signal led_link_up  : std_logic;
  signal led_link_act : std_logic;
  signal led_track    : std_logic;
  signal led_pps      : std_logic;
  
  -- front end leds
  signal s_led_frnt_red  : std_logic;
  signal s_led_frnt_blue : std_logic;
  
  -- user leds (on board)
  signal s_leds_user : std_logic_vector(3 downto 0);
  
  -- lvds
  signal s_lvds_p_i     : std_logic_vector(4 downto 0);
  signal s_lvds_n_i     : std_logic_vector(4 downto 0);
  signal s_lvds_i_led   : std_logic_vector(4 downto 0);
  signal s_lvds_p_o     : std_logic_vector(4 downto 0);
  signal s_lvds_n_o     : std_logic_vector(4 downto 0);
  signal s_lvds_o_led   : std_logic_vector(4 downto 0);
  signal s_lvds_oen     : std_logic_vector(4 downto 0);
  
  constant c_family  : string := "Arria V"; 
  constant c_project : string := "microtca_control";
  constant c_initf   : string := c_project & ".mif" 
  -- projectname is standard to ensure a stub mif that prevents unwanted scanning of the bus 
  -- multiple init files for n processors are to be seperated by semicolon ';'
  
begin

  main : monster
    generic map(
      g_family      => c_family,
      g_project     => c_project,
      g_flash_bits  => 25,
      g_gpio_out    => 6, -- 2xfront end+4xuser leds
      g_lvds_inout  => 5, -- front end lemos
      g_lvds_invert => true,
      g_en_pcie     => true,
      g_en_usb      => true,
      g_en_lcd      => true,
      g_lm32_init_files => c_initf
    )
    port map(
      core_clk_20m_vcxo_i    => clk_20m_vcxo_i,
      core_clk_125m_pllref_i => clk_125m_pllref_i,
      core_clk_125m_sfpref_i => sfp234_ref_clk_i,
      core_clk_125m_local_i  => clk_125m_local_i,
      core_rstn_i            => pbs2,
      wr_onewire_io          => rom_data,
      wr_sfp_sda_io          => sfp4_mod2,
      wr_sfp_scl_io          => sfp4_mod1,
      wr_sfp_det_i           => sfp4_mod0,
      wr_sfp_tx_o            => sfp4_txp_o,
      wr_sfp_rx_i            => sfp4_rxp_i,
      wr_dac_sclk_o          => dac_sclk,
      wr_dac_din_o           => dac_din,
      wr_ndac_cs_o           => ndac_cs,
      gpio_o(5 downto 2)     => s_leds_user(3 downto 0),
      gpio_o(1)              => s_led_frnt_blue,
      gpio_o(0)              => s_led_frnt_red,
      lvds_p_i               => s_lvds_p_i,
      lvds_n_i               => s_lvds_n_i,
      lvds_i_led_o           => s_lvds_i_led,
      lvds_p_o               => s_lvds_p_o,
      lvds_n_o               => s_lvds_n_o,
      lvds_o_led_o           => s_lvds_o_led,
      lvds_oen_o             => s_lvds_oen,
      led_link_up_o          => led_link_up,
      led_link_act_o         => led_link_act,
      led_track_o            => led_track,
      led_pps_o              => led_pps,
      pcie_refclk_i          => pcie_refclk_i,
      pcie_rstn_i            => nPCI_RESET,
      pcie_rx_i              => pcie_rx_i,
      pcie_tx_o              => pcie_tx_o,
      usb_rstn_o             => ures,
      usb_ebcyc_i            => pa(3),
      usb_speed_i            => pa(0),
      usb_shift_i            => pa(1),
      usb_readyn_io          => pa(7),
      usb_fifoadr_o          => pa(5 downto 4),
      usb_sloen_o            => pa(2),
      usb_fulln_i            => ctl(1),
      usb_emptyn_i           => ctl(2),
      usb_slrdn_o            => slrd,
      usb_slwrn_o            => slwr,
      usb_pktendn_o          => pa(6),
      usb_fd_io              => fd,
      lcd_scp_o              => di(3),
      lcd_lp_o               => di(1),
      lcd_flm_o              => di(2),
      lcd_in_o               => di(0));

  -- SFP1-3 are not mounted
  sfp1_tx_disable_o <= '1';
  sfp2_tx_disable_o <= '1';
  sfp3_tx_disable_o <= '1';
  sfp4_tx_disable_o <= '0';

  -- Link LEDs
  wrdis <= '0';
  dres  <= '1';
  di(5) <= '0' when (not led_link_up)                   = '1' else 'Z'; -- red
  di(6) <= '0' when (    led_link_up and not led_track) = '1' else 'Z'; -- blue
  di(4) <= '0' when (    led_link_up and     led_track) = '1' else 'Z'; -- green

  -- Front end: 6 LEDs for WR and FTRN status (from left to right: red, blue, green, white, red, blue)
  led(1)               <= not (led_link_act and led_link_up); -- red   = traffic/no-link
  led(2)               <= not led_link_up;                    -- blue  = link
  led(3)               <= not led_track;                      -- green = timing valid
  led(4)               <= not led_pps;                        -- white = PPS
  led(5)               <= s_led_frnt_red;                     -- red   = generic front end - gpio0
  led(6)               <= s_led_frnt_blue;                    -- blue  = generic front end - gpio1
  
  -- On board/user leds: 8 leds (from left to right: white, green, blue, red, white, green, blue, red)
  led_user(1)          <= not (led_link_act and led_link_up); -- red   = traffic/no-link
  led_user(2)          <= not led_link_up;                    -- blue  = link
  led_user(3)          <= not led_track;                      -- green = timing valid
  led_user(4)          <= not led_pps;                        -- white = PPS
  led_user(8 downto 5) <= s_leds_user;                        -- gpio5 ... gpio2
  
  -- sfp leds
  ledsfpg(3 downto 1) <= (others => '1');
  ledsfpr(3 downto 1) <= (others => '1');
  ledsfpg(4) <= not led_link_up;
  ledsfpr(4) <= not led_link_act;
  
  -- wires to CPLD, currently unused
  con <= (others => 'Z');
  
  -- lvds/lemos in/out
  s_lvds_p_i(4 downto 0) <= lvds_p_i(4 downto 0);
  s_lvds_n_i(4 downto 0) <= lvds_n_i(4 downto 0);
  lvds_p_o(4 downto 0)   <= s_lvds_p_o(4 downto 0);
  lvds_n_o(4 downto 0)   <= s_lvds_n_o(4 downto 0);
  
  -- lvds/lemos output enable
  lvds_ctrl_oen_o(0) <= '0' when s_lvds_oen(0)='0' else 'Z'; -- LVTTL_IO1
  lvds_ctrl_oen_o(1) <= '0' when s_lvds_oen(1)='0' else 'Z'; -- LVTTL_IO2
  lvds_ctrl_oen_o(2) <= '0' when s_lvds_oen(2)='0' else 'Z'; -- LVTTL_IO3
  lvds_ctrl_oen_o(3) <= '0' when s_lvds_oen(3)='0' else 'Z'; -- LVTTL_IO4
  lvds_ctrl_oen_o(4) <= '0' when s_lvds_oen(4)='0' else 'Z'; -- LVTTL_IO5
  
  -- lvds/lemos terminator (terminate on input mode)
  lvds_ctrl_term_o(0) <= '1' when s_lvds_oen(0)='1' else '0';
  lvds_ctrl_term_o(1) <= '1' when s_lvds_oen(1)='1' else '0';
  lvds_ctrl_term_o(2) <= '1' when s_lvds_oen(2)='1' else '0';
  lvds_ctrl_term_o(3) <= '1' when s_lvds_oen(3)='1' else '0';
  lvds_ctrl_term_o(4) <= '1' when s_lvds_oen(4)='1' else '0';
  
  -- lvds/lemos direction leds (blue)
  lvds_led_ndir_o(0) <= not(s_lvds_oen(0));
  lvds_led_ndir_o(1) <= not(s_lvds_oen(1));
  lvds_led_ndir_o(2) <= not(s_lvds_oen(2));
  lvds_led_ndir_o(3) <= not(s_lvds_oen(3));
  lvds_led_ndir_o(4) <= not(s_lvds_oen(4));
  
  -- lvds/lemos activity leds (red)
  lvds_led_nact_o(0) <= not(s_lvds_i_led(0)) or not(s_lvds_o_led(0));
  lvds_led_nact_o(1) <= not(s_lvds_i_led(1)) or not(s_lvds_o_led(1));
  lvds_led_nact_o(2) <= not(s_lvds_i_led(2)) or not(s_lvds_o_led(2));
  lvds_led_nact_o(3) <= not(s_lvds_i_led(3)) or not(s_lvds_o_led(3));
  lvds_led_nact_o(4) <= not(s_lvds_i_led(4)) or not(s_lvds_o_led(4));
  
end rtl;
