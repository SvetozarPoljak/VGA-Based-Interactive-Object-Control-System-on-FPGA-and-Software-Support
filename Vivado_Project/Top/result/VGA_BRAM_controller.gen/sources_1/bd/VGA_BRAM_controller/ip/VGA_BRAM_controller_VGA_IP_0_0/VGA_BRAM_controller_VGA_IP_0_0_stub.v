// Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2020.2 (win64) Build 3064766 Wed Nov 18 09:12:45 MST 2020
// Date        : Wed Jun  5 13:31:16 2024
// Host        : DESKTOP-3F0U81A running 64-bit major release  (build 9200)
// Command     : write_verilog -force -mode synth_stub
//               c:/Users/Korisnik/Desktop/VGA_BRAM/Vivado_Project/Top/result/VGA_BRAM_controller.gen/sources_1/bd/VGA_BRAM_controller/ip/VGA_BRAM_controller_VGA_IP_0_0/VGA_BRAM_controller_VGA_IP_0_0_stub.v
// Design      : VGA_BRAM_controller_VGA_IP_0_0
// Purpose     : Stub declaration of top-level module interface
// Device      : xc7z010clg400-1
// --------------------------------------------------------------------------------

// This empty module with port declaration file causes synthesis tools to infer a black box for IP.
// The synthesis directives are for Synopsys Synplify support to prevent IO buffer insertion.
// Please paste the declaration into a Verilog source file or add the file as an additional source.
(* X_CORE_INFO = "VGA,Vivado 2020.2" *)
module VGA_BRAM_controller_VGA_IP_0_0(clk, reset, vga_hsync, vga_vsync, vga_rgb, 
  bram_addr, bram_din, bram_en)
/* synthesis syn_black_box black_box_pad_pin="clk,reset,vga_hsync,vga_vsync,vga_rgb[15:0],bram_addr[31:0],bram_din[31:0],bram_en" */;
  input clk;
  input reset;
  output vga_hsync;
  output vga_vsync;
  output [15:0]vga_rgb;
  output [31:0]bram_addr;
  input [31:0]bram_din;
  output bram_en;
endmodule
