/*

  Box3D.ino

  Draw a rotating 3D box

  Universal uC Color Graphics Library

  Copyright (c) 2014, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this list
    of conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


  (juh) 16 Jan 2023:
    Modified as example for I2CWrapper Ucglib module. Replaced MX and MY macros with 
    variables to prevent constant I2C-polling, see comments below.
    Note that you can use this example to play around with the I2Cdeley to
    see how it impacts speed vs. reliability. See comment in the setup() function.
    

*/
/// @cond

#include <Wire.h>
#include "UcglibI2C.h"

I2Cwrapper wrapper(0x08); // each target device is represented by a wrapper...
UcglibI2C ucg(&wrapper); // ...that the pin interface needs to communicate with the controller



// define a 3d point structure
struct pt3d
{
  ucg_int_t x, y, z;
};

struct surface
{
  uint8_t p[4];
  int16_t z;
};

struct pt2d
{
  ucg_int_t x, y;
  unsigned is_visible;
};


// define the point at which the observer looks, 3d box will be centered there
/* (juh) These macros are inefficient, as the values will be retrieved constantly, even if they don't change.
    In direct mode, this will not make much of a difference, but with UcglibI2C, each(!) occurence of the macro
    will invoke a pair of receive/request-events, making the convert_3d_to_2d() function increadibly slow, 
    resulting in a fps close to 1. That's why we'll turn them into variables and initialize them in setup()
*/
//#define MX (ucg.getWidth()/2)
//#define MY (ucg.getHeight()/2)
ucg_int_t MX;
ucg_int_t MY;

// define a value that corresponds to "1"
#define U 64

// eye to screen distance (fixed)
#define ZS U

// cube edge length is 2*U
struct pt3d cube[8] =
{
  { -U, -U, U},
  { U, -U, U},
  { U, -U, -U},
  { -U, -U, -U},
  { -U, U, U},
  { U, U, U},
  { U, U, -U},
  { -U, U, -U},
};

// define the surfaces
struct surface cube_surface[6] =
{
  { {0, 1, 2, 3}, 0 },  // bottom
  { {4, 5, 6, 7}, 0 },  // top
  { {0, 1, 5, 4}, 0 },  // back

  { {3, 7, 6, 2}, 0 },  // front
  { {1, 2, 6, 5}, 0 },  // right
  { {0, 3, 7, 4}, 0 },  // left
};

// define some structures for the copy of the box, calculation will be done there
struct pt3d cube2[8];
struct pt2d cube_pt[8];

// will contain a rectangle border of the box projection into 2d plane
ucg_int_t x_min, x_max;
ucg_int_t y_min, y_max;

int16_t sin_tbl[65] = {
  0, 1606, 3196, 4756, 6270, 7723, 9102, 10394, 11585, 12665, 13623, 14449, 15137, 15679, 16069, 16305, 16384, 16305, 16069, 15679,
  15137, 14449, 13623, 12665, 11585, 10394, 9102, 7723, 6270, 4756, 3196, 1606, 0, -1605, -3195, -4755, -6269, -7722, -9101, -10393,
  -11584, -12664, -13622, -14448, -15136, -15678, -16068, -16304, -16383, -16304, -16068, -15678, -15136, -14448, -13622, -12664, -11584, -10393, -9101, -7722,
  -6269, -4755, -3195, -1605, 0
};

int16_t cos_tbl[65] = {
  16384, 16305, 16069, 15679, 15137, 14449, 13623, 12665, 11585, 10394, 9102, 7723, 6270, 4756, 3196, 1606, 0, -1605, -3195, -4755,
  -6269, -7722, -9101, -10393, -11584, -12664, -13622, -14448, -15136, -15678, -16068, -16304, -16383, -16304, -16068, -15678, -15136, -14448, -13622, -12664,
  -11584, -10393, -9101, -7722, -6269, -4755, -3195, -1605, 0, 1606, 3196, 4756, 6270, 7723, 9102, 10394, 11585, 12665, 13623, 14449,
  15137, 15679, 16069, 16305, 16384
};


void copy_cube(void)
{
  uint8_t i;
  for ( i = 0; i < 8; i++ )
  {
    cube2[i] = cube[i];
  }
}

void rotate_cube_y(uint16_t w)
{
  uint8_t i;
  int16_t x, z;
  /*
    x' = x * cos(w) + z * sin(w)
    z' = - x * sin(w) + z * cos(w)
  */
  for ( i = 0; i < 8; i++ )
  {
    x = ((int32_t)cube2[i].x * (int32_t)cos_tbl[w] + (int32_t)cube2[i].z * (int32_t)sin_tbl[w]) >> 14;
    z = (- (int32_t)cube2[i].x * (int32_t)sin_tbl[w] + (int32_t)cube2[i].z * (int32_t)cos_tbl[w]) >> 14;
    //printf("%d: %d %d --> %d %d\n", i, cube2[i].x, cube2[i].z, x, z);
    cube2[i].x = x;
    cube2[i].z = z;
  }
}

void rotate_cube_x(uint16_t w)
{
  uint8_t i;
  int16_t y, z;
  for ( i = 0; i < 8; i++ )
  {
    y = ((int32_t)cube2[i].y * (int32_t)cos_tbl[w] + (int32_t)cube2[i].z * (int32_t)sin_tbl[w]) >> 14;
    z = (- (int32_t)cube2[i].y * (int32_t)sin_tbl[w] + (int32_t)cube2[i].z * (int32_t)cos_tbl[w]) >> 14;
    cube2[i].y = y;
    cube2[i].z = z;
  }
}

