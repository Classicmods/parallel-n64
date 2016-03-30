#include <stdint.h>

#include "../../../Graphics/3dmath.h"

#include "glide64_gSP.h"

void calc_point_light(VERTEX *v, float *vpos);
void uc6_obj_sprite(uint32_t w0, uint32_t w1);

void load_matrix (float m[4][4], uint32_t addr)
{
   int x,y;  // matrix index
   uint16_t *src = (uint16_t*)gfx_info.RDRAM;
   addr >>= 1;

   // Adding 4 instead of one, just to remove mult. later
   for (x = 0; x < 16; x += 4)
   {
      for (y=0; y<4; y++)
         m[x>>2][y] = (float)((((int32_t)src[(addr+x+y)^1]) << 16) | src[(addr+x+y+16)^1]) / 65536.0f;
   }
}

void glide64gSPCombineMatrices(void)
{
   MulMatrices(rdp.model, rdp.proj, rdp.combined);
   g_gdp.flags ^= UPDATE_MULT_MAT;
}

void glide64gSPSegment(int32_t seg, int32_t base)
{
   rdp.segment[seg] = base;
}

void glide64gSPClipVertex(uint32_t v)
{
   VERTEX *vtx = (VERTEX*)&rdp.vtx[v];

   vtx->scr_off = 0;
   if (vtx->x > +vtx->w)   vtx->scr_off |= 2;
   if (vtx->x < -vtx->w)   vtx->scr_off |= 1;
   if (vtx->y > +vtx->w)   vtx->scr_off |= 8;
   if (vtx->y < -vtx->w)   vtx->scr_off |= 4;
   if (vtx->w < 0.1f)      vtx->scr_off |= 16;
}

void glide64gSPLookAt(uint32_t l, uint32_t n)
{
   int8_t  *rdram_s8  = (int8_t*) (gfx_info.RDRAM  + RSP_SegmentToPhysical(l));
   int8_t dir_x = rdram_s8[11];
   int8_t dir_y = rdram_s8[10];
   int8_t dir_z = rdram_s8[9];
   rdp.lookat[n][0] = (float)(dir_x) / 127.0f;
   rdp.lookat[n][1] = (float)(dir_y) / 127.0f;
   rdp.lookat[n][2] = (float)(dir_z) / 127.0f;
   rdp.use_lookat = (n == 0) || (n == 1 && (dir_x || dir_y));
}

void glide64gSPLight(uint32_t l, int32_t n)
{
   int16_t *rdram     = (int16_t*)(gfx_info.RDRAM  + RSP_SegmentToPhysical(l));
   uint8_t *rdram_u8  = (uint8_t*)(gfx_info.RDRAM  + RSP_SegmentToPhysical(l));
   int8_t  *rdram_s8  = (int8_t*) (gfx_info.RDRAM  + RSP_SegmentToPhysical(l));

   --n;

   if (n < 8)
   {
      /* Get the data */
      rdp.light[n].nonblack  = rdram_u8[3];
      rdp.light[n].nonblack += rdram_u8[2];
      rdp.light[n].nonblack += rdram_u8[1];

      rdp.light[n].col[0]    = rdram_u8[3] / 255.0f;
      rdp.light[n].col[1]    = rdram_u8[2] / 255.0f;
      rdp.light[n].col[2]    = rdram_u8[1] / 255.0f;
      rdp.light[n].col[3]    = 1.0f;

      rdp.light[n].dir[0]    = (float)rdram_s8[11] / 127.0f;
      rdp.light[n].dir[1]    = (float)rdram_s8[10] / 127.0f;
      rdp.light[n].dir[2]    = (float)rdram_s8[9] / 127.0f;

      rdp.light[n].x         = (float)rdram[5];
      rdp.light[n].y         = (float)rdram[4];
      rdp.light[n].z         = (float)rdram[7];
      rdp.light[n].ca        = (float)rdram[0] / 16.0f;
      rdp.light[n].la        = (float)rdram[4];
      rdp.light[n].qa        = (float)rdram[13] / 8.0f;

      //g_gdp.flags |= UPDATE_LIGHTS;
   }
}

