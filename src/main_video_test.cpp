/*****************************************************************//**
 * @file main_video_test.cpp
 *
 * @brief Basic test of 4 basic i/o cores
 *
 * @author p chu
 * @version v1.0: initial release
 *********************************************************************/

//#define _DEBUG
#include "chu_init.h"
#include "gpio_cores.h"
#include "vga_core.h"
#include "sseg_core.h"
#include "i2c_core.h"
#include "ps2_core.h"
#include <math.h>

void test_start(GpoCore *led_p) {
   int i;

   for (i = 0; i < 20; i++) {
      led_p->write(0xff00);
      sleep_ms(50);
      led_p->write(0x0000);
      sleep_ms(50);
   }
}

/**
 * check bar generator core
 * @param bar_p pointer to Gpv instance
 */
void bar_check(GpvCore *bar_p) {
   bar_p->bypass(0);
   sleep_ms(3000);
}

/**
 * check color-to-grayscale core
 * @param gray_p pointer to Gpv instance
 */
void gray_check(GpvCore *gray_p) {
   gray_p->bypass(0);
   sleep_ms(3000);
   gray_p->bypass(1);
}

/**
 * check osd core
 * @param osd_p pointer to osd instance
 */
void osd_check(OsdCore *osd_p) {
   osd_p->set_color(0x0f0, 0x001); // dark gray/green
   osd_p->bypass(0);
   osd_p->clr_screen();
   for (int i = 0; i < 64; i++) {
      osd_p->wr_char(8 + i, 20, i);
      osd_p->wr_char(8 + i, 21, 64 + i, 1);
      sleep_ms(100);
   }
   sleep_ms(3000);
}

/**
 * test frame buffer core
 * @param frame_p pointer to frame buffer instance
 */
void frame_check(FrameCore *frame_p) {
   int x, y, color;

   frame_p->bypass(0);
   for (int i = 0; i < 10; i++) {
      frame_p->clr_screen(0x008);  // dark green
      for (int j = 0; j < 20; j++) {
         x = rand() % 640;
         y = rand() % 480;
         color = rand() % 512;
         frame_p->plot_line(400, 200, x, y, color);
      }
      sleep_ms(300);
   }
   sleep_ms(3000);
}

/**
 * test ghost sprite
 * @param ghost_p pointer to mouse sprite instance
 */
void mouse_check(SpriteCore *mouse_p) {
   int x, y;

   mouse_p->bypass(0);
   // clear top and bottom lines
   for (int i = 0; i < 32; i++) {
      mouse_p->wr_mem(i, 0);
      mouse_p->wr_mem(31 * 32 + i, 0);
   }

   // slowly move mouse pointer
   x = 0;
   y = 0;
   for (int i = 0; i < 80; i++) {
      mouse_p->move_xy(x, y);
      sleep_ms(50);
      x = x + 4;
      y = y + 3;
   }
   sleep_ms(3000);
   // load top and bottom rows
   for (int i = 0; i < 32; i++) {
      sleep_ms(20);
      mouse_p->wr_mem(i, 0x00f);
      mouse_p->wr_mem(31 * 32 + i, 0xf00);
   }
   sleep_ms(3000);
}

/**
 * test ghost sprite
 * @param ghost_p pointer to ghost sprite instance
 */
void ghost_check(SpriteCore *ghost_p) {
   int x, y;

   // slowly move mouse pointer
   ghost_p->bypass(0);
   ghost_p->wr_ctrl(0x1c);  //animation; blue ghost
   x = 0;
   y = 100;
   for (int i = 0; i < 156; i++) {
      ghost_p->move_xy(x, y);
      sleep_ms(100);
      x = x + 4;
      if (i == 80) {
         // change to red ghost half way
         ghost_p->wr_ctrl(0x04);
      }
   }
   sleep_ms(3000);
}

// external core instantiation
GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore sw(get_slot_addr(BRIDGE_BASE, S3_SW));
FrameCore frame(FRAME_BASE);
GpvCore bar(get_sprite_addr(BRIDGE_BASE, V7_BAR));
GpvCore gray(get_sprite_addr(BRIDGE_BASE, V6_GRAY));
SpriteCore ghost(get_sprite_addr(BRIDGE_BASE, V3_GHOST), 1024);
SpriteCore mouse(get_sprite_addr(BRIDGE_BASE, V1_MOUSE), 1024);
OsdCore osd(get_sprite_addr(BRIDGE_BASE, V2_OSD));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));
I2cCore CMPS2(get_slot_addr(BRIDGE_BASE, S10_I2C));
Ps2Core ps2(get_slot_addr(BRIDGE_BASE, S11_PS2));