void trans_cube(uint16_t z)
{
  uint8_t i;
  for ( i = 0; i < 8; i++ )
  {
    cube2[i].z += z;
  }
}

void reset_min_max(void)
{
  x_min = 0x07fff;
  y_min = 0x07fff;
  x_max = -0x07fff;
  y_max = -0x07fff;
}

// calculate xs and ys from a 3d value
void convert_3d_to_2d(struct pt3d *p3, struct pt2d *p2)
{
  int32_t t;
  p2->is_visible = 1;
  if ( p3->z >= ZS )
  {
    t = ZS;
    t *= p3->x;
    t <<= 1;
    t /= p3->z;
    if ( t >= -MX && t <= MX - 1 )
    {
      t += MX;
      p2->x = t;

      if ( x_min > t )
        x_min = t;
      if ( x_max < t )
        x_max = t;

      t = ZS;
      t *= p3->y;
      t <<= 1;
      t /= p3->z;
      if ( t >= -MY && t <= MY - 1 )
      {
        t += MY;
        p2->y = t;

        if ( y_min > t )
          y_min = t;
        if ( y_max < t )
          y_max = t;
      }
      else
      {
        p2->is_visible = 0;
      }
    }
    else
    {
      p2->is_visible = 0;
    }
  }
  else
  {
    p2->is_visible = 0;
  }
}

void convert_cube(void)
{
  uint8_t i;
  reset_min_max();
  for ( i = 0; i < 8; i++ )
  {
    convert_3d_to_2d(cube2 + i, cube_pt + i);
    //printf("%d: %d %d\n", i, cube_pt[i].x, cube_pt[i].y);
  }
}

void calculate_z(void)
{
  uint8_t i, j;
  uint16_t z;
  for ( i = 0; i < 6; i++ )
  {
    z = 0;
    for ( j = 0; j < 4; j++ )
    {
      z += cube2[cube_surface[i].p[j]].z;
    }
    z /= 4;
    cube_surface[i].z = z;
    //printf("%d: z=%d\n", i, z);
  }
}

void draw_cube(void)
{
  uint8_t i, ii;
  uint8_t skip_cnt = 3;   /* it is known, that the first 3 surfaces are invisible */
  int16_t z, z_upper;


  z_upper = 32767;
  for (;;)
  {
    ii = 6;
    z = -32767;
    for ( i = 0; i < 6; i++ )
    {
      if ( cube_surface[i].z <= z_upper )
      {
        if ( z < cube_surface[i].z )
        {
          z = cube_surface[i].z;
          ii = i;
        }
      }
    }

    if ( ii >= 6 )
      break;
    //printf("%d z=%d upper=%d\n", ii, z, z_upper);
    z_upper = cube_surface[ii].z;
    cube_surface[ii].z++;

    if ( skip_cnt > 0 )
    {
      skip_cnt--;
    }
    else
    {
      ucg.setColor(0, ((ii + 1) & 1) * 255, (((ii + 1) >> 1) & 1) * 255, (((ii + 1) >> 2) & 1) * 255);
      ucg.drawTetragon(
        cube_pt[cube_surface[ii].p[0]].x, cube_pt[cube_surface[ii].p[0]].y,
        cube_pt[cube_surface[ii].p[1]].x, cube_pt[cube_surface[ii].p[1]].y,
        cube_pt[cube_surface[ii].p[2]].x, cube_pt[cube_surface[ii].p[2]].y,
        cube_pt[cube_surface[ii].p[3]].x, cube_pt[cube_surface[ii].p[3]].y); //delay(20);
    }
  }
}


void calc_and_draw(uint16_t w, uint16_t v)
{
  copy_cube();
  rotate_cube_y(w);
  rotate_cube_x(v);
  trans_cube(U * 8);
  convert_cube();
  calculate_z();
  draw_cube();
}


void setup(void)
{

  Wire.begin();
  wrapper.reset();
  
  /* (juh) Lowering the I2Cdelay will lead to function calls being visibly skipped, i.e
   * one side of the box might not be drawn etc., or in the worst case, updating
   * will stop altogether, which means that the target device was flooded with
   * too much traffic it couldn't handle. Experiment with this to get a feeling
   * for the delays you'll need in your own code.
   */
  // wrapper.setI2Cdelay(10); 

  delay(200);
  ucg.begin(UCG_FONT_MODE_TRANSPARENT); delay(200);
  ucg.setRotate90();
  ucg.clearScreen(); delay(200);
  ucg.setFont(I2C_ucg_font_ncenR10_tr);
  //ucg.setFont(I2C_ucg_font_helvB08_hr);
  ucg.setPrintPos(0, 25);
  ucg.setColor(255, 255, 255);
  ucg.print("UcglibI2C Box3D");
  MX = ucg.getWidth() / 2;
  MY = ucg.getHeight() / 2;
}


uint16_t w = 0;
uint16_t v = 0;

void loop(void)
{
  calc_and_draw(w, v >> 3);

  v += 3;
  v &= 511;

  w++;
  w &= 63;
  delay(30);

  ucg.setColor(0, 0, 0);
  ucg.drawBox(x_min, y_min, x_max - x_min + 1, y_max - y_min + 1);
}

/// @endcond