void glide64gSPLightColor( uint32_t lightNum, uint32_t packedColor )
{
   lightNum--;

   if (lightNum < 8)
   {
      rdp.light[lightNum].col[0] = _SHIFTR( packedColor, 24, 8 ) * 0.0039215689f;
      rdp.light[lightNum].col[1] = _SHIFTR( packedColor, 16, 8 ) * 0.0039215689f;
      rdp.light[lightNum].col[2] = _SHIFTR( packedColor, 8, 8 )  * 0.0039215689f;
      rdp.light[lightNum].col[3] = 255;
   }
}

void glide64gSP1Triangle( int32_t v0, int32_t v1, int32_t v2, int32_t flag )
{
   VERTEX *v[3];

   v[0] = &rdp.vtx[v0];
   v[1] = &rdp.vtx[v1];
   v[2] = &rdp.vtx[v2];

   cull_trianglefaces(v, 1, true, true, 0);
}

void glide64gSP1Quadrangle(int32_t v0, int32_t v1, int32_t v2, int32_t v3)
{
   VERTEX *v[6];

   if (rdp.skip_drawing)
      return;

   v[0] = &rdp.vtx[v0];
   v[1] = &rdp.vtx[v1];
   v[2] = &rdp.vtx[v2];
   v[3] = &rdp.vtx[v3];
   v[4] = &rdp.vtx[v0];
   v[5] = &rdp.vtx[v2];

   cull_trianglefaces(v, 2, true, true, 0);
}

void glide64gSP2Triangles(const int32_t v00, const int32_t v01, const int32_t v02, const int32_t flag0,
                    const int32_t v10, const int32_t v11, const int32_t v12, const int32_t flag1 )
{
   VERTEX *v[6];

   if (rdp.skip_drawing)
      return;

   v[0] = &rdp.vtx[v00];
   v[1] = &rdp.vtx[v01];
   v[2] = &rdp.vtx[v02];
   v[3] = &rdp.vtx[v10];
   v[4] = &rdp.vtx[v11];
   v[5] = &rdp.vtx[v12];

   cull_trianglefaces(v, 2, true, true, 0);
}

void glide64gSP4Triangles( int32_t v00, int32_t v01, int32_t v02,
                    int32_t v10, int32_t v11, int32_t v12,
                    int32_t v20, int32_t v21, int32_t v22,
                    int32_t v30, int32_t v31, int32_t v32 )
{
   VERTEX *v[12];

   if (rdp.skip_drawing)
      return;

   v[0]  = &rdp.vtx[v00];
   v[1]  = &rdp.vtx[v01];
   v[2]  = &rdp.vtx[v02];
   v[3]  = &rdp.vtx[v10];
   v[4]  = &rdp.vtx[v11];
   v[5]  = &rdp.vtx[v12];
   v[6]  = &rdp.vtx[v20];
   v[7]  = &rdp.vtx[v21];
   v[8]  = &rdp.vtx[v22];
   v[9]  = &rdp.vtx[v30];
   v[10] = &rdp.vtx[v31];
   v[11] = &rdp.vtx[v32];

   cull_trianglefaces(v, 4, true, true, 0);
}

void glide64gSPViewport(uint32_t v)
{
   int16_t *rdram     = (int16_t*)(gfx_info.RDRAM  + RSP_SegmentToPhysical( v ));

   int16_t scale_y = rdram[0] >> 2;
   int16_t scale_x = rdram[1] >> 2;
   int16_t scale_z = rdram[3];
   int16_t trans_x = rdram[5] >> 2;
   int16_t trans_y = rdram[4] >> 2;
   int16_t trans_z = rdram[7];
   if (settings.correct_viewport)
   {
      scale_x = abs(scale_x);
      scale_y = abs(scale_y);
   }
   rdp.view_scale[0] = scale_x * rdp.scale_x;
   rdp.view_scale[1] = -scale_y * rdp.scale_y;
   rdp.view_scale[2] = 32.0f * scale_z;
   rdp.view_trans[0] = trans_x * rdp.scale_x;
   rdp.view_trans[1] = trans_y * rdp.scale_y;
   rdp.view_trans[2] = 32.0f * trans_z;

   g_gdp.flags |= UPDATE_VIEWPORT;
}