int main() {
	const uint8_t DEV_ADDR = 0x30;
		uint8_t wbytes[2];
		uint8_t bytes[6];
		int x,y,z;
		double xGauss,yGauss,zGauss;
		double heading;
		int lbtn, rbtn, xmov, ymov;

		osd.wr_char(75, 15, 69); // N
	    osd.wr_char(2, 15, 87); // S
	    osd.wr_char(42, 28, 83); // W
	    osd.wr_char(42, 2, 78); // N
	    gray.bypass(sw.read(6));

	    ghost.move_xy(320,235);

	    uart.disp("\n\rPS2 device (1-keyboard / 2-mouse): ");
		int id = ps2.init();
		uart.disp(id);
		uart.disp("\n\r");

   while (1)
   {
		wbytes[0] = 0x07;
		wbytes[1] = 0x0E;
		CMPS2.write_transaction(DEV_ADDR,wbytes,2,1);
		CMPS2.read_transaction(DEV_ADDR,bytes,1,0);
		uart.disp("Control Register 0 Status: ");
		uart.disp(bytes[0],16);
		uart.disp("\n\r");
		uart.disp("\n\r");
		sleep_ms(8);

		wbytes[0] = 0x00;
		CMPS2.write_transaction(DEV_ADDR,wbytes,1,1);
		sleep_ms(8);
		CMPS2.read_transaction(DEV_ADDR,bytes,6,0);
		x = (int) bytes[1];
		x = (x << 8) + (int) bytes[0];
		xGauss = x * .00048828125;
		xGauss -= 16;

		y = (int) bytes[3];
		y = (y << 8) + (int) bytes[2];
		yGauss = y * .00048828125;
		yGauss -= 16;

		z = (int) bytes[5];
		z = (z << 8) + (int) bytes[4];
		zGauss = z * .00048828125;
		zGauss -= 16;

		uart.disp("X Gauss: ");
		uart.disp(xGauss);
		uart.disp("\n\r");
		uart.disp("\n\r");

		uart.disp("Y Gauss: ");
		uart.disp(yGauss);
		uart.disp("\n\r");
		uart.disp("\n\r");

		uart.disp("Z Gauss: ");
		uart.disp(zGauss);
		uart.disp("\n\r");
		uart.disp("\n\r");

		if(xGauss == 0)
		{
			if(yGauss < 0) heading = 90;
			if(yGauss >= 0) heading = 0;
		}
		else
		{
			heading = (double) atan2(yGauss,xGauss)*(180/3.141592);
			if(heading < 0) heading = heading + 360;
		}

		uart.disp("Heading: ");
		uart.disp(heading);
		uart.disp("\n\r");
		uart.disp("\n\r");


	  ps2.get_mouse_activity(&lbtn, &rbtn, &xmov, &ymov);
	   //North
	      if(heading > 315 || heading <= 45){
	          //sprite looks up
	           //up
			  if(lbtn) ghost.wr_ctrl(0x13);
			  else if(rbtn) ghost.wr_ctrl(0x0B);
			  else ghost.wr_ctrl(3);
	      }
	      //East
	      if(heading > 45 && heading <= 135){
	          //sprite looks right
	          //
			  if(lbtn) ghost.wr_ctrl(0x10);
			  else if(rbtn) ghost.wr_ctrl(0x08);
			  else ghost.wr_ctrl(0);
	      }
	      //South
	      if(heading > 135 && heading <= 225){
	          //sprite looks down
	          //ghost.wr_ctrl(1); // down
			  if(lbtn) ghost.wr_ctrl(0x11);
			  else if(rbtn) ghost.wr_ctrl(0x09);
			  else ghost.wr_ctrl(1); 	      }
	      //West
	      if(heading > 225 && heading <= 315){
	          //sprite looks left
	          //ghost.wr_ctrl(2); // left
	          if(lbtn) ghost.wr_ctrl(0x12);
	          else if(rbtn) ghost.wr_ctrl(0x0A);
	          else ghost.wr_ctrl(2);
	      }

   } // while
} //main