void glide64gSPForceMatrix( uint32_t mptr )
{
   uint32_t address = RSP_SegmentToPhysical( mptr );

   load_matrix(rdp.combined, address);

   g_gdp.flags &= ~UPDATE_MULT_MAT;
}

void glide64gSPObjMatrix( uint32_t mtx )
{
   uint32_t addr         = RSP_SegmentToPhysical(mtx) >> 1;

   rdp.mat_2d.A          = ((int*)gfx_info.RDRAM)[(addr+0)>>1] / 65536.0f;
   rdp.mat_2d.B          = ((int*)gfx_info.RDRAM)[(addr+2)>>1] / 65536.0f;
   rdp.mat_2d.C          = ((int*)gfx_info.RDRAM)[(addr+4)>>1] / 65536.0f;
   rdp.mat_2d.D          = ((int*)gfx_info.RDRAM)[(addr+6)>>1] / 65536.0f;
   rdp.mat_2d.X          = ((int16_t*)gfx_info.RDRAM)[(addr+8)^1] / 4.0f;
   rdp.mat_2d.Y          = ((int16_t*)gfx_info.RDRAM)[(addr+9)^1] / 4.0f;
   rdp.mat_2d.BaseScaleX = ((uint16_t*)gfx_info.RDRAM)[(addr+10)^1] / 1024.0f;
   rdp.mat_2d.BaseScaleY = ((uint16_t*)gfx_info.RDRAM)[(addr+11)^1] / 1024.0f;
}

void glide64gSPObjSubMatrix( uint32_t mtx )
{
   uint32_t addr         = RSP_SegmentToPhysical(mtx) >> 1;

   rdp.mat_2d.X          = ((int16_t*)gfx_info.RDRAM)[(addr+0)^1] / 4.0f;
   rdp.mat_2d.Y          = ((int16_t*)gfx_info.RDRAM)[(addr+1)^1] / 4.0f;
   rdp.mat_2d.BaseScaleX = ((uint16_t*)gfx_info.RDRAM)[(addr+2)^1] / 1024.0f;
   rdp.mat_2d.BaseScaleY = ((uint16_t*)gfx_info.RDRAM)[(addr+3)^1] / 1024.0f;
}

void glide64gSPObjLoadTxtr(uint32_t tx)
{
   uint32_t addr = RSP_SegmentToPhysical(tx) >> 1;
   uint32_t type = ((uint32_t*)gfx_info.RDRAM)[(addr + 0) >> 1]; // 0, 1

   switch (type)
   {
      case G_OBJLT_TLUT:
         {
            uint32_t image = RSP_SegmentToPhysical(((uint32_t*)gfx_info.RDRAM)[(addr + 2) >> 1]); // 2, 3
            uint16_t phead = ((uint16_t *)gfx_info.RDRAM)[(addr + 4) ^ 1] - 256; // 4
            uint16_t pnum  = ((uint16_t *)gfx_info.RDRAM)[(addr + 5) ^ 1] + 1; // 5

            //FRDP ("palette addr: %08lx, start: %d, num: %d\n", image, phead, pnum);
            load_palette (image, phead, pnum);
         }
         break;
      case G_OBJLT_TXTRBLOCK:
         {
            uint32_t w0, w1;
            uint32_t image = RSP_SegmentToPhysical(((uint32_t*)gfx_info.RDRAM)[(addr + 2) >> 1]); // 2, 3
            uint16_t tmem  = ((uint16_t *)gfx_info.RDRAM)[(addr + 4) ^ 1]; // 4
            uint16_t tsize = ((uint16_t *)gfx_info.RDRAM)[(addr + 5) ^ 1]; // 5
            uint16_t tline = ((uint16_t *)gfx_info.RDRAM)[(addr + 6) ^ 1]; // 6

            glide64gDPSetTextureImage(
                  g_gdp.ti_format,        /* format */
                  G_IM_SIZ_8b,            /* siz */
                  1,                      /* width */
                  image                   /* address */
                  );

            g_gdp.tile[7].tmem = tmem;
            g_gdp.tile[7].size = 1;
            w0 = __RSP.w0           = 0;
            w1 = __RSP.w1           = 0x07000000 | (tsize << 14) | tline;

            glide64gDPLoadBlock(
                  ((w1 >> 24) & 0x07), 
                  (w0 >> 14) & 0x3FF, /* ul_s */
                  (w0 >>  2) & 0x3FF, /* ul_t */
                  (w1 >> 14) & 0x3FF, /* lr_s */
                  (w1 & 0x0FFF) /* dxt */
                  );
         }
         break;
      case G_OBJLT_TXTRTILE:
         {
            int line;
            uint32_t w0, w1;
            uint32_t image   = RSP_SegmentToPhysical(((uint32_t*)gfx_info.RDRAM)[(addr + 2) >> 1]); // 2, 3
            uint16_t tmem    = ((uint16_t *)gfx_info.RDRAM)[(addr + 4) ^ 1]; // 4
            uint16_t twidth  = ((uint16_t *)gfx_info.RDRAM)[(addr + 5) ^ 1]; // 5
            uint16_t theight = ((uint16_t *)gfx_info.RDRAM)[(addr + 6) ^ 1]; // 6
#if 0
            FRDP ("tile addr: %08lx, tmem: %08lx, twidth: %d, theight: %d\n", image, tmem, twidth, theight);
#endif
            line             = (twidth + 1) >> 2;

            glide64gDPSetTextureImage(
                  g_gdp.ti_format,        /* format */
                  G_IM_SIZ_8b,            /* siz */
                  line << 3,              /* width */
                  image                   /* address */
                  );

            g_gdp.tile[7].tmem = tmem;
            g_gdp.tile[7].line = line;
            g_gdp.tile[7].size = 1;

            w0 = __RSP.w0 = 0;
            w1 = __RSP.w1 = 0x07000000 | (twidth << 14) | (theight << 2);

            glide64gDPLoadTile(
                  (uint32_t)((w1 >> 24) & 0x07),      /* tile */
                  (uint32_t)((w0 >> 14) & 0x03FF),    /* ul_s */
                  (uint32_t)((w0 >> 2 ) & 0x03FF),    /* ul_t */
                  (uint32_t)((w1 >> 14) & 0x03FF),    /* lr_s */
                  (uint32_t)((w1 >> 2 ) & 0x03FF)     /* lr_t */
                  );
         }
         break;
   }
}

/*
 * Loads into the RSP vertex buffer the vertices that will be used by the 
 * gSP1Triangle commands to generate polygons.
 *
 * v  - Segment address of the vertex list  pointer to a list of vertices.
 * n  - Number of vertices (1 - 32).
 * v0 - Starting index in vertex buffer where vertices are to be loaded into.
 */
void glide64gSPVertex(uint32_t v, uint32_t n, uint32_t v0)
{
   unsigned int i;
   float x, y, z;
   uint32_t iter = 16;
   void   *vertex  = (void*)(gfx_info.RDRAM + v);

   for (i=0; i < (n * iter); i+= iter)
   {
      VERTEX *vtx = (VERTEX*)&rdp.vtx[v0 + (i / iter)];
      int16_t *rdram    = (int16_t*)vertex;
      uint8_t *rdram_u8 = (uint8_t*)vertex;
      uint8_t *color = (uint8_t*)(rdram_u8 + 12);
      y                 = (float)rdram[0];
      x                 = (float)rdram[1];
      vtx->flags        = (uint16_t)rdram[2];
      z                 = (float)rdram[3];
      vtx->ov           = (float)rdram[4];
      vtx->ou           = (float)rdram[5];
      vtx->uv_scaled    = 0;
      vtx->a            = color[0];

      vtx->x = x*rdp.combined[0][0] + y*rdp.combined[1][0] + z*rdp.combined[2][0] + rdp.combined[3][0];
      vtx->y = x*rdp.combined[0][1] + y*rdp.combined[1][1] + z*rdp.combined[2][1] + rdp.combined[3][1];
      vtx->z = x*rdp.combined[0][2] + y*rdp.combined[1][2] + z*rdp.combined[2][2] + rdp.combined[3][2];
      vtx->w = x*rdp.combined[0][3] + y*rdp.combined[1][3] + z*rdp.combined[2][3] + rdp.combined[3][3];

      vtx->uv_calculated = 0xFFFFFFFF;
      vtx->screen_translated = 0;
      vtx->shade_mod = 0;

      if (fabs(vtx->w) < 0.001)
         vtx->w = 0.001f;
      vtx->oow = 1.0f / vtx->w;
      vtx->x_w = vtx->x * vtx->oow;
      vtx->y_w = vtx->y * vtx->oow;
      vtx->z_w = vtx->z * vtx->oow;
      CalculateFog (vtx);

      gSPClipVertex(v0 + (i / iter));

      if (rdp.geom_mode & G_LIGHTING)
      {
         vtx->vec[0] = (int8_t)color[3];
         vtx->vec[1] = (int8_t)color[2];
         vtx->vec[2] = (int8_t)color[1];

         if (settings.ucode == 2 && rdp.geom_mode & G_POINT_LIGHTING)
         {
            float tmpvec[3] = {x, y, z};
            calc_point_light (vtx, tmpvec);
         }
         else
         {
            NormalizeVector (vtx->vec);
            calc_light (vtx);
         }

         if (rdp.geom_mode & G_TEXTURE_GEN)
         {
            if (rdp.geom_mode & G_TEXTURE_GEN_LINEAR)
               calc_linear (vtx);
            else
               calc_sphere (vtx);
         }

      }
      else
      {
         vtx->r = color[3];
         vtx->g = color[2];
         vtx->b = color[1];
      }
      vertex = (char*)vertex + iter;
   }
}

void glide64gSPFogFactor(int16_t fm, int16_t fo )
{
   rdp.fog_multiplier = fm;
   rdp.fog_offset     = fo;
}

void glide64gSPNumLights(int32_t n)
{
   if (n > 12)
      return;

   rdp.num_lights = n;
   g_gdp.flags |= UPDATE_LIGHTS;
}

void glide64gSPPopMatrixN(uint32_t param, uint32_t num )

{
   if (rdp.model_i > num - 1)
   {
      rdp.model_i -= num;
   }
   memcpy (rdp.model, rdp.model_stack[rdp.model_i], 64);
   g_gdp.flags |= UPDATE_MULT_MAT;
}

void glide64gSPPopMatrix(uint32_t param)
{
   switch (param)
   {
      case 0: // modelview
         if (rdp.model_i > 0)
         {
            rdp.model_i--;
            memcpy (rdp.model, rdp.model_stack[rdp.model_i], 64);
            g_gdp.flags |= UPDATE_MULT_MAT;
         }
         break;
      case 1: // projection, can't
         break;
      default:
#ifdef DEBUG
         DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to pop matrix stack below 0\n" );
         DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPPopMatrix( %s );\n",
               (param == G_MTX_MODELVIEW) ? "G_MTX_MODELVIEW" :
               (param == G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_INVALID" );
#endif
         break;
   }
}

void glide64gSPDlistCount(uint32_t count, uint32_t v)
{
   uint32_t address = RSP_SegmentToPhysical(v);

   if (__RSP.PCi >= 9 || address == 0)
      return;

   __RSP.PCi ++;  // go to the next PC in the stack
   __RSP.PC[__RSP.PCi] = address;  // jump to the address
   __RSP.count = count + 1;
}

void glide64gSPCullDisplayList( uint32_t v0, uint32_t vn )
{
	if (glide64gSPCullVertices( v0, vn ))
      gSPEndDisplayList();
}

void glide64gSPModifyVertex( uint32_t vtx, uint32_t where, uint32_t val )
{
   VERTEX *v = (VERTEX*)&rdp.vtx[vtx];

   switch (where)
   {
      case 0:
         uc6_obj_sprite(__RSP.w0, __RSP.w1);
         break;

      case G_MWO_POINT_RGBA:
         v->r = (uint8_t)(val >> 24);
         v->g = (uint8_t)((val >> 16) & 0xFF);
         v->b = (uint8_t)((val >> 8) & 0xFF);
         v->a = (uint8_t)(val & 0xFF);
         v->shade_mod = 0;
         break;

      case G_MWO_POINT_ST:
         {
            float scale = (rdp.othermode_h & RDP_PERSP_TEX_ENABLE) ? 0.03125f : 0.015625f;
            v->ou = (float)((int16_t)(val>>16)) * scale;
            v->ov = (float)((int16_t)(val&0xFFFF)) * scale;
            v->uv_calculated = 0xFFFFFFFF;
            v->uv_scaled = 1;
         }
         break;

      case G_MWO_POINT_XYSCREEN:
         {
            float scr_x = (float)((int16_t)(val>>16)) / 4.0f;
            float scr_y = (float)((int16_t)(val&0xFFFF)) / 4.0f;
            v->screen_translated = 2;
            v->sx = scr_x * rdp.scale_x + rdp.offset_x;
            v->sy = scr_y * rdp.scale_y + rdp.offset_y;
            if (v->w < 0.01f)
            {
               v->w = 1.0f;
               v->oow = 1.0f;
               v->z_w = 1.0f;
            }
            v->sz = rdp.view_trans[2] + v->z_w * rdp.view_scale[2];

            v->scr_off = 0;
            if (scr_x < 0) v->scr_off |= 1;
            if (scr_x > rdp.vi_width) v->scr_off |= 2;
            if (scr_y < 0) v->scr_off |= 4;
            if (scr_y > rdp.vi_height) v->scr_off |= 8;
            if (v->w < 0.1f) v->scr_off |= 16;
         }
         break;
      case G_MWO_POINT_ZSCREEN:
         {
            float scr_z = _FIXED2FLOAT((int16_t)_SHIFTR(val, 16, 16), 15);
            v->z_w = (scr_z - rdp.view_trans[2]) / rdp.view_scale[2];
            v->z = v->z_w * v->w;
         }
         break;
   }
}

bool glide64gSPCullVertices( uint32_t v0, uint32_t vn )
{
   unsigned i;
   uint32_t clip = 0;

	if (vn < v0)
   {
      // Aidyn Chronicles - The First Mage seems to pass parameters in reverse order.
      const uint32_t v = v0;
      v0 = vn;
      vn = v;
   }

   /* Wipeout 64 passes vn = 512, increasing MAX_VTX to 512+ doesn't fix. */
   if (vn > MAX_VTX)
      return false;

   for (i = v0; i <= vn; i++)
   {
      VERTEX *v = (VERTEX*)&rdp.vtx[i];
      // Check if completely off the screen (quick frustrum clipping for 90 FOV)
      if (v->x >= -v->w) clip |= 0x01;
      if (v->x <= v->w)  clip |= 0x02;
      if (v->y >= -v->w) clip |= 0x04;
      if (v->y <= v->w)  clip |= 0x08;
      if (v->w >= 0.1f)  clip |= 0x10;
      if (clip == 0x1F)
         return false;
   }
   return true;
}
